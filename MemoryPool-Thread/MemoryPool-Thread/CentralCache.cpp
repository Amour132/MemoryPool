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
	
	//此时说明中心缓存没有，需要向下一层的PageChache申请
	size_t npage = ClassSize::NumMovePage(bytes);
	Span* newspan = PageCache::GetInstance()->NewSpan(npage);

	//将申请到的Span切割然后链接起来
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

	//最后将他插入到Spanlist中
	spanlist->PushFront(newspan);
	return newspan;
}

size_t CentralCache::FetchRangeObj(void*& start, void*& end, size_t n, size_t bytes)
{
	//计算位置
	size_t index = ClassSize::Index(bytes);
	SpanList* spanlist = &_spanlist[index];
	
	//因为中心缓存是公共资源所以要加锁
	std::unique_lock<std::mutex> lock(spanlist->_mtx);

	Span* span = GetOneSpan(spanlist, bytes);
	void* cur = span->_objlist;
	void* pre = cur;
	size_t fetchnum = 0;
	//需要几个从当前spanlist取出几个，如果spanlist不够就把有的全给出去
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
		//将span进行映射，为了后面的合并
		Span* span = PageCache::GetInstance()->MapObjectToSpan(start);

		if (--span->_usecount == 0)//说明span里面的对象都是空闲的可以还给PageCache进行合并
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