#pragma once

#include "Comm.h"

class ThreadCache
{
public:
	void* Allocate(size_t size);
	void Deallocate(void* ptr, size_t size);

	//从中心缓存获取对象
	void* FetchFromCentralCache(size_t index, size_t bytes);
	//对象太多，回收到中心缓存
	void TooLongRecyle(FreeList* freelist, size_t byte);

private:
	FreeList _freelist[NLISTS];
};

static _declspec(thread) ThreadCache* tls_threadcache = nullptr;
