#pragma once
#define BEGIN_NAMESPACE_GEAR namespace Gear {
#define END_NAMESPACE  }

BEGIN_NAMESPACE_GEAR 

template<typename T>
class RefCountedPtr
{
public:
    RefCountedPtr();
    RefCountedPtr(T* ptr);
    RefCountedPtr(const RefCountedPtr& other);
    ~RefCountedPtr();
    
}

END_NAMESPACE
