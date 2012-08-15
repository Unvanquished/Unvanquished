@echo off
mkdir build-64
cd build-64
REM Get value for %args%
call "%~dp0/cmake/Visual_Studio_arguments.bat"
cmake -C../cmake/Visual_Studio64.cmake %args% -G "Visual Studio 10 Win64" ..
pause
