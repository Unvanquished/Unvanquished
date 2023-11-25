# Unvanquished

Unvanquished is an arena game with RTS elements (you can build) in which two very different factions fight.

[![GitHub tag](https://img.shields.io/github/tag/Unvanquished/Unvanquished.svg)](https://github.com/Unvanquished/Unvanquished/tags)
[![GitHub release](https://img.shields.io/github/release/Unvanquished/Unvanquished.svg)](https://github.com/Unvanquished/Unvanquished/releases/latest)
[![Github universal zip release](https://img.shields.io/github/downloads/Unvanquished/Unvanquished/total.svg?label=zip%20downloads)](https://github.com/Unvanquished/Unvanquished/releases/latest)
[![Github updater release](https://img.shields.io/github/downloads/Unvanquished/updater/total.svg?label=updater%20downloads)](https://github.com/Unvanquished/updater/releases/latest)
[![Flatpak release](https://img.shields.io/flathub/downloads/net.unvanquished.Unvanquished.svg?label=flatpak%20installs)](https://flathub.org/apps/details/net.unvanquished.Unvanquished)
[![Sourceforge files](https://img.shields.io/sourceforge/dt/unvanquished.svg?label=sourceforge%20files)](https://sourceforge.net/projects/unvanquished/files/latest/download)

[![IRC](https://img.shields.io/badge/irc-%23unvanquished%2C%23unvanquished--dev-9cf.svg)](https://unvanquished.net/chat/)
[![Matrix](https://img.shields.io/badge/matrix-Unvanquished-9cf)](https://matrix.to/#/!WnuetRiQZJNBTKwMrF:matrix.org?via=matrix.org)
[![Discord](https://img.shields.io/badge/discord-Unvanquished-9cf)](https://discord.gg/usuDT9Pyna)

| Windows | macOS | Linux |
|---------|-----|-------|
| [![AppVeyor branch](https://img.shields.io/appveyor/ci/DolceTriade/unvanquished/master.svg)](https://ci.appveyor.com/project/DolceTriade/unvanquished/history) | [![Azure branch](https://img.shields.io/azure-devops/build/UnvanquishedDevelopment/8c34c73e-2b4f-43c9-b146-33aee7f3593a/2/master.svg)](https://dev.azure.com/UnvanquishedDevelopment/Unvanquished/_build?definitionId=2) | [![Azure branch](https://img.shields.io/azure-devops/build/UnvanquishedDevelopment/8c34c73e-2b4f-43c9-b146-33aee7f3593a/2/master.svg)](https://dev.azure.com/UnvanquishedDevelopment/Unvanquished/_build?definitionId=2) |

ℹ️ We provide ready-to-use downloads for the Unvanquished game on the [download page](https://unvanquished.net/download/).

ℹ️ This repository contains the source code for the game logic of Unvanquished.

💡️ If you only want to play the game or run a server and want to rebuild, you only need to build the [Dæmon engine](https://github.com/DaemonEngine/Daemon) and [download the game's assets](#downloading-the-games-assets).

💡️ You need to build this repository if you want to contribute to the game logic or make a mod with custom game logic binaries.

See below for [build](#building-the-game-binaries) and [launch](#running-the-game) instructions.

## Translating the game

[![Weblate](https://hosted.weblate.org/widgets/unvanquished/-/multi-auto.svg)](https://hosted.weblate.org/projects/unvanquished/unvanquished)

The Unvanquished game is translated on the [Weblate](https://hosted.weblate.org/projects/unvanquished/unvanquished) platform, this is the easier way to contribute translations since it offers a central place were people can contribute and review translations while reducing risks of merge conflicts. Weblate preserves authorship and automatically produces commits that can be associated with your GitHub account.

## Requirements to build the game binaries

To fetch and build Unvanquished, you'll need:
`git`,
`cmake`,
`python` ≥ 2,
`python-yaml`,
`python-jinja`,
and a C++14 compiler. The following are actively supported:
`gcc` ≥ 8,
`clang` ≥ 3.5,
Visual Studio/MSVC (at least Visual Studio 2019).

## Dependencies

Required:
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
`libopus`,
`libopusfile`.

Optional:
`ncurses`.

### MSYS2

MSYS2 is the recommended way to build using MinGW on a Windows host.

Required packages for 64-bit: `mingw-w64-x86_64-gcc`, `mingw-w64-x86_64-cmake`, `make`  
Required packages for 32-bit: `mingw-w64-i686-gcc`, `mingw-w64-i686-cmake`, `make`

See [the wiki](https://wiki.unvanquished.net/wiki/Compiling_the_source#MinGW) for more detailed instructions.

## Downloading the sources for the game binaries

Unvanquished requires several sub-repositories to be fetched before compilation. If you have not yet cloned this repository:

```sh
git clone --recurse-submodules https://github.com/Unvanquished/Unvanquished.git
```

If you have already cloned:

```sh
cd Unvanquished/
git submodule update --init --recursive
```

ℹ️ If cmake complains about missing files in `daemon/` folder or similar issue then you have skipped this step.

## Downloading the game's assets

If you don't have the assets, you can download them first. You can download an universal zip from the [download page](https://unvanquished.net/download/), here is the [link to the latest version](https://unvanquished.net/download/zip).
The `pkg/` folder can be stored in the [homepath](https://wiki.unvanquished.net/wiki/Homepath).

Otherwise you can use the package downloader script. This script can use `aria2c`, `curl` or `wget`.`aria2c` is recommended.

You can do `./download-paks --help` for more options.

```sh
./download-paks build/pkg
```

The above snippet places the `pkg` directory in the
[libpath](https://wiki.unvanquished.net/wiki/Libpath) of the Daemon engine that you are building,
so it will only be automatically found from this build directory. If you prefer the paks to be
found by from all installations/build directories, consider downloading to the `pkg/` subdirectory
of the [homepath](https://wiki.unvanquished.net/wiki/Homepath).

## Building the game binaries

ℹ️ This will build both the game engine and the game logic. Building both makes things more convenient for testing and debugging.

💡️ Instead of `-j4` you can use `-jN` where `N` is your number of CPU cores to distribute compilation on them. Linux systems usually provide a handy `nproc` tool that tells the number of CPU core so you can just do `-j$(nproc)` to use all available cores.

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
mkdir build; cd build
cmake ..
make -j4
```

### MSYS2

```sh
mkdir build; cd build
cmake .. -G "MSYS Makefiles"
make -j4
```

### Linux cross-compile to Windows

💡️ For a 32-bit build use the `cross-toolchain-mingw32.cmake` toolchain file instead.

```sh
mkdir build-win64; cd build-win64
cmake .. -DCMAKE_TOOLCHAIN_FILE=../daemon/cmake/cross-toolchain-mingw64.cmake
make -j4
```

## Running the game

ℹ️ On Windows you'll have to use `daemon.exe` and `daemonded.exe` instead of `./daemon` and `./daemonded`, everything else will be the same.

First, enter the `build/` folder:

```sh
cd build
```

### Running the client

Assuming the `pkg/` folder is stored next to the `daemon` binary:

```sh
./daemon
```

You can also use `-pakpath` to load assets from a `pkg/` folder stored somewhere else, `<PATH>` stands for the path to the `pkg/` folder.

```sh
./daemon -pakpath <PATH>
```

### Running the server

If you want to run a server, you may prefer to use the non-graphical `daemonded` server binary instead of `daemon` and start the first map (here, _plat23_ is used as an example) from the command line.

Assuming the `pkg/` folder is stored next to the `daemonded` binary:

```
./daemonded +map plat23
```

You can use `-pakpath` to load assets from a `pkg/` folder stored somewhere else (see above).

### Configuring the server

You may want to copy and extend some [configuration samples](dist/configs), they must be stored in your user home directory, for details see the [game locations](https://wiki.unvanquished.net/wiki/Game_locations) wiki page and the page about [running a server](https://wiki.unvanquished.net/wiki/Server/Running).

## Running a modified game as a developer

ℹ️ This is not a tutorial to distribute a modified game usable by players. See the wiki for mod
[building](https://wiki.unvanquished.net/wiki/Making_an_awesome_mod)
and [hosting](https://wiki.unvanquished.net/wiki/Hosting_one%27s_own_awesome_mod)
tutorials.

ℹ️ This is not needed to run the released game or a server.

As a developer, you may want to run the game logic as a shared library and load modified assets from the `pkg/unvanquished_src.dpkdir` submodule.

Assuming you already have a release `pkg/` folder, you can do it this way:

```sh
./daemon -pakpath ../pkg -set vm.sgame.type 3 -set vm.cgame.type 3
```

Note that only the basic `unvanquished_src.dpkdir` asset package is provided that way, and running Unvanquished only with that package will bring you some warnings about other missing packages and you will miss soundtrack and stuff like that.

ℹ️ If you are looking for the sources of the whole assets, have a look at the [UnvanquishedAssets](https://github.com/UnvanquishedAssets/UnvanquishedAssets) repository. Beware that unlike the `unvanquished_src.dpkdir` package most of them can't be loaded correctly by the engine without being built first.

## Dealing with submodules

Please don't commit submodules in pull requests, as the submodule references may not exist after the pull requests are rebased and merged.

When a feature is split across more than one repository, submodules are expected to be committed after each pull requests has been merged.

ℹ️ More details can be found in the wiki: [wiki:Dealing with submodules](https://wiki.unvanquished.net/wiki/Dealing_with_submodules).
