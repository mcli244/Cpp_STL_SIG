#include <iostream>
#include "stl_alloc.h"

using namespace std;

void OutOfMem(void)
{
    cout << "Out of memery\n";
}
int main()
{
    cout << "Hello World!123\n";
    size_t n = 536870912;

    // set_new_handler(OutOfMem);
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
    // 
    // int* p = new int[n];
    // 
    // delete[]p;
    return 0;
}
