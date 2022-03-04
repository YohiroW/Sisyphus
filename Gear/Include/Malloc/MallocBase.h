#include <stddef.h>
#include <atomic>


//
extern class FallocBase* GlobalMalloc;

class MallocBase
{
public:
	virtual void* Malloc(size_t size, unsigned int align) = 0;
	virtual void* Realloc(void* origin, size_t size, unsigned int align) = 0;
	virtual void Free(void* origin) = 0;

	// 
	
};