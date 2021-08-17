@echo off

set GENERATOR="Visual Studio 16 2019"

mkdir Build
cd Build
cmake -G %GENERATOR% ..

pause