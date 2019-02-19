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

			//重新完成一次映射
			for (size_t i = 0; i < split->_page_num; ++i)
			{
				_id_span_map[split->_page_id + i] = split;
			}

			return split;
		}
	}
	//如果没有，PageCache则需要向系统申请内存
	//申请一个最大的页
	void* ptr = VirtualAlloc(NULL, (NPAGE - 1) << PAGE_SHIFT, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	if (ptr == nullptr)
	{
		throw std::bad_alloc();//如果失败抛异常
	}

	Span* largespan = new Span;
	largespan->_page_id = (PageId)ptr >> PAGE_SHIFT;  //根据地址算出页号
	largespan->_page_num = NPAGE - 1;
	_pagelist[NPAGE - 1].PushFront(largespan);

	for (size_t i = 0; i < largespan->_page_num; ++i)
	{
		_id_span_map[largespan->_page_id + i] = largespan;
	}

	//走到这里说明PageCache已经有资源可以申请了所以再调用一次NewSpan
	return NewSpan(npage);
}

//根据对象计算页号
//根据页号和Span的映射找出对应的Span
Span* PageCache::MapObjectToSpan(void* obj)
{
	PageId pageid = (PageId)obj >> PAGE_SHIFT;
	auto it = _id_span_map.find(pageid);
	assert(it != _id_span_map.end());

	return it->second;
}


// 释放空闲span回到Pagecache，并合并相邻的span
void PageCache::ReleaseSpanToPageCahce(Span* span)
{
	//找到前一个Span
	auto previt = _id_span_map.find(span->_page_id - 1);
	while (previt != _id_span_map.end())
	{
		Span* prespan = previt->second;
		//说明前一个Span不是空闲的无法合并
		if (prespan->_usecount != 0)
			break;
		//说明合并之后的页过大超过管理的范围
		if (prespan->_page_num + span->_page_num >= NPAGE)
			break;

		//此时说明可以合并
		_pagelist[prespan->_page_num].Earse(prespan);
		prespan->_page_num += span->_page_num;

		delete span;
		span = prespan;

		previt = _id_span_map.find(span->_page_id - 1);
	}

	//然后合并他的后一个
	auto nextit = _id_span_map.find(span->_page_id + span->_page_num);
	while (nextit != _id_span_map.end())
	{
		Span* nextspan = nextit->second;
		if (nextspan->_usecount == 0)
			break;
		if (nextspan->_page_num + span->_page_num >= NPAGE)
			break;

		//可以合并
		_pagelist[nextspan->_page_num].Earse(nextspan);
		span->_page_num += nextspan->_page_num;

		delete nextspan;
		nextit = _id_span_map.find(span->_page_id + span->_page_num);
	}
	//重新完成映射
	for (size_t i = 0; i < span->_page_num; ++i)
	{
		_id_span_map[span->_page_id + i] = span;
	}
	_pagelist[span->_page_num].PushFront(span);
}
