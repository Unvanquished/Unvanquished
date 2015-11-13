#Unvanquished

[![GitHub tag](https://img.shields.io/github/tag/Unvanquished/Unvanquished.svg)](https://github.com/Unvanquished/Unvanquished/tags)
[![GitHub release](https://img.shields.io/github/release/Unvanquished/Unvanquished.svg)](https://github.com/Unvanquished/Unvanquished/releases/latest)
[![Github release](https://img.shields.io/github/downloads/Unvanquished/Unvanquished/latest/total.svg)](https://github.com/Unvanquished/Unvanquished/releases/latest)

[![IRC](http://img.shields.io/irc/%23unvanquished%2C%23unvanquished-dev.png)](https://webchat.freenode.net/?channels=%23unvanquished%2C%23unvanquished-dev)

| Windows | OSX | Linux |
|---------|-----|-------|
| [![AppVeyor branch](https://img.shields.io/appveyor/ci/DolceTriade/unvanquished/master.svg)](https://ci.appveyor.com/project/DolceTriade/unvanquished/history) | [![Travis branch](https://img.shields.io/travis/Unvanquished/Unvanquished/osx-ci.svg)](https://travis-ci.org/Unvanquished/Unvanquished/branches) | [![Travis branch](https://img.shields.io/travis/Unvanquished/Unvanquished/master.svg)](https://travis-ci.org/Unvanquished/Unvanquished/branches) |

This repository contains the gamelogic of the game Unvanquished.
You need to download the game's assets in addition to that to make it run.
See below for build and launch instructions.

##Dependencies

`zlib`,
`libgmp`,
`libnettle`,
`libcurl`,
`SDL2`,
`GLEW`,
`libpng`,
`libjpeg` ≥ 8,
`libwebp` ≥ 0.2.0,
`Freetype`,
`OpenAL`,
`libogg`,
`libvorbis`,
`libspeex`,
`libtheora`,
`libopus`,
`libopusfile`

###Buildtime

`cmake`

###Optional

`ncurses`,
`libGeoIP`

###MSYS2

`base-devel`

64-bit: `mingw-w64-x86_64-{toolchain,cmake,aria2}`
*or*
32-bit: `mingw-w64-i686-{toolchain,cmake,aria2}`

##Build Instructions

Instead of `make`, you can use `make -jN` where `N` is your number of CPU cores to speed up compilation.

###CBSE Toolchain

If you want to mess around with the gamelogic code and modify entities, you'll need the cbse toolchain.
Get it using:


    cd src/utils/cbse
    git submodule init
    git submodule update

Then you can modify the entities.yml file in src/sgame/. After modifying the entites.yml file,
you can run the generate_entities.sh script o regenerate the auto generated component code.

###Visual Studio

 1. Run CMake.
 2. Choose your compiler.
 3. Open `Unvanquished.sln` and compile.

###Linux, Mac OS X

    mkdir build && cd build
    cmake ..
    make

###MSYS2

    mkdir build && cd build
    cmake -G "MSYS Makefiles" ..
    make

###Linux cross-compile to Windows

    mkdir build && cd build
    cmake -DCMAKE_TOOLCHAIN_FILE=../daemon/cmake/cross-toolchain-mingw32.cmake .. # ¹
    make

¹ *Use `cross-toolchain-mingw64.cmake` for a Win64 build.*

###Mac OS X universal app

    mkdir build32 && cd build32
    cmake -DCMAKE_OSX_ARCHITECTURES=i386 -DCMAKE_OSX_DEPLOYMENT_TARGET=10.7 ..
    make
    cd ..
    mkdir build64 && cd build64
    cmake -DCMAKE_OSX_ARCHITECTURES=x86_64 -DCMAKE_OSX_DEPLOYMENT_TARGET=10.7 ..
    make
    cd ..
    ./make-macosx-app.sh build32 build64

##Launch Instructions

###Linux, Mac OS X, MSYS2

####If Unvanquished's assets are installed on your system

    cd build
    ./daemon -pakpath PATH # ¹

¹ *`PATH` is the path to the `pkg` directory that contains the game's assets.*

####If you don't have the assets

    cd build
    mkdir pkg

`../download-pk3-torrent.sh pkg # ¹`
*or*
`../download-pk3.sh pkg # ²`

    ./daemon

¹ *Fast, requires `aria2c`.*
² *Unreliable speed, requires `curl`.*

####If you're a developer

As a developer, you will want to load your own assets in addition to those shipped with the game. To do that:

    cd build
    mkdir -p pkg; cd pkg
    ln -s ../../main git_source.pk3dir
    mkdir assets_source.pk3dir
    echo git > assets_source.pk3dir/DEPS

You can now put loose assets into `assets_source.pk3dir` or you can put additional pk3dir directories or pk3 containers inside `pkg` and add their names (the format is `NAME_VERSION.pk3(dir)`) as lines to the `DEPS` file. In order to launch Unvanquished, use one of the following commands:

  - `./daemon -pakpath PATH -set fs_extrapaks assets # ¹`
  - `./daemon -pakpath PATH -set fs_extrapaks assets -set vm.sgame.type 3 -set vm.cgame.type 3 -set vm.sgame.debug 1 -set vm.cgame.debug 1 +devmap plat23 # ²`

¹ *Runs the game and loads the `assets` package and its dependencies. `PATH` is the path to Unvanquished's base packages and maps. Omit `-pakpath PATH` if `pkg` contains these assets.*
² *In addition, load a shared-object gamelogic you compiled and allow it to be debugged. Launch the map Platform 23 with cheats enabled after startup.*

###Windows

####If Unvanquished's assets are installed on your system

Run `daemon.exe -pakpath PATH`, where `PATH` is the path to the `pkg` directory that contains the game's assets.

####If you don't have the assets

  1. Copy the `pkg` directory from the release zip ([torrent](https://cdn.unvanquished.net/latest.php) | [web](http://sourceforge.net/projects/unvanquished/files/Universal_Zip/)) into your build directory.
  2. Run `daemon.exe`.
