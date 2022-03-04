#pragma once

#include "Gear.h"
#include <stddef.h>
#include <atomic>

BEGIN_NAMESPACE_GEAR

extern class FallocBase* GlobalMalloc;

class MallocBase
{
public:
	virtual void* Malloc(size_t size, unsigned int align) = 0;
	virtual void* Realloc(void* origin, size_t size, unsigned int align) = 0;
	virtual void Free(void* origin) = 0;

	// 
	
};

END_NAMESPACE