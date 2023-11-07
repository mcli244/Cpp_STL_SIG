#pragma once
// 一级空间适配器
#if 0
#   include <new>
#   define __THROW_BAD_ALLOC throw bad_alloc
#elif !defined(__THROW_BAD_ALLOC)
#   include<iostream>
#   define __THROW_BAD_ALLOC std::cerr << "out of memory" << std::endl; exit(1)
#endif
using namespace std;

template <int inst>
class __malloc_alloc_template
{
public:
	static void* allocate(size_t n)
	{
		void* rp = malloc(n);

		if (NULL == rp)
			return oom_allocate(n);	// 尝试调用oom_allocate做申请失败的补救措施

		return rp;
	}
	
	static void deallocate(void* p, size_t n)
	{
		free(p);
	}
	
	static void* reallocate(void* p, size_t old_n, size_t new_n)
	{
		void *rp = realloc(p, new_n);

		if (NULL == rp)
			return oom_reallocate(p, new_n);// 尝试调用申请失败的补救措施

		return rp;
	}
	
	static void* set_malloc_handler(void(*f)())
	{
		void (*old)() = __malloc_alloc_oomm_handler;	// TODO:没看懂为啥要返回之前的值
		__malloc_alloc_oomm_handler = f;
		return old;
	}
	
private:
	static void* oom_allocate(size_t n);
	static void* oom_reallocate(void* p, size_t new_n);
	static void (*__malloc_alloc_oomm_handler)(void);
};

template <int inst>
void (*__malloc_alloc_template<inst>::__malloc_alloc_oomm_handler)() = nullptr;


template <int inst>
void * __malloc_alloc_template<inst>::oom_allocate(size_t n)
{
	void* rp = NULL;
	for (;;)
	{
		if (__malloc_alloc_oomm_handler == NULL)
		{
			__THROW_BAD_ALLOC;
		}	// 抛出异常
		(*__malloc_alloc_oomm_handler)();	// 执行客户设置进来的回调函数，期望改函数内能执行内存回收机制，让后面再次尝试malloc有机会成功
		rp = malloc(n);
		if (rp) return rp;
	}
	
	return rp;
}

template <int inst>
void* __malloc_alloc_template<inst>::oom_reallocate(void* p, size_t new_n)
{
	void* rp = NULL;
	for (;;)
	{
		if (__malloc_alloc_oomm_handler == NULL)
		{
			__THROW_BAD_ALLOC;
		}	// 抛出异常
		(*__malloc_alloc_oomm_handler)();	// 执行客户设置进来的回调函数，期望改函数内能执行内存回收机制，让后面再次尝试malloc有机会成功
		rp = realloc(p, new_n);
		if (rp) return rp;
	}
	return rp;
}


