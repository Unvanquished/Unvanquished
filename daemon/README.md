#Daemon#

This repository contains the engine of the game Unvanquished. See below for build and launch instructions.

##Dependencies##

`zlib`, `libgmp`, `libnettle`, `libcurl`, `SDL2`, `GLEW`, `libpng`, `libjpeg` ≥ 8, `libwebp` ≥ 0.2.0, `Freetype`, `OpenAL`, `libogg`, `libvorbis`, `libtheora`, `libopus`, `libopusfile`

####Buildtime####

`cmake`

####Optional####

`ncurses`, `libGeoIP`

##Build Instructions##

Instead of `make`, you can use `make -jN` where `N` is your number of CPU cores to speed up compilation.

###Visual Studio###

  1. Run CMake.
  2. Choose your compiler.
  3. Open `Daemon.sln` and compile.

###Linux, Mac OS X, MSYS###

  1. `mkdir build && cd build`
  2. `cmake ..`
  3. `make`

###Linux cross-compile to Windows###

  1. `mkdir build && cd build`
  2. `cmake -DCMAKE_TOOLCHAIN_FILE=../cmake/cross-toolchain-mingw32.cmake ..`¹
  3. `make`

¹ *Use `cross-toolchain-mingw64.cmake` for a Win64 build.*
