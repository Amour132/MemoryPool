#include "Comm.h"
#include "ThreadCache.h"

void* ThreadCache::FetchFromCentralCache(size_t index, size_t bytes)
{
	FreeList& freelist = _freelist[index];

}

void* ThreadCache::Allocate(size_t size)
{
	assert(size < MAXBYTES);
	//�����Ƚ��ж���
	size = ClassSize::RoundUp(size);
	//���λ��
	size_t index = ClassSize::Index(size);
	FreeList& freelist = _freelist[index];

	if (!freelist.Empty())//�������������ڴ�ʱ
	{
		return freelist.Pop();
	}
	//��������û���ڴ棬Ҫ��CentralCacheȥ��ȡ
	else
	{
		return FetchFromCentralCache(size, index);
	}
}

void ThreadCache::Deallocate(void* ptr, size_t size)
{
	assert(size < MAXBYTES);
	size_t index = ClassSize::Index(size);
	FreeList& freelist = _freelist[index];
	freelist.Push(ptr);
}