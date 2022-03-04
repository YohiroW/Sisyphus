project( Gear )
cmake_minimum_required( VERSION 3.16 )

# support latest cpp features
if (MSVC_VERSION GREATER_EQUAL "1900")
    include(CheckCXXCompilerFlag)
    CHECK_CXX_COMPILER_FLAG("/std:c++latest" _cpp_latest_flag_supported)
    if (_cpp_latest_flag_supported)
        add_compile_options("/std:c++latest")
    endif()
endif()

if(CMAKE_SIZEOF_VOID_P)
    set(platform x64)
else(CMAKE_SIZEOF_VOID_P)
    set(platform x86)
endif(CMAKE_SIZEOF_VOID_P)	 

# use unicode in projects		
add_definitions(-DUNICODE -D_UNICODE)

# include directories
include_directories("Include")

# source files
set(srcs
    Include/Gear.h    
    Include/Base/Defs.h
    Include/Base/Types.h
    Include/Base/Misc.h   
    Include/Base/Log.h
    Include/Base/RefCountedObject.h
    Source/Base/Misc.cpp
    Source/Base/Log.cpp

    Include/Malloc/MallocBase.h
    Include/Benchmark/Benchmark.h
    Source/Benchmark/Benchmark.cpp
	
    Include/TestCase/TestCaseInterface.h
    Source/TestCase/TestCaseAllocation.hpp
    Source/TestCase/TestCasePerfStress.hpp
    Source/TestCase/TestCaseReflection.hpp 
    
    Source/main.cpp
    )

add_executable(Gear "${srcs}")

# include directories
target_include_directories(Gear
    PUBLIC 
        ${PROJECT_SOURCE_DIR}/Include
)


