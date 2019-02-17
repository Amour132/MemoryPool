#pragma once

#include <iostream>
#include "Comm.h"

class ThreadCache
{
public:
	void* Allocate(size_t size);
	void Deallocate(void* ptr, size_t size);
 
	//�����Ļ����ȡ����
	void* FetchFromCentralCache(size_t index, size_t bytes);
	//����̫�࣬���յ����Ļ���
	void TooLongRecyle(FreeList* freelist, size_t byte);

private:
	FreeList _freelist[NLIST];
};

static _declspec(thread) ThreadCache* tls_threadcache = nullptr;