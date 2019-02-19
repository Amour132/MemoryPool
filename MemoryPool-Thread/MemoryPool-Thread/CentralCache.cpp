#include "CentralCache.h"
#include "PageCache.h"

CentralCache CentralCache::_inst;

Span* CentralCache::GetOneSpan(SpanList* spanlist, size_t bytes)
{
	Span* span = spanlist->begin();
	while (span != spanlist->end())
	{
		if (span != nullptr)
			return span;

		span = span->_next;
	}
	
	//��ʱ˵�����Ļ���û�У���Ҫ����һ���PageChache����
	size_t npage = ClassSize::NumMovePage(bytes);
	Span* newspan = PageCache::GetInstance()->NewSpan(npage);

	//�����뵽��Span�и�Ȼ����������
	char* start = (char*)(newspan->_page_id << PAGE_SHIFT);
	char* end = start + (newspan->_page_num << PAGE_SHIFT);
	char* cur = start;
	char* next = end + bytes;
	while (next < end)
	{
		NextObj(cur) = next;
		cur = next;
		next = next + bytes;
	}
	NextObj(cur);
	newspan->_objlist = start;
	newspan->_usecount = 0;
	newspan->_objsize = bytes;

	//��������뵽Spanlist��
	spanlist->PushFront(newspan);
	return newspan;
}

size_t CentralCache::FetchRangeObj(void*& start, void*& end, size_t n, size_t bytes)
{
	//����λ��
	size_t index = ClassSize::Index(bytes);
	SpanList* spanlist = &_spanlist[index];
	
	//��Ϊ���Ļ����ǹ�����Դ����Ҫ����
	std::unique_lock<std::mutex> lock(spanlist->_mtx);

	Span* span = GetOneSpan(spanlist, bytes);
	void* cur = span->_objlist;
	void* pre = cur;
	size_t fetchnum = 0;
	//��Ҫ�����ӵ�ǰspanlistȡ�����������spanlist�����Ͱ��е�ȫ����ȥ
	while (cur != nullptr && fetchnum < n)
	{
		pre = cur;
		cur = NextObj(cur);
		fetchnum++;
	}

	start = span->_objlist;
	end = pre;
	NextObj(end) = nullptr;

	span->_objlist = cur;
	span->_usecount += fetchnum;

	return fetchnum;
}

void CentralCache::ReleaseToSpan(void* start, size_t byte_size)
{
	size_t index = ClassSize::Index(byte_size);
	SpanList* spanlist = &_spanlist[index];

	std::unique_lock<std::mutex> lock(spanlist->_mtx);

	while (start)
	{
		void* next = NextObj(start);
		//��span����ӳ�䣬Ϊ�˺���ĺϲ�
		Span* span = PageCache::GetInstance()->MapObjectToSpan(start);

		if (--span->_usecount == 0)//˵��span����Ķ����ǿ��еĿ��Ի���PageCache���кϲ�
		{
			spanlist->Earse(span);

			span->_objlist = nullptr;
			span->_objsize = 0;
			span->_pre = nullptr;
			span->_next = nullptr;

			PageCache::GetInstance()->ReleaseSpanToPageCahce(span);
		}

		NextObj(start) = span->_objlist;
		span->_objlist = start;

		start = next;
	}
}