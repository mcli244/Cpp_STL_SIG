#include <iostream>
#include "stl_alloc.h"

using namespace std;

void OutOfMem(void)
{
    cout << "Out of memery\n";
}

// 二级空间配置器
#define __ALIGN 8

size_t ROUND_UP(size_t bytes)
{
    return (bytes + __ALIGN - 1) & ~(__ALIGN - 1);
}

size_t FREELIST_INDEX(size_t bytes)
{
    return (bytes + __ALIGN - 1) / __ALIGN - 1;
}


struct Student
{
    char name[20];
    int age;
};

// 这个联合体的两个成员free_list_link和client_data公用一个空间
// 以最大的free_list_link 指针，占用4字节（32位系统）
union obj 
{
    obj* free_list_link;
    char client_data[1];
};


obj* CreatFreeList(int n)
{
    char* chunk = (char *)malloc(sizeof(struct Student) * n);   // 这里的char*类型 主要是为了把内存按字节计算
    obj* cur_obj, *next_obj;
    obj* list_obj;
    
    list_obj = next_obj = (obj *)chunk;
    for (int i = 0; i < n; ++i)
    {
        cur_obj = next_obj;
        next_obj = (obj *)((char *)next_obj + sizeof(struct Student));

        if(i+1 == n) 
            cur_obj->free_list_link = NULL;
        else
            cur_obj->free_list_link = next_obj;
    }

    return list_obj;
}


int main()
{
    cout << "Hello World!123abc\n";

    int* p = (int *)__default_alloc_template<0, 0>::allocate(sizeof(int));  // 4
    int* p1 = (int*)__default_alloc_template<0, 0>::allocate(sizeof(int));  // 4

    int* p2 = (int*)__default_alloc_template<0, 0>::allocate(sizeof(int)*25); // 100
    int* p3 = (int*)__default_alloc_template<0, 0>::allocate(sizeof(int)*25); // 100


#if 0
    size_t n = 536870912;

    cout << "ROUND_UP(9):" << ROUND_UP(9) << endl;
    cout << "FREELIST_INDEX(9):" << FREELIST_INDEX(9) << endl;


    obj* free_list;

    free_list = CreatFreeList(5);

    // 添加数据
    Student* sp1;
    sp1 = (Student *)free_list->client_data;
    free_list = free_list->free_list_link;
    sp1->age = 20;
    strcpy(&sp1->name[0], "mic");


    // 移除数据
    obj* free_obj = (obj*)sp1;

    free_obj->free_list_link = free_list;
    free_list = free_obj;

#endif

   
#if 0
    // __malloc_alloc_template::set_malloc_handler(OutOfMem);
    int *p = (int *)__malloc_alloc_template<0>::allocate(n);
    int* p1 = (int*)__malloc_alloc_template<0>::allocate(n);
    int* p2 = (int*)__malloc_alloc_template<0>::allocate(n);
    int* p3 = (int*)__malloc_alloc_template<0>::allocate(n);
    int* p4 = (int*)__malloc_alloc_template<0>::allocate(n);
    int* p5 = (int*)__malloc_alloc_template<0>::allocate(n);
    
    __malloc_alloc_template<0>::deallocate(p, n);
    __malloc_alloc_template<0>::deallocate(p1, n);
    __malloc_alloc_template<0>::deallocate(p2, n);
    __malloc_alloc_template<0>::deallocate(p3, n);
    __malloc_alloc_template<0>::deallocate(p4, n);
    __malloc_alloc_template<0>::deallocate(p5, n);
   
#endif
    // set_new_handler(OutOfMem);
    // int* p = new int[n];
    // 
    // delete[]p;
    return 0;
}
