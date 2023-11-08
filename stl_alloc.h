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

typedef __malloc_alloc_template<0> malloc_alloc;

// �����ռ������
enum {__ALIGN = 8};
enum {__MAX_BYTES = 128};
enum {__NFREELISTS = __MAX_BYTES / __ALIGN};

template<bool thread, int inst>
class __default_alloc_template 
{
public:
	static void* allocate(size_t n);
	static void deallocate(void* p, size_t n);
	static void* reallocate(void *p, size_t old_sz, size_t new_sz);

private:
	union obj
	{
		union obj* free_list_link;
		char client_data[1];
	};
	
	static size_t ROUND_UP(size_t bytes)
	{
		return (bytes + __ALIGN - 1) & ~(__ALIGN - 1);
	}

	static size_t FREELIST_INDEX(size_t bytes)
	{
		return (bytes + __ALIGN - 1) / __ALIGN - 1;
	}

	static void* refill(size_t n);
	static char* chunk_alloc(size_t n, int& nobjs);

	// �����������飬�����Ӧ��С���ڴ�
	// index 0 1   2  3  4  5  6  7  8  9 10 11 12   13  14  15
	// block 8 16 24 32 40 48 56 64 72 80 88 96 104 112 120 128
	static obj* free_list[__NFREELISTS];
	static char* start_free;
	static char* end_free;
	static size_t heap_size;
};


template<bool thread, int inst>
char * __default_alloc_template<thread, inst>::start_free = 0;

template<bool thread, int inst>
char* __default_alloc_template<thread, inst>::end_free = 0;

template<bool thread, int inst>
size_t __default_alloc_template<thread, inst>::heap_size = 0;

template<bool thread, int inst>
typename __default_alloc_template<thread, inst>::obj *
__default_alloc_template<thread, inst>::free_list[__NFREELISTS] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

template<bool thread, int inst>
char* __default_alloc_template<thread, inst>::chunk_alloc(size_t n, int &nobjs)
{
	char* result;
	size_t total_bytes = n * nobjs;
	size_t bytes_left = end_free - start_free;

	if (bytes_left >= total_bytes)
	{
		result = start_free;
		start_free += total_bytes;
		return result;
	}
	else if (bytes_left >= n)
	{
		nobjs = bytes_left / n;	// ʣ�µ��ڴ湻����
		total_bytes = n * nobjs; // ���¼�������Ҫ���ֽ���
		result = start_free;
		start_free += total_bytes;
		return result;
	}
	else
	{
		size_t bytes_to_get = 2 * total_bytes + ROUND_UP(bytes_left);	// һ���Զ�����һЩ

		// �������ʣ��ռ䣬���Ǵ�С�����㱾�������С������Ҫ�����ⲿ�ֿռ䣬���ⶪʧ�ڴ�
		// ���磺����100�ֽڣ�����ʣ��56�ֽڣ�__ALIGN�ı��� ps:������Ƶ���ã�
		// �����ǣ������56�ֽڣ����뵽56�ֽڶ�Ӧ����������ͷ������������ֿռ������56�ֽ���������
		if (bytes_left > 0)
		{
			obj** my_free_list = free_list + FREELIST_INDEX(bytes_left); // ����ʣ���ڴ�������ĸ���������
			// ���һ��ͷ�����϶���
			((obj*)start_free)->free_list_link = *my_free_list;			 // ���ڵĿ����ڴ���ʼָ���nextָ��my_free_list���������ֵ
			*my_free_list = (obj *)start_free;									 // �ٰ�����ֻ������ָ��֮ǰ����ʼָ��
		}

		// ���ݱ��������С ȥ��ϵͳ�����ڴ�
		start_free = (char *)malloc(bytes_to_get);
		if (start_free == 0)
		{
			// ��������bytes_to_get����չ������ȥ�������룬����������Ҫ80�ֽڣ���չ�����2 * (80 * 20) + 16 = 3216�ֽ�
			// ����malloc���벻��3216�ֽڵ����ݣ��ǿ��ǲ�����������100�ֽڵĻ����������80�ֽڿ��������������������л������п���ģ������ó����ȼ���
			obj **my_free_list, *p;
			for (int i = n; i <= __MAX_BYTES; i += __ALIGN)
			{
				my_free_list = free_list + FREELIST_INDEX(i);	// ��������������Ӧ�Ĵ�С�����β�ѯ���ÿռ�
				p = *my_free_list;
				if (0 != p)	// ĳ������������ʣ��ռ䣬�����ѯ��inex=12 block=104 ����������������ʣ��ռ䣬��ô�ð����104�Ŀռ�����ȥ
				{
					*my_free_list = p->free_list_link; // ���������������ڵ㣬ָ����һ����
					start_free = (char*)p;
					end_free = start_free + i;
					return chunk_alloc(n, nobjs);  // �����ٴε��þ����ڴ������ȥ��
				}
			}

			// �����ǰ���е������������涼û�п���ռ��ˣ���ȥһ���ռ��������ȥ�����ڴ�
			end_free = 0;
			start_free = (char *)malloc_alloc::allocate(bytes_to_get);	// һ���ռ��������������벻�������set_new_handler֮��Ĳ���ȥ�ͷ��ڴ棬����ֱ���׳��쳣��������
		}
		end_free = start_free + bytes_to_get;
		heap_size += bytes_to_get;

		return chunk_alloc(n, nobjs);
	}
}

template<bool thread, int inst>
void* __default_alloc_template<thread, inst>::refill(size_t n)
{
	int nobjs = 20;
	char *chunk = chunk_alloc(n, nobjs);

	if (nobjs == 1)
		return chunk;

	obj** my_free_list;
	obj* cur_obj, * next_obj;

	my_free_list = free_list + FREELIST_INDEX(n);	// ���ݴ�Сn��ȷ����������������������ĸ�
	*my_free_list = next_obj = (obj*)(chunk + n);	// ��ֻ�������ͷ�ڵ�ָ��ڶ���
	
	// ʣ��Ĳ�����֯�γ���������
	for (int i = 1; ; ++i)
	{
		cur_obj = next_obj;
		next_obj = (obj*)((char*)next_obj + n);
		
		if (nobjs - 1 == i)	// ���һ��
		{
			cur_obj->free_list_link = 0;
			break;
		}	
		else
			cur_obj->free_list_link = next_obj;
	}

	return chunk;
}

template<bool thread, int inst>
void* __default_alloc_template<thread, inst>::allocate(size_t n)
{
	if (n > __MAX_BYTES)	// ���ڶ����ռ�����������ֵ������һ���ռ������
		return malloc_alloc::allocate(n);
	
	obj** my_free_list;
	obj* result;
	my_free_list = free_list + FREELIST_INDEX(n);
	result = *my_free_list;
	if (result == 0)	// �����������滹û�����ռ�
	{
		void* r = refill(ROUND_UP(n));
		return r;
	}

	*my_free_list = result->free_list_link; // �޸���������ִ��δ��ʹ�õ���һ���ڵ�
	return result;
}


