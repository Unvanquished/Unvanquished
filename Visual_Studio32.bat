@echo off
mkdir build-32
cd build-32
REM Get value for %args%
call "%~dp0/cmake/Visual_Studio_arguments.bat"
cmake -C../cmake/Visual_Studio32.cmake %args%  -G "Visual Studio 10" ..
pause
