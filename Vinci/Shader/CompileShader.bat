echo off

rem cd %dp0

%VULKAN_SDK%/Bin/glslangValidator.exe -V DummyVertexShader.vert
%VULKAN_SDK%/Bin/glslangValidator.exe -V DummyPixelShader.frag

pause