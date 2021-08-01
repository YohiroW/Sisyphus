#pragma once

// [DEBUG] TO BE REMOVED
#define GLFW_INCLUDE_VULKAN

// default alignment in cpp
#define DEFAULT_MEMORY_ALIGNMENT 16

enum class AllocScope
{
	AS_COMMAND,
	AS_OBJECT,
	AS_CACHE,
	AS_DEVICE,
	AS_INSTANCE,
	AS_MAX
};

class IAllocatorInterface
{
public:
	virtual void* Allocate(size_t size, size_t alignment, AllocScope scope) = 0;
	virtual void* Reallocate(void* originAddr, size_t size, size_t alignment, AllocScope scope) = 0;
	virtual void Free(void* addr) = 0;
};

class BaseAllocator: public IAllocatorInterface
{
public:
	void* Allocate(size_t size, size_t alignment, AllocScope scope) override;
	void* Reallocate(void* originAddr, size_t size, size_t alignment, AllocScope scope) override;
	void Free(void* addr) override;
};

