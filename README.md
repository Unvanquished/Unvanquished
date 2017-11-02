# Unvanquished

[![GitHub tag](https://img.shields.io/github/tag/Unvanquished/Unvanquished.svg)](https://github.com/Unvanquished/Unvanquished/tags)
[![GitHub release](https://img.shields.io/github/release/Unvanquished/Unvanquished.svg)](https://github.com/Unvanquished/Unvanquished/releases/latest)
[![Github release](https://img.shields.io/github/downloads/Unvanquished/Unvanquished/latest/total.svg)](https://github.com/Unvanquished/Unvanquished/releases/latest)

[![IRC](http://img.shields.io/irc/%23unvanquished%2C%23unvanquished-dev.png)](https://webchat.freenode.net/?channels=%23unvanquished%2C%23unvanquished-dev)

| Windows | OSX | Linux |
|---------|-----|-------|
| [![AppVeyor branch](https://img.shields.io/appveyor/ci/DolceTriade/unvanquished/master.svg)](https://ci.appveyor.com/project/DolceTriade/unvanquished/history) | [![Travis branch](https://img.shields.io/travis/Unvanquished/Unvanquished/master.svg)](https://travis-ci.org/Unvanquished/Unvanquished/branches) | [![Travis branch](https://img.shields.io/travis/Unvanquished/Unvanquished/master.svg)](https://travis-ci.org/Unvanquished/Unvanquished/branches) |

This repository contains the gamelogic of the game Unvanquished.

You need to download the game's assets in addition to that to make it run.
See below for build and launch instructions.

## Dependencies

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
`libtheora`,
`libopus`,
`libopusfile`

### Buildtime

`cmake`,
`python` ≥ 2,
`python-yaml`,
`python-jinja`

### Optional

`ncurses`,
`libGeoIP`

### MSYS2

`base-devel`

64-bit: `mingw-w64-x86_64-{toolchain,cmake,aria2}`  
_or_ 32-bit: `mingw-w64-i686-{toolchain,cmake,aria2}`

## Build Instructions

Instead of `make`, you can use `make -jN` where `N` is your number of CPU cores to speed up compilation. Linux systems usually provide an handy `nproc` tool that tells the number of CPU core so you can just do `make -j$(nproc)` to use all available cores.

### Visual Studio

1. Run CMake.
2. Choose your compiler.
3. Open `Unvanquished.sln` and compile.

### Linux, Mac OS X

```sh
mkdir build && cd build
cmake ..
make
```

### MSYS2

```sh
mkdir build && cd build
cmake -G "MSYS Makefiles" ..
make
```

### Linux cross-compile to Windows

```sh
mkdir build && cd build
# Use “cross-toolchain-mingw64.cmake” for a Win64 build.
cmake -DCMAKE_TOOLCHAIN_FILE=../daemon/cmake/cross-toolchain-mingw32.cmake ..
make
```

### Mac OS X universal app

```sh
mkdir build32 && cd build32
cmake -DCMAKE_OSX_ARCHITECTURES=i386 -DCMAKE_OSX_DEPLOYMENT_TARGET=10.7 ..
make
cd ..
mkdir build64 && cd build64
cmake -DCMAKE_OSX_ARCHITECTURES=x86_64 -DCMAKE_OSX_DEPLOYMENT_TARGET=10.7 ..
make
cd ..
./make-macosx-app.sh build32 build64
```

## Launch Instructions

### Linux, Mac OS X, MSYS2

#### If Unvanquished's assets are already installed somewhere on your system

```sh
cd build
# <PATH> is the path to the “pkg/” directory that contains the game's assets.
./daemon -pakpath <PATH>
```

#### If you don't have the assets, you can download them first


```sh
cd build
mkdir pkg
```

```sh
# Fast, requires “aria2c”:
../download-dpk-torrent.sh pkg
# Or with unreliable speed, requires “curl”:
../download-dpk.sh pkg
```

```sh
./daemon
```

#### Loading base asset package from sources

As a developer, you will want to load assets from repository. To do that:

```sh
cd build
./daemon -pakpath ../pkg -set vm.sgame.type 3 -set vm.cgame.type 3
```

Note that only the basic `unvanquished_src.dpkdir` asset package is provided that way, and running Unvanquished only with that package will bring you some warnings about other missing packages and you will miss soundtrack and stuff like that. Note that you need to load your own game code (using _vm type_ switches) at this point.

This should be enough to start the game and reach the main menu and from there, join a server. If the server supports autodownload mechanism, Unvanquished will fetch all the missing packages from it.

If you are looking for the sources of the whole assets, have a look at the [UnvanquishedAssets](https://github.com/UnvanquishedAssets/UnvanquishedAssets) repository. Beware, unlike the `unvanquished_src.dpkdir` package, most of them can't be loaded correctly by the engine without being built first.

#### Loading your own assets

As a developer, you will want to load your own assets in addition to those shipped with the game. To do that:

```sh
cd build
mkdir -p pkg && cd pkg
mkdir assets_src.dpkdir
```

You can now put loose assets into `assets_src.dpkdir` or you can put additional dpkdir directories or dpk containers inside `pkg` and add their names (the format is `<NAME>_<VERSION>.dpk[dir]`) as lines to the `DEPS` file (the format is `<NAME> <VERSION>`). Version is required in package filename but optional in `DEPS` file. In order to launch Unvanquished, use one of the following commands:

```sh
# Runs the game and loads the “assets” package and its dependencies from “pkg/” directory:
./daemon -set fs_extrapaks assets

# Runs the game and loads the “assets” package and its dependencies from <PATH>
# when <PATH> is not one of the default Unvanquished paths:
./daemon -pakpath <PATH> -set fs_extrapaks assets

# In addition, load a shared-object gamelogic you compiled and allow it to be debugged,
# then launch the map “Platform 23” with cheats enabled after startup:
./daemon -pakpath <PATH> -set fs_extrapaks assets \
			-set vm.sgame.type 3 -set vm.cgame.type 3 \
			+devmap plat23

# Same but load an NaCl gamelogic you compiled and allow it to be debugged:
./daemon -pakpath <PATH> -set fs_extrapaks assets \
			-set vm.sgame.type 1 -set vm.cgame.type 1 \
			-set vm.sgame.debug 1 -set vm.cgame.debug 1 \
			+devmap plat23
```

### Windows

#### If Unvanquished's assets are installed on your system

Run `daemon.exe -pakpath <PATH>`, where `<PATH>` is the path to the `pkg/` directory that contains the game's assets.

#### If you don't have the assets

1. Copy the `pkg/` directory from the release zip ([torrent](https://cdn.unvanquished.net/latest.php) | [web](https://github.com/Unvanquished/Unvanquished/releases)) into your build directory.
2. Run `daemon.exe`.
