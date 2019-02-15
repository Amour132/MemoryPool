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


//拿到头四个字节
static inline void*& NextObj(void* ptr)
{
	return *((void**)ptr);
}

//管理对象自由链表的长度
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

//管理内存对齐问题
class ClassSize
{
	//如果对齐数太小可能不能体现内存对齐的优势，但是太大会造成资源的浪费。造成内碎片问题
	//所以区分各个区间，使用不同的字节对齐。
	// 控制在12%左右的内碎片浪费
	// [1,128]				8byte对齐 freelist[0,16)
	// [129,1024]			16byte对齐 freelist[16,72)
	// [1025,8*1024]		128byte对齐 freelist[72,128)
	// [8*1024+1,64*1024]	512byte对齐 freelist[128,240)
public:
	static inline size_t _RoundUp(size_t size, size_t align)
	{
		return (size + align - 1)& ~(align - 1);
		//加上对齐字节数减一使得如果一旦有余数则跳到下一个，再将余数清零，使得到达整数倍处
	}

	static inline size_t RoundUp(size_t size)
	{
		assert(size < MAXBYTES);  //不能超过最大值

		//开始根据size大小选择对齐方式

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

	//确定内存块得位置
	static inline size_t _Index(size_t bytes, size_t align_shift)
	{
		return ((bytes + (1 << align_shift) - 1) >> align_shift) - 1;
	}

	static inline size_t Index(size_t bytes)
	{
		assert(bytes < MAXBYTES);
		//static int group[4] = { 16, 56, 56, 112 }; //每个区间的自由链表数

		//可以简化成每个区间+之前区间的所有链表数
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

	//计算一次像系统获得几页
	static size_t NumMovePage(size_t size)
	{
		size_t num = NumMoveSize(size);
		size_t npage = size*num;

		npage <<= 12;
		if (npage == 0)//如果申请的不过一页那就给一页
			npage = 1;

		return npage;
	}
};



typedef size_t PageId;
struct Span
{
	PageId _page_id; //页号
	size_t _page_num; //页的数量

	void* _objlist = nullptr; //用来管理对象自由链表
	size_t _objsize = 0; //对象大小
	size_t _usecount = 0;  //计数，已使用个数

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