#pragma once

#include "Comm.h"

class PageCache
{
public:
	static PageCache* GetInstance()
	{
		return &_inst;
	}

	Span* NewSpan(size_t npage);
	//获取映射 对象和Span的映射
	Span* MapObjectToSpan(void* obj);

	// 释放空闲span回到Pagecache，并合并相邻的span
	void ReleaseSpanToPageCahce(Span* span);

private:
	SpanList _pagelist[NPAGE];

	PageCache() = default;
	PageCache(const PageCache& p) = delete;
	PageCache& operator=(const PageCache& p) = delete;

	static PageCache _inst;
};
