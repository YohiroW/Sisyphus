project( Vinci )
cmake_minimum_required( VERSION 3.18 )

##### To test branch #####

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

# use unicode in projects		
add_definitions(-DUNICODE -D_UNICODE)

set(thirdPartyPath "${CMAKE_SOURCE_DIR}/ThirdParty")

# include directories
include_directories("$ENV{VULKAN_SDK}/Include/")
include_directories("ThirdParty/glfw-3.3.4/include/")

# add referenced library		 		 
set(libs glfw3.lib)

# source files
set(srcs
	Source/main.cpp)

if(CMAKE_SIZEOF_VOID_P)
    set(platform x64)
else(CMAKE_SIZEOF_VOID_P)
    set(platform x86)
endif(CMAKE_SIZEOF_VOID_P)	 

link_directories("${thirdPartyPath}/glfw-3.3.4/Lib/")

add_executable(Vinci "${srcs}")
target_link_libraries(Vinci "${libs}")

# Add custom command to copy data needed
# add_custom_command(
    # TARTGET Vinci
    # POST_BUILD
    # COMMAND if not exist "$(OutDir)renderdoc.dll"
    # copy /Y
    # "$(OutDir)..\\..\\ThirdParty\\RenderDoc\\renderdoc.dll"
    # "$(OutDir)")

# working directories
# set_target_properties(Vinci PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/Assets")
# set_target_properties(Vinci PROPERTIES LINK_FLAGS "/SUBSYSTEM:WINDOWS")
