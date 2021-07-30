echo off

%VULKAN_SDK%/Bin/glslangValidator.exe -V DummyVertexShader.vert
%VULKAN_SDK%/Bin/glslangValidator.exe -V DummyPixelShader.frag

pause