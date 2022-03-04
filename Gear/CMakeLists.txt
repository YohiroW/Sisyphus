project( Gear )
cmake_minimum_required( VERSION 3.16 )

##########
include( CheckCXXCompilerFlag )
check_cxx_compiler_flag( "-std=c++11"   COMPILER_SUPPORTS_CXX11 )
check_cxx_compiler_flag( "-std=c++0x"   COMPILER_SUPPORTS_CXX0X )
if( COMPILER_SUPPORTS_CXX11 )
    if( CMAKE_COMPILER_IS_GNUCXX )
        set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11" )
    else()
        set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11" )
    endif()
elseif( COMPILER_SUPPORTS_CXX0X)
    set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x" )
else()
    # MSVC, On by default (if available)
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
    Include/Base/Def.h
    Include/Base/RefCountedObject.h
    Include/Malloc/MallocBase.h
    Include/Benchmark/Benchmark.h

    Source/Benchmark/Benchmark.cpp
	Source/main.cpp)

add_executable(Gear "${srcs}")

# include directories
target_include_directories(Gear
    PUBLIC 
        ${PROJECT_SOURCE_DIR}/Include
)


