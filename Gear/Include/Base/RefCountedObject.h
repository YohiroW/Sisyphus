#pragma once
#include"Def.h"

BEGIN_NAMESPACE_GEAR 

//  Object should provide functions like serialize/fastload/duplicate/call bridge/get descriptor/register/construct/destruct and so on...
//  Which should be a meta object with class descriptor, name, id, 
class RefCountedObject: public Object
{
public:
    typedef Object super;

    static ClassDescriptor* GetStaticClassDescriptor();
    virtual ClassDescriptor* GetClassDescriptor() const;
    
    RefCountedObject();
    virtual ~RefCountedObject();
    
    void Increase();
    void Decrease();
    bool IsShared();
    
    static void Initialize();
    static void Shutdown();
    
    // Should be called from safe place where ref count not changed.
    static void Collect();

protected:
    void ReleaseResources();
    
private:
    // Is this necessary?
    // RefCountedObject(const RefCountedObject& r);
    
    virtual void Delete();
    
private:
    int32 ReferenceCount;
    bool bIsGarbage;
    
    static <AdaptiveLock> GC_Lock;
    static PtrArray<RefCountedObject> GarbageList;
};

template<typename T>
class RefCountedPtr
{
public:
    RefCountedPtr();bool
    RefCountedPtr(T* ptr);
    RefCountedPtr(const RefCountedPtr& other);
    ~RefCountedPtr();
    
    // Assign
    RefCountedPtr& operator = (const RefCountedPtr& other);
    RefCountedPtr& operator = (T* ptr);
    
    bool operator == (const RefCountedPtr& other) const;
    bool operator == (T* ptr) const;
    bool operator != (const RefCountedPtr& other) const;
    bool operator != (T* ptr) const;
    
    bool operator < (const RefCountedPtr& other) const;
    
    //
    T* operator -> () const;
    T* GetPtr() const;
    
    //
    void Release();

private:
    T* Ptr;
}

//////////////////////////////////////////////////////////////////////
// Inline implementation

inline void RefCountedObject::Increase()
{
    Atomic::Increment(ReferenceCount);
}

inline void RefCountedObject::Decrease()
{
    int32 refCount = Atomic::Decrement(ReferenceCount);
    
    if(refCount< 1)
    {
       Delete(); 
    }
}

inline int32 GetRefCount()
{
    return referenceCount;
}

inline bool IsShared()
{
    return referenceCount > 1;
}


template<typename T>
inline RefCountedPtr<T>::RefCountedPtr(T* ptr): 
 Ptr(ptr)
 {
     // avoid LHS
     if(ptr)
     {
         ptr->Increase();
     }
 }

template<typename T>
inline RefCountedPtr<T>::~RefCountedPtr()
{
    T* ptr = Ptr;
    if(ptr)
    {
        ptr->Decrease();
    }
}

template<typename T>
inline RefCountedPtr<T>& RefCountedPtr<T>::operator = (const RefCountedPtr& other)
{
    // Not thread safe
    T* newPtr = other.Ptr;
    if(newPtr)
    {
        newPtr->Increase();
    }
    
    T* oldPtr = Ptr;
    Ptr = newPtr;
    
    // Safe operation
    if(oldPtr)
    {
        oldPtr->Decrease();
    }
    
    return (*this);
}

template<typename T>
inline RefCountedPtr<T>& RefCountedPtr<T>::operator = (T* other)
{
    if(other)
    {
        other->Increase();
    }
    
    T* oldPtr = Ptr;
    Ptr = other;
    
    // Safe operation
    if(oldPtr)
    {
        oldPtr->Decrease();
    }
    
    return (*this);
}

template<typename T>
inline bool RefCountedPtr<T>::operator == (const T* other) const
{
    return Ptr == other;
}

template<typename T>
inline bool RefCountedPtr<T>::operator == (const RefCountedPtr& other) const
{
    return Ptr == other.Ptr;
}

template<typename T>
inline bool RefCountedPtr<T>::operator != (const T* other) const
{
    return Ptr != other;
}

template<typename T>
inline bool RefCountedPtr<T>::operator != (const RefCountedPtr& other) const
{
    return Ptr != other.Ptr;
}

template<typename T>
inline T* RefCountedPtr<T>::operator -> () const
{
    return Ptr;
}

template<typename T>
inline T* RefCountedPtr<T>::GetPtr() const
{
    return Ptr;
}

template<typename T>
inline void RefCountedPtr<T>::Release()
{
    T* ptr = Ptr;
    Ptr = nullptr;
    
    if(ptr)
    {
        ptr->Decrease();
    }
}

END_NAMESPACE
