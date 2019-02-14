#pragma once

#include <iostream>
#include <assert.h>

const size_t NLIST = 240;  
const size_t MAXBYTES = 64 * 1024 * 1024;

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
		else if (size <=1024)
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
};

