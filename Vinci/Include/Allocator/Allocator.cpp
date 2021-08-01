#include "Allocator.h"
#include <malloc.h>
#include <stdlib.h>
#include <immintrin.h>

//void* aligned_malloc(size_t size, size_t alignment)
//{
//	if (alignment & (alignment - 1))
//	{
//		return nullptr;
//	}
//	else
//	{
//		void* praw = malloc(sizeof(void*) + size + alignment);
//		if (praw)
//		{
//			void* pbuf = reinterpret_cast<void*>(reinterpret_cast<size_t>(praw) + sizeof(void*));
//			void* palignedbuf = reinterpret_cast<void*>((reinterpret_cast<size_t>(pbuf) | (alignment - 1)) + 1);
//			((static_cast<void**>palignedbuf)[-1]) = praw;
//			return palignedbuf;
//		}
//		else
//		{
//			return nullptr;
//		}
//	}
//}
//
//void aligned_free(void* palignedmem)
//{
//	free(reinterpret_cast<void*>((static_cast<void**>palignedmem)[-1]));
//}

void* BaseAllocator::Allocate(size_t size, size_t alignment, AllocScope scope)
{
	return malloc(size);
}

void* BaseAllocator::Reallocate(void* originAddr, size_t size, size_t alignment, AllocScope scope)
{
	Free(originAddr);
	return malloc(size);
}

void BaseAllocator::Free(void* addr)
{
	free(addr);
}
