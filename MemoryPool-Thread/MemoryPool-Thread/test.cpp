#include "ConcurrentAlloc.h"
#include "Comm.h"

void TestConcurrentAlloc()
{
	size_t n = 100000;
	std::vector<void*> v;
	for (size_t i = 0; i < n; ++i)
	{
		v.push_back(ConcurrentAlloc(10));
		std::cout << v.back() << std::endl;
	}
	std::cout << std::endl << std::endl;

	for (size_t i = 0; i < n; ++i)
	{
		ConcurrentFree(v[i], 10);
	}
	v.clear();

	for (size_t i = 0; i < n; ++i)
	{
		v.push_back(ConcurrentAlloc(10));
		std::cout << v.back() << std::endl;
	}

	for (size_t i = 0; i < n; ++i)
	{
		ConcurrentFree(v[i], 10);
	}
	v.clear();
}



int main()
{
	TestConcurrentAlloc();
	return 0;
}