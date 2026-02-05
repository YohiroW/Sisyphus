@echo off

set GENERATOR="Visual Studio 17 2022"

if not exist "Build" mkdir Build
cd Build

cmake -G %GENERATOR% ..

pause