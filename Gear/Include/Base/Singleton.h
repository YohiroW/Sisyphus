#pragma once

#include <new>

BEGIN_NAMESPACE_GEAR

class SingletonLazy
{
protected:
    template<typename T> static void Construct(void* place)
    {
        new (place) T;
    }

    template<typename T> static void Destruct(void* singleton)
    {
        singleton->~T();
    }

};

template<typename T>
class Singleton<T>
{
public:


protected:

}


END_NAMESPACE