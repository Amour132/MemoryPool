#pragma once
#include "Comm.h"
#include "ThreadCache.h"
#include "PageCache.h"

void* ConcurrentAlloc(size_t size)
{
	if (size > MAXBYTES)//�������Ķ������ֱ��ȥPageCache����
	{
		size_t roundsize = ClassSize::_Roundup(size, 1 << 12);
		size_t npage = roundsize >> PAGE_SHIFT;

		Span* span = PageCache::GetInstance()->NewSpan(npage);
		void* ptr = (void*)(span->_page_id << 12);
		return ptr;
	}
	else
	{
		// ͨ��tls����ȡ�߳��Լ���threadcache
		if (tls_threadcache == nullptr)
		{
			tls_threadcache = new ThreadCache;
			//cout << std::this_thread::get_id() << "->" << tls_threadcache << endl;
		}

		return tls_threadcache->Allocate(size);
	}
}

void ConcurrentFree(void* ptr)
{
	//ͨ��ӳ���ϵ�õ���Ӧ��Span,Ȼ��õ���С
	Span* span = PageCache::GetInstance()->MapObjectToSpan(ptr);
	size_t size = span->_objsize;
	if (size > MAXBYTES)
	{
		PageCache::GetInstance()->ReleaseSpanToPageCahce(span);
	}
	else
	{
		tls_threadcache->Deallocate(ptr, size);
	}
}