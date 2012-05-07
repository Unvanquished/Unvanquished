Daemon Engine
-------------


Dependencies
------------
    - Freetype
    - libpng
    - libjpeg (DO NOT USE VERSION 6)
    - libcurl
    - libsdl
    - OpenAL
    - libwebp (optional)
    - MySQL (optional)
    - libxvid (optional)
    - Newton (provided)
    - libgmp
    - GLEW

Build Instructions
------------------

    Visual Studio
    -------------
    Open Daemon.sln in src/engine/
    Pick which configuration you want to build.
    Click "Build"

    Linux  (CMake is required to build.)
    -----
    ccmake .
    Press 'c'
    Fill in the blanks for any libraries that you cannot find. 
    Press 'c' and then 'g'
    make
    (use `make -jN` where N is your number of CPU cores to speed up compilation)
    
    MINGW + MSYS
    ------------
    mkdir build && cd build (from Unvanquished root directory)
    cmake -G "MSYS Makefiles" -DCMAKE_PREFIX_PATH="C:\MINGW"  ..
    make

Run Instructions for Unvanquished
----------------------------------

Download all the asset packs from
http://www.sourceforge.net/projects/Unvanquished/files/Assets and any maps (the mappack from
http://sourceforge.net/projects/Unvanquished/files/Map%20Pack/maps.7z/download
is recommended) into main

Run with ./daemon.arch or Daemon.exe depending on OS.
