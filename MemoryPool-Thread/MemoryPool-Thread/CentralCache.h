#pragma once

#include "Comm.h"
#include "ThreadCache.h"
#include "PageCache.h"


//单例模式，不用加锁，提高了效率
class CentralCache
{
public:
	static CentralCache* GetInstance()
	{
		return &_inst;
	}
	//获取一个span
	Span* GetOneSpan(SpanList* spanlist, size_t bytes);

	// 从中心缓存获取一定数量的对象给thread cache
	size_t FetchRangeObj(void* start, void* end, size_t n, size_t bytes);

	//如果有很多空闲空间，则将其归还到Span
	void ReleaseToSpan(void* start, size_t byte_size);


private:
	SpanList _spanlist[NLIST];

	CentralCache() = default;
	CentralCache(const CentralCache& c) = delete;
	CentralCache& operator=(const CentralCache& c) = delete;

	static CentralCache _inst;
};