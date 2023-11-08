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

typedef __malloc_alloc_template<0> malloc_alloc;

// 二级空间分配器
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

	// 自由链表数组，管理对应大小的内存
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
		nobjs = bytes_left / n;	// 剩下的内存够几块
		total_bytes = n * nobjs; // 重新计算所需要的字节数
		result = start_free;
		start_free += total_bytes;
		return result;
	}
	else
	{
		size_t bytes_to_get = 2 * total_bytes + ROUND_UP(bytes_left);	// 一次性多申请一些

		// 如果还有剩余空间，但是大小不满足本次申请大小，则需要处理这部分空间，以免丢失内存
		// 例如：申请100字节，现在剩余56字节（__ALIGN的倍数 ps:这里设计的真好）
		// 做法是，把这个56字节，插入到56字节对应的自由链表头部，让这个部分空间归属于56字节自由链表
		if (bytes_left > 0)
		{
			obj** my_free_list = free_list + FREELIST_INDEX(bytes_left); // 计算剩余内存该属于哪个自由链表
			// 完成一个头部插上动作
			((obj*)start_free)->free_list_link = *my_free_list;			 // 现在的空余内存起始指针的next指向，my_free_list自由链表的值
			*my_free_list = (obj *)start_free;									 // 再把所属只有链表指向之前的起始指针
		}

		// 根据本次申请大小 去向系统申请内存
		start_free = (char *)malloc(bytes_to_get);
		if (start_free == 0)
		{
			// 由于现在bytes_to_get是扩展了需求去做的申请，比如现在需要80字节，扩展后就是2 * (80 * 20) + 16 = 3216字节
			// 现在malloc申请不到3216字节的数据，那看是不是能满足有100字节的基本需求，这个80字节可能在其他的自由链表中还存在有空余的，可以拿出来救急。
			obj **my_free_list, *p;
			for (int i = n; i <= __MAX_BYTES; i += __ALIGN)
			{
				my_free_list = free_list + FREELIST_INDEX(i);	// 按自由链表所对应的大小，依次查询可用空间
				p = *my_free_list;
				if (0 != p)	// 某个自由链表有剩余空间，例如查询到inex=12 block=104 所属的自由链表还有剩余空间，那么久把这个104的空间分配出去
				{
					*my_free_list = p->free_list_link; // 更新这个自由链表节点，指向下一个块
					start_free = (char*)p;
					end_free = start_free + i;
					return chunk_alloc(n, nobjs);  // 这里再次调用就有内存申请出去了
				}
			}

			// 如果当前所有的自由链表里面都没有空余空间了，就去一级空间分配器中去申请内存
			end_free = 0;
			start_free = (char *)malloc_alloc::allocate(bytes_to_get);	// 一级空间分配器，如果申请不到会调用set_new_handler之类的操作去释放内存，或者直接抛出异常结束运行
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

	my_free_list = free_list + FREELIST_INDEX(n);	// 根据大小n来确认用自由链表数组里面的哪个
	*my_free_list = next_obj = (obj*)(chunk + n);	// 将只有链表的头节点指向第二块
	
	// 剩余的部分组织形成自由链表
	for (int i = 1; ; ++i)
	{
		cur_obj = next_obj;
		next_obj = (obj*)((char*)next_obj + n);
		
		if (nobjs - 1 == i)	// 最后一块
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
	if (n > __MAX_BYTES)	// 大于二级空间分配器的最大值，就用一级空间分配器
		return malloc_alloc::allocate(n);
	
	obj** my_free_list;
	obj* result;
	my_free_list = free_list + FREELIST_INDEX(n);
	result = *my_free_list;
	if (result == 0)	// 自由链表里面还没与分配空间
	{
		void* r = refill(ROUND_UP(n));
		return r;
	}

	*my_free_list = result->free_list_link; // 修改自由链表，执行未被使用的下一个节点
	return result;
}


