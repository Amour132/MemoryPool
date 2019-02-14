#pragma once

#include <iostream>
#include "Comm.h"

class ThreadCache
{
public:
	void* Allocate(size_t size);
	void Deallocate(void* ptr,size_t size);

private:
	void* FetchFromCentralCache(size_t index, size_t bytes);

private:
	FreeList _freelist[NLIST];
};