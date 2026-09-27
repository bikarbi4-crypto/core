#include "allocations.h"
#include <new>
#include <cstdlib>
#include <malloc.h>
void* operator new(std::size_t n) {Allocations::record(n);if(auto* p=std::malloc(n?n:1))return p;throw std::bad_alloc();}
void* operator new[](std::size_t n){return ::operator new(n);}
void operator delete(void* p) noexcept{std::free(p);}
void operator delete[](void* p) noexcept{std::free(p);}
void operator delete(void* p,std::size_t) noexcept{std::free(p);}
void operator delete[](void* p,std::size_t) noexcept{std::free(p);}
void* operator new(std::size_t n,std::align_val_t a){Allocations::record(n);if(auto* p=_aligned_malloc(n?n:1,(std::size_t)a))return p;throw std::bad_alloc();}
void operator delete(void* p,std::align_val_t) noexcept{_aligned_free(p);}
void operator delete(void* p,std::size_t,std::align_val_t) noexcept{_aligned_free(p);}
