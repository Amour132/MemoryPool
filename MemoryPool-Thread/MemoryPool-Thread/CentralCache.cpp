#include "CentralCache.h"
#include "PageCache.h"

CentralCache CentralCache::_inst;

Span* CentralCache::GetOneSpan(SpanList* spanlist, size_t bytes)
{
	Span* span = spanlist->begin();
	while (span != spanlist->end())
	{
		if (span->_objlist != nullptr)
			return span;

		span = span->_next;
	}

	// 向pagecache申请一个新的合适大小的span
	size_t npage = ClassSize::NumMovePage(bytes);
	Span* newspan = PageCache::GetInstance()->NewSpan(npage);

	// 将span的内存切割成对象挂起来
	char* start = (char*)(newspan->_page_id << PAGE_SHIFT);
	char* end = start + (newspan->_page_num << PAGE_SHIFT);
	char* cur = start;
	char* next = cur + bytes;
	while (next < end)
	{
		NextObj(cur) = next;
		cur = next;
		next = cur + bytes;
	}
	NextObj(cur) = nullptr;
	newspan->_objlist = start;
	newspan->_objsize = bytes;
	newspan->_usecount = 0;

	// 将newspan插入到spanlist
	spanlist->PushFront(newspan);
	return newspan;
}

// 从中心缓存获取一定数量的对象给thread cache
size_t CentralCache::FetchRangeObj(void*& start, void*& end, size_t num, size_t bytes)
{
	size_t index = ClassSize::Index(bytes);
	SpanList* spanlist = &_spanlist[index];

	// 对当前桶进行加锁
	std::unique_lock<std::mutex> lock(spanlist->_mtx);

	Span* span = GetOneSpan(spanlist, bytes);
	void* cur = span->_objlist;
	void* prev = cur;
	size_t fetchnum = 0;
	while (cur != nullptr && fetchnum < num)
	{
		prev = cur;
		cur = NextObj(cur);
		++fetchnum;
	}

	start = span->_objlist;
	end = prev;
	NextObj(end) = nullptr;

	span->_objlist = cur;
	span->_usecount += fetchnum;

	return fetchnum;
}

void CentralCache::ReleaseListToSpans(void* start, size_t byte)
{
	size_t index = ClassSize::Index(byte);
	SpanList* spanlist = &_spanlist[index];
	std::unique_lock<std::mutex> lock(spanlist->_mtx);

	while (start)
	{
		void* next = NextObj(start);
		Span* span = PageCache::GetInstance()->MapObjectToSpan(start);

		NextObj(start) = span->_objlist;
		span->_objlist = start;

		//当释放回到空的span，把空的span转移到头上
		if (span->_objlist == nullptr)
		{
			spanlist->Erase(span);

		}

		// usecount == 0表示span切出去的对象都还回来了
		// 释放span回到pagecache进行合并
		if (--span->_usecount == 0)
		{
			spanlist->Erase(span);

			span->_objlist = nullptr;
			span->_objsize = 0;
			span->_prev = nullptr;
			span->_next = nullptr;

			PageCache::GetInstance()->ReleaseSpanToPageCahce(span);
		}

		start = next;
	}
}

