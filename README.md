# Unvanquished

Unvanquished is an arena game with RTS elements (you can build) in which two very different factions fight.

[![GitHub release](https://img.shields.io/github/release/Unvanquished/Unvanquished.svg)](https://github.com/Unvanquished/Unvanquished/releases/latest)
[![Github updater release](https://img.shields.io/github/downloads/Unvanquished/updater/total.svg?label=updater%20downloads)](https://github.com/Unvanquished/updater/releases/latest)
[![Flatpak release](https://img.shields.io/flathub/downloads/net.unvanquished.Unvanquished.svg?label=flatpak%20installs)](https://flathub.org/apps/details/net.unvanquished.Unvanquished)
[![Github universal zip release](https://img.shields.io/github/downloads/Unvanquished/Unvanquished/total.svg?label=zip%20downloads)](https://github.com/Unvanquished/Unvanquished/releases/latest)
[![Sourceforge files](https://img.shields.io/sourceforge/dt/unvanquished.svg?label=sourceforge%20files)](https://sourceforge.net/projects/unvanquished/files/latest/download)

[![IRC](https://img.shields.io/badge/irc-%23unvanquished%2C%23unvanquished--dev-9cf.svg)](https://unvanquished.net/chat/)
[![Matrix](https://img.shields.io/badge/matrix-Unvanquished-9cf)](https://matrix.to/#/!WnuetRiQZJNBTKwMrF:matrix.org?via=matrix.org)
[![Discord](https://img.shields.io/badge/discord-Unvanquished-9cf)](https://discord.gg/usuDT9Pyna)

| Windows | macOS | Linux |
|---------|-----|-------|
| [![AppVeyor branch](https://img.shields.io/appveyor/ci/UnvanquishedDevelopment/unvanquished/master.svg)](https://ci.appveyor.com/project/UnvanquishedDevelopment/unvanquished/history) | [![Azure branch](https://img.shields.io/azure-devops/build/UnvanquishedDevelopment/8c34c73e-2b4f-43c9-b146-33aee7f3593a/2/master.svg)](https://dev.azure.com/UnvanquishedDevelopment/Unvanquished/_build?definitionId=2) | [![Azure branch](https://img.shields.io/azure-devops/build/UnvanquishedDevelopment/8c34c73e-2b4f-43c9-b146-33aee7f3593a/2/master.svg)](https://dev.azure.com/UnvanquishedDevelopment/Unvanquished/_build?definitionId=2) |

ℹ️ We provide ready-to-use downloads for the Unvanquished game on the [download page](https://unvanquished.net/download/).

ℹ️ This repository contains the source code for the game logic of Unvanquished.

💡️ If you only want to play the game or run a server and want to rebuild, you only need to build the [Dæmon engine](https://github.com/DaemonEngine/Daemon) and [download the game's assets](#downloading-the-games-assets).

💡️ You need to build this repository if you want to contribute to the game logic or make a mod with custom game logic binaries.

See below for [build](#building-the-game-binaries) and [launch](#running-the-game) instructions.

If you are interested in creating or using Unvanquished packages for package managers, see
[wiki:Linux packages](https://wiki.unvanquished.net/wiki/Linux_packages).

## Translating the game

[![Weblate](https://hosted.weblate.org/widgets/unvanquished/-/multi-auto.svg)](https://hosted.weblate.org/projects/unvanquished/unvanquished)

The Unvanquished game is translated on the [Weblate](https://hosted.weblate.org/projects/unvanquished/unvanquished) platform, this is the easier way to contribute translations since it offers a central place where people can contribute and review translations while reducing risks of merge conflicts. Weblate preserves authorship and automatically produces commits that can be associated with your GitHub account.

## Requirements to build the game binaries

Detailed instructions for compiling the sources can be found on the wiki: [wiki:Compiling_the_source](https://wiki.unvanquished.net/wiki/Compiling_the_source).

To fetch and build Unvanquished, you'll need:
`git`,
`cmake`,
`python` ≥ 2,
`python-yaml`,
`python-jinja`,
and a C++14 compiler. The following are actively supported:
`gcc` ≥ 9,
`clang` ≥ 11,
Visual Studio/MSVC (at least Visual Studio 2019).

## Dependencies

Required:
`zlib`,
`libgmp`,
`libnettle`,
`libcurl`,
`SDL3`,
`GLEW`,
`libpng`,
`libjpeg` ≥ 8,
`libwebp` ≥ 0.2.0,
`OpenAL`,
`libogg`,
`libvorbis`,
`libopus`,
`libopusfile`.

Optional:
`Freetype` (submodule will be used if not available),
`Lua` (submodule will be used if not available),
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


## 🌐 Web Resources & Interactive Index
- [SNAP FIX](https://quizverses-9d2f2.web.app/snap-fix.html)
- [MERGE CUBES 2048 3D](https://learnquester.github.io/merge-cubes-2048-3d.html)
- [CATEGORY SIMULATION 3](https://theskillquest.pages.dev/category-simulation-3.html)
- [4 COLORS CARD MANIA](https://thelearnquesters.pages.dev/4-colors-card-mania.html)
- [WONDERS OF EGYPT MAHJONG](https://themindskillplayplay.pages.dev/wonders-of-egypt-mahjong.html)
- [MY DINOSAUR LAND](https://thelearnquesters.pages.dev/my-dinosaur-land.html)
- [SLINGSHOT FORTRESS](https://iskillplay.web.app/slingshot-fortress.html)
- [MATCH MASTERS](https://skillplay.github.io/match-masters.html)
- [CHAOS ROAD COMBAT CAR RACING](https://studyplayings.pages.dev/chaos-road-combat-car-racing.html)
- [CATEGORY CASUAL 13](https://themindzone.pages.dev/category-casual-13.html)
- [LULUS FASHION WORLD](https://thequizzone.pages.dev/lulus-fashion-world.html)
- [STICK ROPE HERO](https://themindskillplayplay.pages.dev/stick-rope-hero.html)
- [CATEGORY LOVE12](https://thelearnquester.web.app/category-love12.html)
- [CATEGORY ARMY40](https://theskillquest.pages.dev/category-army40.html)
- [WATER SHOOTER](https://themindskillplayplay.pages.dev/water-shooter.html)
- [DROP BRICKS BREAKER](https://thelearnquesters.pages.dev/drop-bricks-breaker.html)
- [WORD CLASH](https://studyquests.github.io/word-clash.html)
- [ELEMENTZ](https://quizverses.pages.dev/elementz.html)
- [COLOR NONOGRAM PUZZLE 2](https://thelearnquesters.pages.dev/color-nonogram-puzzle-2.html)
- [FEED ME MONSTERS IDLE BATTLE](https://thelearnquester.web.app/feed-me-monsters-idle-battle.html)
- [TRI PEAKS EMERLAND SOLITAIRE](https://thelearnquester.web.app/tri-peaks-emerland-solitaire.html)
- [CATEGORY CASUAL 8](https://learnquesters.pages.dev/category-casual-8.html)
- [CATEGORY TOP DOWN251](https://quizverses.pages.dev/category-top-down251.html)
- [CATEGORY SNAKE GAMES](https://iskillplay.web.app/category-snake-games.html)
- [PUZZLE BLOCKS](https://thequizzone.pages.dev/puzzle-blocks.html)
- [OKAY](https://quizverses-9d2f2.web.app/okay.html)
- [IMPOSTER COLORING BOOK](https://thequizzone.pages.dev/imposter-coloring-book.html)
- [GARTEN OF BANBAN 1 ESCAPE](https://quizverses-9d2f2.web.app/garten-of-banban-1-escape.html)
- [GRENADE SIMULATOR](https://thelearnquester.web.app/grenade-simulator.html)
- [INDYGIRL AND THE GOLDEN SKULL](https://skillplay.github.io/indygirl-and-the-golden-skull.html)
- [MY HAPPY FARM](https://quizverses-9d2f2.web.app/my-happy-farm.html)
- [LIFE CLICKER](https://thequizzone.pages.dev/life-clicker.html)
- [SEA MATCH](https://quizverses-9d2f2.web.app/sea-match.html)
- [CLASH RUN](https://themindskillplayplay.pages.dev/clash-run.html)
- [MMA SUPER FIGHT](https://thelearnquester.web.app/mma-super-fight.html)
- [COIN MERGE MACHINE](https://thequizzone.pages.dev/coin-merge-machine.html)
- [BUTTERFLY SORT PUZZLE](https://learnquester.pages.dev/butterfly-sort-puzzle.html)
- [OBBY PUMP UP YOUR MUSCLES 1 PER SECOND](https://thelearnquesters.pages.dev/obby-pump-up-your-muscles-1-per-second.html)
- [FORMULA RACING GAMES CAR GAME](https://thequizzone.pages.dev/formula-racing-games-car-game.html)
- [DUNGEON MASTER CULT CRAFT](https://quizverses-9d2f2.web.app/dungeon-master-cult-craft.html)
- [DUO FAMILY SANTA](https://themindzone.pages.dev/duo-family-santa.html)
- [BATTLESHIP](https://quizverses-9d2f2.web.app/battleship.html)
- [CYBERPUNK CORPORATION](https://themindzone.pages.dev/cyberpunk-corporation.html)
- [K POP HUNTER FASHION](https://themindskillplayplay.pages.dev/k-pop-hunter-fashion.html)
- [CATEGORY AGILITY 3](https://themindplay.pages.dev/category-agility-3.html)
- [CATEGORY 3D1 371](https://iskillplay.web.app/category-3d1-371.html)
- [MAHJONG CLASSIC](https://learnquester.pages.dev/mahjong-classic.html)
- [SPRUNKI QUIZ](https://themindskillplayplay.pages.dev/sprunki-quiz.html)
- [JUMPER](https://themindzone.pages.dev/jumper.html)
- [DIRTY MONEY THE RICH GET RICH](https://quizverses-9d2f2.web.app/dirty-money-the-rich-get-rich.html)
- [DEADLY PARKOUR](https://learnquester.pages.dev/deadly-parkour.html)
- [CATEGORY DRAWING34](https://thelearnquester.web.app/category-drawing34.html)
- [DARTS JAM](https://iskillplay.web.app/darts-jam.html)
- [BOLTS UNSCREW IT](https://skillplay.github.io/bolts-unscrew-it.html)
- [INDEX29](https://theskillquest.pages.dev/index29.html)
- [CHARGER CITY DRIVER](https://iskillplay.web.app/charger-city-driver.html)
- [CATEGORY MERGE GAMES](https://thelearnquester.web.app/category-merge-games.html)
- [SPOOKY CHAINS](https://iskillplay.web.app/spooky-chains.html)
- [HIGHWAY BUS RUSH](https://themindzone.pages.dev/highway-bus-rush.html)
- [STRIKE IT](https://quizverses-9d2f2.web.app/strike-it.html)
- [GIANT CROWD IO HOUSE CAPTURE](https://themindplay.pages.dev/giant-crowd-io-house-capture.html)
- [CATEGORY EXPLOIT](https://learnquester.pages.dev/category-exploit.html)
- [THE SUPERHERO LEAGUE](https://thequizzone.pages.dev/the-superhero-league.html)
- [INDEX2](https://iskillplay.web.app/index2.html)
- [DUNK CHALLENGE](https://quizverses-9d2f2.web.app/dunk-challenge.html)
- [BRAINROT BRIDGE RACE 3D](https://learnquester.pages.dev/brainrot-bridge-race-3d.html)
- [POP PUZZLE](https://quizverses-9d2f2.web.app/pop-puzzle.html)
- [CS COMMAND SNIPERS](https://themindskillplayplay.pages.dev/cs-command-snipers.html)
- [CATEGORY RUNNING](https://themindplay.pages.dev/category-running.html)
- [BUS JAM ESCAPE](https://quizverses-9d2f2.web.app/bus-jam-escape.html)
- [CATEGORY THINKY 3](https://iskillplay.web.app/category-thinky-3.html)
- [MR BULLET STEALTH NINJA KILLSTREAK](https://themindskillplayplay.pages.dev/mr-bullet-stealth-ninja-killstreak.html)
- [MAZE ESCAPE CHALLENGE](https://themindskillplayplay.pages.dev/maze-escape-challenge.html)
- [TERMS](https://learnquesters.pages.dev/terms.html)
- [GUESS THE DRAWING](https://quizverses-9d2f2.web.app/guess-the-drawing.html)
- [BRAINROT ICE TRUCK](https://themindskillplayplay.pages.dev/brainrot-ice-truck.html)
- [INDEX35](https://themindskillplayplay.pages.dev/index35.html)
- [SAVE BABY CAPYBARAS PULL PIN](https://quizverses-9d2f2.web.app/save-baby-capybaras-pull-pin.html)
- [PRINXY WINTERELLA](https://themindplay.github.io/prinxy-winterella.html)
- [ARROW SURVIVAL 15 SECONDS](https://iskillplay.web.app/arrow-survival-15-seconds.html)
- [LOVE IN STYLE](https://quizverses-9d2f2.web.app/love-in-style.html)
- [THE BODYGUARD](https://iskillplay.web.app/the-bodyguard.html)
- [OCEAN POP](https://themindplay.pages.dev/ocean-pop.html)
- [SUGAR HEROES](https://iskillplay.web.app/sugar-heroes.html)
- [CATEGORY PIXEL313](https://quizverses.pages.dev/category-pixel313.html)
- [SPACE SHIFT](https://thelearnquesters.pages.dev/space-shift.html)
- [SUPER SWING](https://thelearnquesters.pages.dev/super-swing.html)
- [INDEX15](https://learnquesters.pages.dev/index15.html)
- [FOOD SORT 3D](https://thelearnquesters.pages.dev/food-sort-3d.html)
- [CATEGORY MINING75](https://thelearnquester.web.app/category-mining75.html)
- [MACHINE CITY BALLS](https://iskillplay.web.app/machine-city-balls.html)
- [CATEGORY LOGIC538](https://learnquester.pages.dev/category-logic538.html)
- [THE PATAGONIANS](https://iskillplay.web.app/the-patagonians.html)
- [CATEGORY RPG80](https://quizverses.pages.dev/category-rpg80.html)
- [FLAG MASTER WORLD FLAGS QUIZ](https://quizverses-9d2f2.web.app/flag-master-world-flags-quiz.html)
- [DIRTY MONEY THE RICH GET RICH](https://iskillplay.web.app/dirty-money-the-rich-get-rich.html)
- [CATEGORY IDLE445](https://quizverses.pages.dev/category-idle445.html)
- [BOLTS](https://skillplay.github.io/bolts.html)
- [CATEGORY POINT AND CLICK](https://themindplay.github.io/category-point-and-click.html)
- [CATEGORY RAMMERHEAD](https://learnquester.pages.dev/category-rammerhead.html)
- [HAMMER MASTERCRAFT DESTROY](https://iskillplay.web.app/hammer-mastercraft-destroy.html)
- [ITALIAN BRAINROT QUIZ](https://learnquester.github.io/italian-brainrot-quiz.html)
- [JEWEL GARDEN STORY](https://iskillplay.web.app/jewel-garden-story.html)
- [IMPOSTER 3D](https://learnquesters.pages.dev/imposter-3d.html)
- [STICKMAN JUMP](https://quizverses-9d2f2.web.app/stickman-jump.html)
- [CUBE SPEED DASH](https://themindplay.github.io/cube-speed-dash.html)
- [APPLE WORM](https://quizverses-9d2f2.web.app/apple-worm.html)
- [CATEGORY FOOD](https://quizverses.pages.dev/category-food.html)
- [ANIME COUPLE AVATAR MAKER](https://quizverses-9d2f2.web.app/anime-couple-avatar-maker.html)
- [MINICRAFT CHEF CAKE WARS](https://thelearnquesters.pages.dev/minicraft-chef-cake-wars.html)
- [STICKER PUZZLE BOOK](https://themindskillplayplay.pages.dev/sticker-puzzle-book.html)
- [TANK SNIPER 3D](https://iskillquest.pages.dev/tank-sniper-3d.html)
- [STRAWBERRY SHORTCAKE](https://iskillplay.web.app/strawberry-shortcake.html)
- [CUBE DROP PUZZLE](https://quizverses-9d2f2.web.app/cube-drop-puzzle.html)
- [MERGE BEASTS](https://thequizzone.pages.dev/merge-beasts.html)
- [WILD WEST MATCH](https://thequizzone.pages.dev/wild-west-match.html)
- [BAD EGG](https://thequizzone.pages.dev/bad-egg.html)
- [CATEGORY FLASH 2](https://quizverses.pages.dev/category-flash-2.html)
- [CATEGORY RACING127](https://themindplay.pages.dev/category-racing127.html)
- [QUEEN OF MAHJONG](https://thelearnquesters.pages.dev/queen-of-mahjong.html)
- [CATEGORY FREE DRESS UP GAMES](https://quizverses.pages.dev/category-free-dress-up-games.html)
- [CATEGORY MERGE224](https://quizverses.pages.dev/category-merge224.html)
- [CATEGORY IDLE CLICKER GAME](https://quizverses.pages.dev/category-idle-clicker-game.html)
- [SPIDER NOOB OBSTACLE COURSE](https://themindzone.pages.dev/spider-noob-obstacle-course.html)
- [DIGITAL AQUA](https://quizverses-9d2f2.web.app/digital-aqua.html)
- [KOMPOTS KITCHEN](https://themindplay.pages.dev/kompots-kitchen.html)
- [PRIVACY](https://themindskillplayplay.pages.dev/privacy.html)
- [INDEX16](https://theskillquest.pages.dev/index16.html)
- [GOAL IO](https://thelearnquesters.pages.dev/goal-io.html)
- [CATEGORY THINKY](https://iskillplay.web.app/category-thinky.html)
