#pragma once

#include "Comm.h"
#include "ThreadCache.h"
#include "PageCache.h"


//����ģʽ�����ü����������Ч��
class CentralCache
{
public:
	static CentralCache* GetInstance()
	{
		return &_inst;
	}
	//��ȡһ��span
	Span* GetOneSpan(SpanList* spanlist, size_t bytes);

	// �����Ļ����ȡһ�������Ķ����thread cache
	size_t FetchRangeObj(void* start, void* end, size_t n, size_t bytes);

	//����кܶ���пռ䣬����黹��Span
	void ReleaseToSpan(void* start, size_t byte_size);


private:
	SpanList _spanlist[NLIST];

	CentralCache() = default;
	CentralCache(const CentralCache& c) = delete;
	CentralCache& operator=(const CentralCache& c) = delete;

	static CentralCache _inst;
};