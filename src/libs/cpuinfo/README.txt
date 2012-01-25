CPUInfo readme

CPUInfo is a simple library aimed at determining CPU features, specs and related OS properties in an easy and unified way, among a wide range of CPUs and OSes. Developers can use it as information (eg log/debug), and to select optimum codepaths at runtime. 
The library is open source and free to use and modify under the BSD license. 
Included is a test application that displays some of the main features. The sourcecode can be studied to find out how to use the CPUInfo library and how to extract the information you are interested in, for use in your own applications.

The library and the test application have been written in a portable style, and can at least be compiled with Microsoft and GNU compilers. Both 32-bit and 64-bit environments are supported, and the code has been tested under Windows XP, Vista, 7, Gentoo Linux, FreeBSD and OS X.

Currently there are solutions and project files included for both Visual Studio 2005 (suffix _VS2005) and 2008 (suffix _VS2008). This builds the CPUInfo.lib file.
There is also a basic makefile for use with various *nix-style environments. This builds the libcpuinfo.a file.

For use in your own applications, you can simply include CPUInfo.h, and link to CPUInfo.lib or libcpuinfo.a depending on your environment.
For MASM users there are CPUInfo.inc and CPUFlags.inc.

There are also some related projects included, mostly aimed at Windows.

Launch3264:
The Launch3264 application is a 32-bit application which will determine whether you are running under a 32-bit or 64-bit Windows environment, and then it will launch the appropriate binary for you.
This makes 32-bit or 64-bit transparent to the end-user, they only see a single executable, and just start it.
The idea is that you rename the launch application to the name you want your application to have, say MyApplication.exe.
The launcher will then look for a MyApplication32.bin on a 32-bit OS, or MyApplication64.bin on a 64-bit OS (it derives these files from its own filename, so no configuration or recompilation is necessary).
So you rename your actual 32-bit application to MyApplication32.bin, and your actual 64-bit application to MyApplication64.bin, and then place them in the same directory with the launcher, which is named MyApplication.exe.

Test3264:
This is a simple program for use as a test with Launch3264, to see if it actually picks the expected version of the executable.

Dependencies:
This is a tool that walks through the import and export tables of a Windows executable or DLL, and checks for any missing imports or exports. This can be useful for more detailed error messages or debugging when an application fails to load on a particular installation of Windows.

Bench.cpp/Bench.h:
This is a simple routine that can be used for counting the number of CPU cycles that a certain function takes.