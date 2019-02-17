#include "Comm.h"
#include "CentralCache.h"

void* ThreadCache::FetchFromCentralCache(size_t index, size_t bytes)
{
	FreeList& freelist = _freelist[index];
	size_t num = 10; //一次批量获取十块

	void* start = nullptr;
	void* end = nullptr;
	size_t fetchnum = CentralCache::GetInstance()->FetchRangeObj(start, end, num, bytes);

	if (fetchnum == 1) //当中心缓存不够
		return start;

	freelist.PushRange(NextObj(start), end, fetchnum - 1);
	return start;
}

void* ThreadCache::Allocate(size_t size)
{
	assert(size < MAXBYTES);
	//首先先进行对齐
	size = ClassSize::RoundUp(size);
	//求出位置
	size_t index = ClassSize::Index(size);
	FreeList& freelist = _freelist[index];

	if (!freelist.Empty())//当自由链表还有内存时
	{
		return freelist.Pop();
	}
	//自由链表没有内存，要到CentralCache去获取
	else
	{
		return FetchFromCentralCache(index, size);
	}
}

void TooLongRecyle(FreeList* freelist, size_t byte)
{
	void* start = freelist->Clear();
	CentralCache::GetInstance()->ReleaseToSpan(start, byte);
}

void ThreadCache::Deallocate(void* ptr, size_t byte)
{
	assert(byte < MAXBYTES);
	size_t index = ClassSize::Index(byte);
	FreeList& freelist = _freelist[index];
	freelist.Push(ptr);
	
	//当自由链表的长度大于一次可申请的最大长度是开始进行回收
	if (freelist.Size() >= freelist.MaxSize())
	{
		TooLongRecyle(&freelist, byte);
	}
}