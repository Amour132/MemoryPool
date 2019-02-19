#pragma once

#include "ThreadCache.h"
#include "Comm.h"

void* ConcurrentAlloc(size_t size)
{
	if (size > MAXBYTES)
	{
		return malloc(size);
	}
	else
	{
		if (tls_threadcache == nullptr)
		{
			tls_threadcache = new ThreadCache;
		}
		return tls_threadcache->Allocate(size);
	}
}

void ConcurrentFree(void* ptr, size_t size)
{
	if (size > MAXBYTES)
	{
		return free(ptr);
	}
	else
	{
		tls_threadcache->Deallocate(ptr,size);
	}
}