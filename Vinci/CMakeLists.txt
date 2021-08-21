project( Vinci )
cmake_minimum_required( VERSION 3.12 )

# support latest cpp features
if (MSVC_VERSION GREATER_EQUAL "1900")
    include(CheckCXXCompilerFlag)
    CHECK_CXX_COMPILER_FLAG("/std:c++latest" _cpp_latest_flag_supported)
    if (_cpp_latest_flag_supported)
        add_compile_options("/std:c++latest")
    endif()
endif()

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(platform x64)
else(CMAKE_SIZEOF_VOID_P EQUAL 4)
    set(platform x86)
endif()	

# use unicode in projects		
add_definitions(-DUNICODE -D_UNICODE)

set(thirdPartyPath "${CMAKE_SOURCE_DIR}/ThirdParty")

# include directories
include_directories("$ENV{VULKAN_SDK}/Include/")
include_directories("ThirdParty/glfw-3.3.4/include/")
include_directories("ThirdParty/fbxsdk/include")

# add referenced library		 		 
set(libs 
	glfw3.lib
	vulkan-1.lib)

# source files
set(srcs
	Include/Allocator/Allocator.h
	Include/Allocator/Allocator.cpp
	Include/Math/Math.hpp
	Source/main.cpp) 

link_directories("${thirdPartyPath}/glfw-3.3.4/Lib/")
link_directories("$ENV{VULKAN_SDK}/Lib/")

add_executable(Vinci "${srcs}")
target_link_libraries(Vinci "${libs}")

# Compile shaders when finish build
add_custom_command(
    TARGET Vinci
    POST_BUILD
    COMMAND "$(OutDir)..\\..\\Shader\\CompileShader.bat")

# Add custom command to copy data needed
add_custom_command(
    TARGET Vinci
    POST_BUILD
    COMMAND if not exist "$(OutDir)renderdoc.dll"
    copy /Y
    "$(OutDir)..\\..\\ThirdParty\\RenderDoc\\renderdoc.dll"
    "$(OutDir)")

# working directories
# set_target_properties(Vinci PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/Assets")
# set_target_properties(Vinci PROPERTIES LINK_FLAGS "/SUBSYSTEM:WINDOWS")
