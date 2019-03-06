#pragma once

#include "Comm.h"

class PageCache
{
public:
	static PageCache* GetInstance()
	{
		return &_inst;
	}

	Span* _NewSpan(size_t npage);
	Span* NewSpan(size_t npage);

	// 获取从对象到span的映射
	Span* MapObjectToSpan(void* obj);
	// 释放空闲span回到Pagecache，并合并相邻的span
	void ReleaseSpanToPageCahce(Span* span);

private:
	SpanList _pagelist[NPAGES];
private:
	PageCache() = default;
	PageCache(const PageCache&) = delete;
	static PageCache _inst;

	std::mutex _mtx;
	std::map<PageId, Span*> _id_span_map;
};
