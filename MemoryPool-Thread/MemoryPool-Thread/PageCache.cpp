#include "PageCache.h"

PageCache PageCache::_inst;


Span* PageCache::NewSpan(size_t npage)
{
	std::unique_lock<std::mutex> lock(_mtx);
	if (npage >= NPAGES)
	{
		void* ptr = SystemAlloc(npage);
		Span* span = new Span;
		span->_page_id = (PageId)ptr >> PAGE_SHIFT;
		span->_page_num = npage;
		span->_objsize = npage << PAGE_SHIFT;
		_id_span_map[span->_page_id] = span;

	}
	Span* span = _NewSpan(npage);
	span->_objsize = npage << PAGE_SHIFT;
	return span;
}



Span* PageCache::_NewSpan(size_t npage)
{
	if (!_pagelist[npage].Empty())
	{
		return _pagelist[npage].PopFront();
	}

	for (size_t i = npage + 1; i < NPAGES; ++i)
	{
		SpanList* pagelist = &_pagelist[i];
		if (!pagelist->Empty())
		{
			Span* span = pagelist->PopFront();
			Span* split = new Span;
			split->_page_id = span->_page_id + span->_page_num - npage;
			split->_page_num = npage;
			span->_page_num -= npage;
			_pagelist[span->_page_num].PushFront(span);

			for (size_t i = 0; i < split->_page_num; ++i)
			{
				_id_span_map[split->_page_id + i] = split;
			}
			return split;
		}
	}

	// 需要向系统申请内存
	void* ptr = SystemAlloc(NPAGES - 1);
	if (ptr == nullptr)
	{
		throw std::bad_alloc();
	}

	Span* largespan = new Span;
	largespan->_page_id = (PageId)ptr >> PAGE_SHIFT;
	largespan->_page_num = NPAGES - 1;
	_pagelist[NPAGES - 1].PushFront(largespan);

	for (size_t i = 0; i < largespan->_page_num; ++i)
	{
		_id_span_map[largespan->_page_id + i] = largespan;
	}
	for (size_t i = 0; i < largespan->_page_num; ++i)
	{
		_id_span_map[largespan->_page_id + i] = largespan;
	}
	//走到这里说明PageCache已经有资源可以申请了所以再调用一次NewSpan
	return _NewSpan(npage);
}

// 获取从对象到span的映射
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
	std::unique_lock<std::mutex> lock(_mtx);
	if (span->_page_num >= NPAGES) //如果是ThreadCache还回来的大内存
	{
		void* ptr = (void*)(span->_page_id << PAGE_SHIFT);
		VirtualFree(ptr, 0, MEM_RELEASE);
		_id_span_map.erase(span->_page_id);
		delete span;
		return;
	}

	auto previt = _id_span_map.find(span->_page_id - 1);
	while (previt != _id_span_map.end())
	{
		Span* prevspan = previt->second;
		// 不是空闲，则直接跳出
		if (prevspan->_usecount != 0)
			break;

		// 如果合并出超过NPAGES页的span，则不合并，否则没办法管理
		if (prevspan->_page_num + span->_page_num >= NPAGES)
			break;

		_pagelist[prevspan->_page_num].Erase(prevspan);
		prevspan->_page_num += span->_page_num;
		delete span;
		span = prevspan;

		previt = _id_span_map.find(span->_page_id - 1);
	}

	auto nextit = _id_span_map.find(span->_page_id + span->_page_num);
	while (nextit != _id_span_map.end())
	{
		Span* nextspan = nextit->second;
		if (nextspan->_usecount != 0)
			break;

		if (span->_page_num + nextspan->_page_num >= NPAGES)
			break;

		_pagelist[nextspan->_page_num].Erase(nextspan);
		span->_page_num += nextspan->_page_num;
		delete nextspan;

		nextit = _id_span_map.find(span->_page_id + span->_page_num);
	}

	for (size_t i = 0; i < span->_page_num; ++i)
	{
		_id_span_map[span->_page_id + i] = span;
	}
	_pagelist[span->_page_num].PushFront(span);
}