#pragma once

#include "Comm.h"

class PageCache
{
public:
	static PageCache* GetInstance()
	{
		return &_inst;
	}

	Span* NewSpan(size_t npage);
	//��ȡӳ�� �����Span��ӳ��
	Span* MapObjectToSpan(void* obj);

	// �ͷſ���span�ص�Pagecache�����ϲ����ڵ�span
	void ReleaseSpanToPageCahce(Span* span);

private:
	SpanList _pagelist[NPAGE];

	PageCache() = default;
	PageCache(const PageCache& p) = delete;
	PageCache& operator=(const PageCache& p) = delete;

	static PageCache _inst;
};
