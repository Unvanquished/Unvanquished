Daemon Engine
-------------


Dependencies
------------
    - Freetype
    - libpng
    - libjpeg (DO NOT USE VERSION 6)
    - libcurl
    - libsdl
    - OpenAL (optional)
    - libwebp
    - libxvid (optional)
    - Newton (provided)
    - libgmp
    - GLEW
    - theora
    - speex
    - speexdsp
    - nettle

Build Instructions
------------------

    Visual Studio (CMake is required to build
    -------------
    Execute either Visual_Studio32.bat or Visual_Studio64.bat
    Navigate to Unvanquished/build-32 or Unvanquished/build-64
    Open Daemon.sln

    Linux  (CMake is required to build.)
    -----
    mkdir build && cd build (from Unvanquished root directory)
    ccmake ..
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
http://www.sourceforge.net/projects/unvanquished/files/Assets and any maps (the mappack from
http://sourceforge.net/projects/unvanquished/files/Map%20Pack/maps.7z/download
is recommended) into main

Run with ./daemon.arch or Daemon.exe depending on OS.
