# Unvanquished

[![GitHub tag](https://img.shields.io/github/tag/Unvanquished/Unvanquished.svg)](https://github.com/Unvanquished/Unvanquished/tags)
[![GitHub release](https://img.shields.io/github/release/Unvanquished/Unvanquished.svg)](https://github.com/Unvanquished/Unvanquished/releases/latest)
[![Github release](https://img.shields.io/github/downloads/Unvanquished/Unvanquished/latest/total.svg)](https://github.com/Unvanquished/Unvanquished/releases/latest)

[![IRC](https://img.shields.io/badge/irc-%23unvanquished%2C%23unvanquished--dev-4cc51c.svg)](https://webchat.freenode.net/?channels=%23unvanquished%2C%23unvanquished-dev)

| Windows | OSX | Linux |
|---------|-----|-------|
| [![AppVeyor branch](https://img.shields.io/appveyor/ci/DolceTriade/unvanquished/master.svg)](https://ci.appveyor.com/project/DolceTriade/unvanquished/history) | [![Travis branch](https://img.shields.io/travis/Unvanquished/Unvanquished/master.svg)](https://travis-ci.org/Unvanquished/Unvanquished/branches) | [![Travis branch](https://img.shields.io/travis/Unvanquished/Unvanquished/master.svg)](https://travis-ci.org/Unvanquished/Unvanquished/branches) |

This repository contains the gamelogic of the game Unvanquished.

If you just want to play the game, then technically you only need to build the [Dæmon engine](https://github.com/DaemonEngine/Daemon) and [download the game's assets](#downloading-the-games-assets).

You need to build this repository if you want to contribute to the gamelogic or make a mod.

See below for build and launch instructions.

## Workspace requirements

To fetch and build Unvanquished, you'll need:
`git`,
`cmake`,
`python` = 2,
`python-yaml`,
`python-jinja`,
and a C++11 compiler.

The following are actively supported:
`gcc` ≥ 4.8,
`clang` ≥ 3.5,
Visual Studio/MSVC (at least Visual Studio 2019).

## Dependencies

### Required

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
`Lua`,
`OpenAL`,
`libogg`,
`libvorbis`,
`libtheora`,
`libopus`,
`libopusfile`

### Optional

`ncurses`,
`libGeoIP`

### MSYS2

`base-devel`

64-bit: `mingw-w64-x86_64-{toolchain,cmake,aria2}`  
_or_ 32-bit: `mingw-w64-i686-{toolchain,cmake,aria2}`

MSYS2 is an easy way to get MingW compiler and build dependencies, the standalone MingW on Windows also works.

## Download instructions

Unvanquished requires several sub-repositories to be fetched before compilation. If you have not yet cloned this repository:

```sh
git clone --recurse-submodules https://github.com/Unvanquished/Unvanquished.git
```

If you have already cloned:

```sh
cd Unvanquished/
git submodule update --init --recursive
```

If cmake complains about missing files in `daemon/` folder or similar issue then you have skipped this step.

## Build Instructions

Instead of `-j4` you can use `-jN` where `N` is your number of CPU cores to distribute compilation on them. Linux systems usually provide an handy `nproc` tool that tells the number of CPU core so you can just do `-j$(nproc)` to use all available cores.

Enter the directory before anything else:

```sh
cd Unvanquished/
```

### Visual Studio

1. Run CMake.
2. Choose your compiler.
3. Open `Unvanquished.sln` and compile.

### Linux, macOS

Produced files will be stored in a new directory named `build`.

```sh
cmake -H. -Bbuild
cmake --build build -- -j4
```

### MSYS2

```sh
cmake -H. -Bbuild -G "MSYS Makefiles"
cmake --build build -- -j4
```

### Linux cross-compile to Windows

For a 32-bit build use the `cross-toolchain-mingw32.cmake` toolchain file instead.

```sh
cmake -H. -Bbuild -DCMAKE_TOOLCHAIN_FILE=cmake/cross-toolchain-mingw64.cmake
cmake --build build -- -j4
```

## Launch Instructions

### Linux, macOS, MSYS2

#### If Unvanquished's assets are already installed somewhere on your system

```sh
cd build
# <PATH> is the path to the “pkg/” directory that contains the game's assets.
./daemon -pakpath <PATH>
```

#### Downloading the game's assets

If you don't have the assets, you can download them first.

The package downloader script can use `aria2c`, `curl` or `wget`, `aria2c` is recommended.
You can do `./download-paks --help` for more options.

```sh
./download-paks build/pkg
```

```sh
cd build
./daemon
```

#### Loading base asset package from sources

As a developer, you will want to load assets from repository. To do that:

```sh
cd build
./daemon -pakpath ../pkg -set vm.sgame.type 3 -set vm.cgame.type 3
```

Note that only the basic `unvanquished_src.dpkdir` asset package is provided that way, and running Unvanquished only with that package will bring you some warnings about other missing packages and you will miss soundtrack and stuff like that. You also need to load your own game code (using _vm type_ switches) at this point.

This should be enough to start the game and reach the main menu and from there, join a server. If the server supports autodownload mechanism, Unvanquished will fetch all the missing packages from it.

If you are looking for the sources of the whole assets, have a look at the [UnvanquishedAssets](https://github.com/UnvanquishedAssets/UnvanquishedAssets) repository. Beware that unlike the `unvanquished_src.dpkdir` package most of them can't be loaded correctly by the engine without being built first.

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
```

### Windows

#### If Unvanquished's assets are installed on your system

Run `daemon.exe -pakpath <PATH>`, where `<PATH>` is the path to the `pkg/` directory that contains the game's assets.

#### If you don't have the assets

1. Copy the `pkg/` directory from the release zip ([torrent](https://cdn.unvanquished.net/latest.php) | [web](https://github.com/Unvanquished/Unvanquished/releases)) into your build directory.
2. Run `daemon.exe`.
