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

Run Instructions for Unvanquished
----------------------------------

Copy http://tremz.com/downloads/main/pak0.pk3 and any maps (the mappack from
http://sourceforge.net/projects/unvanquished/files/Map%20Pack/maps.7z/download
is recommended) into main

Run with ./daemon.arch or Daemon.exe depending on OS.
