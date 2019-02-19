#include "Comm.h"
#include "PageCache.h"

PageCache PageCache::_inst;

Span* PageCache::NewSpan(size_t npage)
{
	if (!_pagelist[npage].Empty())
	{
		return _pagelist[npage].PopFront();
	}

	for (int i = npage + 1; i < NPAGE; i++)
	{
		SpanList* spanlist = &_pagelist[i];
		if (!spanlist->Empty())
		{
			Span* span = _pagelist->PopFront();
			Span* split = new Span;
			split->_page_id = span->_page_id + span->_page_num - npage;
			split->_page_num = npage;
			span->_page_num -= npage;
			_pagelist[span->_page_num].PushFront(span);

			//�������һ��ӳ��
			for (size_t i = 0; i < split->_page_num; ++i)
			{
				_id_span_map[split->_page_id + i] = split;
			}

			return split;
		}
	}
	//���û�У�PageCache����Ҫ��ϵͳ�����ڴ�
	//����һ������ҳ
	void* ptr = VirtualAlloc(NULL, (NPAGE - 1) << PAGE_SHIFT, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	if (ptr == nullptr)
	{
		throw std::bad_alloc();//���ʧ�����쳣
	}

	Span* largespan = new Span;
	largespan->_page_id = (PageId)ptr >> PAGE_SHIFT;  //���ݵ�ַ���ҳ��
	largespan->_page_num = NPAGE - 1;
	_pagelist[NPAGE - 1].PushFront(largespan);

	for (size_t i = 0; i < largespan->_page_num; ++i)
	{
		_id_span_map[largespan->_page_id + i] = largespan;
	}

	//�ߵ�����˵��PageCache�Ѿ�����Դ���������������ٵ���һ��NewSpan
	return NewSpan(npage);
}

//���ݶ������ҳ��
//����ҳ�ź�Span��ӳ���ҳ���Ӧ��Span
Span* PageCache::MapObjectToSpan(void* obj)
{
	PageId pageid = (PageId)obj >> PAGE_SHIFT;
	auto it = _id_span_map.find(pageid);
	assert(it != _id_span_map.end());

	return it->second;
}


// �ͷſ���span�ص�Pagecache�����ϲ����ڵ�span
void PageCache::ReleaseSpanToPageCahce(Span* span)
{
	//�ҵ�ǰһ��Span
	auto previt = _id_span_map.find(span->_page_id - 1);
	while (previt != _id_span_map.end())
	{
		Span* prespan = previt->second;
		//˵��ǰһ��Span���ǿ��е��޷��ϲ�
		if (prespan->_usecount != 0)
			break;
		//˵���ϲ�֮���ҳ���󳬹�����ķ�Χ
		if (prespan->_page_num + span->_page_num >= NPAGE)
			break;

		//��ʱ˵�����Ժϲ�
		_pagelist[prespan->_page_num].Earse(prespan);
		prespan->_page_num += span->_page_num;

		delete span;
		span = prespan;

		previt = _id_span_map.find(span->_page_id - 1);
	}

	//Ȼ��ϲ����ĺ�һ��
	auto nextit = _id_span_map.find(span->_page_id + span->_page_num);
	while (nextit != _id_span_map.end())
	{
		Span* nextspan = nextit->second;
		if (nextspan->_usecount == 0)
			break;
		if (nextspan->_page_num + span->_page_num >= NPAGE)
			break;

		//���Ժϲ�
		_pagelist[nextspan->_page_num].Earse(nextspan);
		span->_page_num += nextspan->_page_num;

		delete nextspan;
		nextit = _id_span_map.find(span->_page_id + span->_page_num);
	}
	//�������ӳ��
	for (size_t i = 0; i < span->_page_num; ++i)
	{
		_id_span_map[span->_page_id + i] = span;
	}
	_pagelist[span->_page_num].PushFront(span);
}
