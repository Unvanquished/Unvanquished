#Unvanquished#

This repository contains the engine and gamelogic of the game Unvanquished. You need to download the game's assets in addition to that to make it run. See below for build and launch instructions.

##Dependencies##

`zlib`, `libgmp`, `libnettle`, `libcurl`, `SDL2`, `GLEW`, `libpng`, `libjpeg` ≥ 8, `libwebp` ≥ 0.2.0, `Freetype`, `OpenAL`, `libogg`, `libvorbis`, `libspeex`, `libtheora`, `libopus`, `libopusfile`

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
  2. `cmake -DBUILD_GAME_NACL=OFF -DCMAKE_TOOLCHAIN_FILE=cmake/cross-toolchain-mingw32.cmake ..`¹
  3. `make`

¹ *Use `cross-toolchain-mingw64.cmake` for a Win64 build.*

###Mac OS X universal app###

  1. `mkdir build32 && cd build32`
  2. `cmake -DCMAKE_OSX_ARCHITECTURES=i386 -DCMAKE_OSX_DEPLOYMENT_TARGET=10.7 ..`
  3. `make`
  4. `cd ..`
  5. `mkdir build64 && cd build64`
  6. `cmake -DCMAKE_OSX_ARCHITECTURES=x86_64 -DCMAKE_OSX_DEPLOYMENT_TARGET=10.7 ..`
  7. `make`
  8. `cd ..`
  9. `./make-macosx-app.sh build32 build64`

##Launch Instructions##

###Linux, Mac OS X, MSYS###

####If Unvanquished's assets are installed on your system####

  1. `cd build`
  2. `./daemon -pakpath PATH`¹

¹ *`PATH` is the path to the `pkg` directory that contains the game's assets.*

####If you don't have the assets####

  1. `cd build`
  2. `mkdir pkg`
  3. `../download-pk3-torrent.sh pkg`¹ or `../download-pk3.sh pkg`²
  4. `./daemon`

¹ *Fast, requires `aria2c`.*
² *Unreliable speed, requires `curl`.*

####If you're a developer####

As a developer, you will want to load your own assets in addition to those shipped with the game. To do that:

  1. `cd build`
  2. `mkdir -p pkg; cd pkg`
  3. `ln -s ../../main git_source.pk3dir`
  4. `mkdir assets_source.pk3dir`
  5. `echo git > assets_source.pk3dir/DEPS`

You can now put loose assets into `assets_source.pk3dir` or you can put additional pk3dir directories or pk3 containers inside `pkg` and add their names (the format is `NAME_VERSION.pk3(dir)`) as lines to the `DEPS` file. In order to launch Unvanquished, use one of the following commands:

  - `./daemon -pakpath PATH -set fs_extrapaks assets`¹
  - `./daemon -pakpath PATH -set fs_extrapaks assets -set vm.sgame.type 3 -set vm.cgame.type 3 -set vm.sgame.debug 1 -set vm.cgame.debug 1 +devmap plat23`²

¹ *Runs the game and loads the* `assets` *package and its dependencies.* `PATH` *is the path to Unvanquished's base packages and maps. Omit* `-pakpath PATH` *if* `pkg` *contains these assets.*
² *In addition to* 1.*, load a shared-object gamelogic you compiled and allow it to be debugged. Launch the map Platform 23 with cheats enabled after startup.*

###Windows###

####If Unvanquished's assets are installed on your system####

Run `Daemon.exe -pakpath PATH`, where `PATH` is the path to the `pkg` directory that contains the game's assets.

####If you don't have the assets####

  1. Copy the `pkg` directory from the release zip ([torrent](https://cdn.unvanquished.net/latest.php), [web](http://sourceforge.net/projects/unvanquished/files/Universal_Zip/)) into your build directory.
  2. Run `Daemon.exe`.
