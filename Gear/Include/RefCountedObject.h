#pragma once
#define BEGIN_NAMESPACE_GEAR namespace Gear {
#define END_NAMESPACE  }

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
    static voide Collect();

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

END_NAMESPACE
