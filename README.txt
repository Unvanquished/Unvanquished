Daemon Engine
-------------


Dependencies
------------
    - Freetype
    - libpng
    - libjpeg
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
    Open OpenWolf.sln in src/engine/
    Click "Build"

    Linux  (CMake is required to build.)
    -----
    mkdir build && cd build
    ccmake ..
    Press 'c'
    Fill in the blanks for GLEW, MySQL, libgmp, and libxvid
    Press 'c' and then 'g'
    make

Run Instructions for Tremulous GPP
----------------------------------

Copy the 'main' folder from the root source directory into the build directory.
Copy 'data-1.1.0.pk3' and any maps into main as well.
Put the latest GPP data pack in the 'gpp' folder ( current one can be found at http://downloads.mercenariesguild.net/gpp/data-gppr2223.pk3 )
Copy 'ui' from src/gamelogic/gpp into the 'gpp' folder in the build directory
Run with +set fs_game gpp


    
