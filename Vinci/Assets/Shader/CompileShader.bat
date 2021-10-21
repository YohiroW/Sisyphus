echo off

rem cd %dp0

REM #!/bin/bash
REM ## -V: create SPIR-V binary
REM ## -x: save binary output as text-based 32-bit hexadecimal numbers
REM ## -o: output file
REM glslangValidator -V -x -o glsl_shader.frag.u32 glsl_shader.frag
REM glslangValidator -V -x -o glsl_shader.vert.u32 glsl_shader.vert

%VULKAN_SDK%/Bin/glslangValidator.exe -V DummyVertexShader.vert
%VULKAN_SDK%/Bin/glslangValidator.exe -V DummyPixelShader.frag

pause