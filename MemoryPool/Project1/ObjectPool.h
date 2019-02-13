#ifndef _OBJECT_POOL_H
#define _OBJECT_POOL_H

#include <iostream>

//定长的对象池
template<class T>
class ObjectPool
{
protected:
	struct Block
	{
		Block(size_t bytesize)
		{
			_start = (char*)malloc(bytesize);
			_bytesize = bytesize;
			_next = nullptr;

		}

		char* _start = nullptr;
		size_t _bytesize = 0;
		size_t _byteleft = 0;//剩余字节数
		Block* _next = nullptr;
	};
public:
	ObjectPool(size_t init_num = 8)
	{
		_head = _tail = new Block(init_num);

	}

	T*& OBJ_NEXT(T* obj)
	{
		return *(T**)obj;
	}

	T* New()
	{
		T* obj = nullptr;
		if (_freelist != nullptr)
		{
			obj = _freelist;
			_freelist = OBJ_NEXT(_freelist);
		}
		else
		{
			if (_tail->_byteleft == 0)
			{
				Block* newblock = new Block(_tail->_byteleft * 2);
				_tail->_next = newblock;
				_tail = newblock;
			}
			obj = (T*)_tail->_start + (_tail->_bytesize - _tail->_byteleft);
			_tail->_byteleft -= sizeof(T);


		}
		return obj;
	}
	

	void Delete(T* ptr)
	{
		if (_freelist == nullptr)
		{
			_freelist = ptr;
			(*(T**)ptr) = nullptr;
		}
		else
		{
			(*(T**)ptr) = _freelist;
			_freelist = ptr;
		}
	}

protected:
	//自由链表
	T* _freelist = nullptr;

	//块管理
	Block* _head = nullptr;
	Block* _tail = nullptr;
};

#endif
void TestObjectPool()
{
	ObjectPool<int> pool;
	int* p1 = pool.New();
	int* p2 = pool.New();

	std::cout << p1 << std::endl;
	std::cout << p2 << std::endl;

	pool.Delete(p1);
	pool.Delete(p2);

	std::cout << pool.New() << std::endl;
	std::cout << pool.New() << std::endl;
}
