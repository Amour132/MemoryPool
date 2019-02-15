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
private:
	SpanList _pagelist[NPAGE];

	PageCache() = default;
	PageCache(const PageCache& p) = delete;
	PageCache& operator=(const PageCache& p) = delete;

	static PageCache _inst;
};
