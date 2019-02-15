#include "Comm.h"
#include "CentralCache.h"

void* ThreadCache::FetchFromCentralCache(size_t index, size_t bytes)
{
	FreeList& freelist = _freelist[index];
	size_t num = 10; //һ��������ȡʮ��

	void* start = nullptr;
	void* end = nullptr;
	size_t fetchnum = CentralCache::GetInstance()->FetchRangeObj(start, end, num, bytes);

	if (fetchnum == 1) //�����Ļ��治��
		return start;

	freelist.PushRange(NextObj(start), end, fetchnum - 1);
	return start;
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
		return FetchFromCentralCache(index, size);
	}
}

void ThreadCache::Deallocate(void* ptr, size_t size)
{
	assert(size < MAXBYTES);
	size_t index = ClassSize::Index(size);
	FreeList& freelist = _freelist[index];
	freelist.Push(ptr);
}