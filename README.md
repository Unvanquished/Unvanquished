Daemon Engine
=============


Dependencies
============

  * Freetype
  * libpng
  * libjpeg (DO NOT USE VERSION 6)
  * libcurl
  * libsdl
  * OpenAL (optional)
  * libwebp ( >= 0.2.0 )
  * libxvid (optional)
  * Newton (provided)
  * libgmp
  * GLEW
  * theora
  * speex
  * speexdsp
  * nettle



Build Instructions
==================

Visual Studio
-------------

(CMake is required to build.)

  1. Execute either `Visual_Studio32.bat` or `Visual_Studio64.bat`
     Navigate to `Unvanquished/build-32` or `Unvanquished/build-64`
     Open Daemon.sln


Linux
-----

(CMake is required to build.)

  1. `mkdir build && cd build` (from Unvanquished root directory)
  2. `ccmake ..`
  3. Press 'c'
  4. Fill in the blanks for any libraries that you cannot find.
  5. Press 'c' and then 'g'
  6. `make` (use `make -jN` where `N` is your number of CPU cores to speed up compilation)


MINGW + MSYS
------------
  1. `mkdir build && cd build` (from Unvanquished root directory)
  2. `cmake -G "MSYS Makefiles" -DCMAKE_PREFIX_PATH="C:\MINGW"  ..`
  3. `make`



Run Instructions for Unvanquished
=================================

  1. Download all the asset packs from
     http://www.sourceforge.net/projects/unvanquished/files/Assets and any maps (the mappack from
     http://sourceforge.net/projects/unvanquished/files/Map%20Pack/maps.7z/download
     is recommended) into main

  2. Run with `./daemon` (on \*nix) or `Daemon.exe` (on Windows)

