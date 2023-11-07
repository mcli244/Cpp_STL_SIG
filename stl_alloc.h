#pragma once
// һ���ռ�������
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
			return oom_allocate(n);	// ���Ե���oom_allocate������ʧ�ܵĲ��ȴ�ʩ

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
			return oom_reallocate(p, new_n);// ���Ե�������ʧ�ܵĲ��ȴ�ʩ

		return rp;
	}
	
	static void* set_malloc_handler(void(*f)())
	{
		void (*old)() = __malloc_alloc_oomm_handler;	// TODO:û����ΪɶҪ����֮ǰ��ֵ
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
		}	// �׳��쳣
		(*__malloc_alloc_oomm_handler)();	// ִ�пͻ����ý����Ļص������������ĺ�������ִ���ڴ���ջ��ƣ��ú����ٴγ���malloc�л���ɹ�
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
		}	// �׳��쳣
		(*__malloc_alloc_oomm_handler)();	// ִ�пͻ����ý����Ļص������������ĺ�������ִ���ڴ���ջ��ƣ��ú����ٴγ���malloc�л���ɹ�
		rp = realloc(p, new_n);
		if (rp) return rp;
	}
	return rp;
}


