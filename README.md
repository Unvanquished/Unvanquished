Daemon Engine
=============


Dependencies
============

  * zlib
  * libgmp
  * libnettle
  * libGeoIP
  * libcurl
  * SDL2
  * GLEW
  * libpng
  * libjpeg (version 8 or above)
  * libwebp
  * Freetype
  * OpenAL
  * libogg
  * libvorbis
  * libspeex
  * libtheora
  * libopus
  * libopusfile
  * SQLite3


Build Instructions
==================

(CMake is required to build.)

Visual Studio
-------------

  1. Run CMake
  2. Choose your compiler
  3. Open `Daemon.sln` and compile


Linux, Mac OS X, MSYS
---------------------

  1. Create a build directory and go into it: `mkdir build && cd build`
  2. `cmake ..`
  3. `make -jN` (where `N` is your number of CPU cores to speed up compilation)


Linux cross-compile to Windows
------------------------------

  1. Create a build directory and go into it: `mkdir build && cd build`
  2. `cmake -DCMAKE_TOOLCHAIN_FILE=cmake/cross-toolchain-mingw32.cmake ..` (use cross-toolchain-mingw64.cmake for a Win64 build)
  3. `make -jN` (where `N` is your number of CPU cores to speed up compilation)


Mac OS X universal app
----------------------

  1. `mkdir build32 && cd build32`
  2. `cmake -DCMAKE_OSX_ARCHITECTURES=i386 -DCMAKE_OSX_DEPLOYMENT_TARGET=10.7 ..`
  3. `make -jN` (where `N` is your number of CPU cores to speed up compilation)
  4. `cd ..`
  5. `mkdir build64 && cd build64`
  6. `cmake -DCMAKE_OSX_ARCHITECTURES=x86_64 -DCMAKE_OSX_DEPLOYMENT_TARGET=10.7 ..`
  7. `make -jN` (where `N` is your number of CPU cores to speed up compilation)
  8. `cd ..`
  9. `./make-macosx-app.sh build32 build64`



Run Instructions for Unvanquished
=================================

  1. Download all the asset packs from
     http://www.sourceforge.net/projects/unvanquished/files/Assets and any maps (the mappack from
     http://sourceforge.net/projects/unvanquished/files/Map%20Pack/maps.7z/download
     is recommended) into main

  2. Run with `./daemon` (on \*nix) or `Daemon.exe` (on Windows)

