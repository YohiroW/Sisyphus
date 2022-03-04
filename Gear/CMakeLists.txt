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
# includes
    Include/Base/Def.h
    Include/Base/RefCountedObject.h
    Include/Malloc/MallocBase.h
    Include/Benchmark/Benchmark.h
# sources
    Source/Benchmark/Benchmark.cpp
	Source/main.cpp
# test cases
    Include/TestCase/TestCastInterface.h
    Source/TestCase/TestCaseAllocation.hpp
    Source/TestCase/TestCasePerfStress.hpp
    Source/TestCase/TestCaseReflection.hpp 
    )

add_executable(Gear "${srcs}")

# include directories
target_include_directories(Gear
    PUBLIC 
        ${PROJECT_SOURCE_DIR}/Include
)


