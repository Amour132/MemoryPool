#pragma once

#include <iostream>
#include <assert.h>
#include <thread>
#include <mutex>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#endif // _WIN32

const size_t NLIST = 240;
const size_t MAXBYTES = 64 * 1024 * 1024;
const size_t PAGE_SHIFT = 12;
const size_t NPAGE = 129; //128+1


//�õ�ͷ�ĸ��ֽ�
static inline void*& NextObj(void* ptr)
{
	return *((void**)ptr);
}

//���������������ĳ���
class FreeList
{
public:
	bool Empty()
	{
		return _size == 0;
	}

	void Push(void* obj)
	{
		NextObj(obj) = _list;
		_list = obj;
		_size++;
	}

	void* Pop()
	{
		assert(_list);
		void* obj = _list;
		_list = NextObj(obj);
		_size--;
		return obj;
	}

	void PushRange(void* start, void* end, int num)
	{
		NextObj(end) = _list;
		_list = start;
		_size += num;
	}

	size_t Size()
	{
		return _size;
	}

	size_t MaxSize()
	{
		return _maxsize;
	}
private:
	void* _list = nullptr;
	size_t _size = 0;
	size_t _maxsize = 0;
};

//�����ڴ��������
class ClassSize
{
	//���������̫С���ܲ��������ڴ��������ƣ�����̫��������Դ���˷ѡ��������Ƭ����
	//�������ָ������䣬ʹ�ò�ͬ���ֽڶ��롣
	// ������12%���ҵ�����Ƭ�˷�
	// [1,128]				8byte���� freelist[0,16)
	// [129,1024]			16byte���� freelist[16,72)
	// [1025,8*1024]		128byte���� freelist[72,128)
	// [8*1024+1,64*1024]	512byte���� freelist[128,240)
public:
	static inline size_t _RoundUp(size_t size, size_t align)
	{
		return (size + align - 1)& ~(align - 1);
		//���϶����ֽ�����һʹ�����һ����������������һ�����ٽ��������㣬ʹ�õ�����������
	}

	static inline size_t RoundUp(size_t size)
	{
		assert(size < MAXBYTES);  //���ܳ������ֵ

		//��ʼ����size��Сѡ����뷽ʽ

		if (size <= 128)
		{
			return _RoundUp(size, 8);
		}
		else if (size <= 1024)
		{
			return _RoundUp(size, 16);
		}
		else if (size <= 8192)
		{
			return _RoundUp(size, 128);
		}
		else if (size <= 65536)
		{
			return _RoundUp(size, 512);
		}
	}

	//ȷ���ڴ���λ��
	static inline size_t _Index(size_t bytes, size_t align_shift)
	{
		return ((bytes + (1 << align_shift) - 1) >> align_shift) - 1;
	}

	static inline size_t Index(size_t bytes)
	{
		assert(bytes < MAXBYTES);
		//static int group[4] = { 16, 56, 56, 112 }; //ÿ�����������������

		//���Լ򻯳�ÿ������+֮ǰ���������������
		static int group_arr[4] = { 16, 72, 128, 240 };

		if (bytes <= 128)
		{
			return _Index(bytes, 3);
		}
		else if (bytes <= 1024)
		{                                  //group[0];
			return _Index(bytes - 128, 4) + group_arr[0];
		}
		else if (bytes <= 8192)
		{                                  //group[1] + group[0]
			return _Index(bytes - 1024, 7) + group_arr[1];
		}
		else if (bytes <= 65536)
		{                                  //group[1] + group[0] + group[2]
			return _Index(bytes - 8192, 9) + group_arr[2];
		}
		else
		{
			return -1;
		}
	}

	static size_t NumMoveSize(size_t size)
	{
		if (size == 0)
			return 0;

		int num = static_cast<int>(MAXBYTE / size);
		if (num < 2)
			num = 2;

		if (num > 512)
			num = 512;

		return num;
	}

	//����һ����ϵͳ��ü�ҳ
	static size_t NumMovePage(size_t size)
	{
		size_t num = NumMoveSize(size);
		size_t npage = size*num;

		npage <<= 12;
		if (npage == 0)//�������Ĳ���һҳ�Ǿ͸�һҳ
			npage = 1;

		return npage;
	}
};



typedef size_t PageId;
struct Span
{
	PageId _page_id; //ҳ��
	size_t _page_num; //ҳ������

	void* _objlist = nullptr; //�������������������
	size_t _objsize = 0; //�����С
	size_t _usecount = 0;  //��������ʹ�ø���

	Span* _next = nullptr;
	Span* _pre = nullptr;
};

class SpanList
{
public:
	SpanList()
	{
		_head = new Span;
		_head->_next = _head;
		_head->_pre = _head;
	}

	Span* begin()
	{
		return _head->_next;
	}

	Span* end()
	{
		return _head;
	}

	bool Empty()
	{
		return _head->_next = _head;
	}

	void Insert(Span* pos, Span *newspan)
	{
		assert(pos);
		Span* pre = pos->_pre;

		pre->_next = newspan;
		newspan->_pre = pre;

		newspan->_next = pos;
		pos->_pre = newspan;
	}

	void Earse(Span* pos)
	{
		assert(pos && pos != _head);
		Span* pre = pos->_pre;
		Span* next = pos->_next;

		pre->_next = next;
		next->_pre = pre;
	}

	void PushFront(Span* newspan)
	{
		assert(newspan);
		Insert(begin(), newspan);
	}

	Span* PopFront()
	{
		Span* cur = begin();
		Earse(cur);
		return cur;
	}

private:
	Span* _head = nullptr;
};