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
    Open Daemon.sln in src/engine/
    Pick which configuration you want to build.
    Click "Build"

    Linux  (CMake is required to build.)
    -----
    ccmake .
    Press 'c'
    Fill in the blanks for GLEW, MySQL, libgmp, and libxvid
    Press 'c' and then 'g'
    make
    (use `make -jN` where N is your number of CPU cores to speed up compilation)

Run Instructions for Tremulous GPP
----------------------------------

Copy http://tremz.com/downloads/main/pak0.pk3 and any maps (the mappack from
http://sourceforge.net/projects/unvanquished/files/Map%20Pack/maps.7z/download
is recommended) into main
Put the latest GPP data pack in the 'gpp' folder ( current one can be found at http://downloads.mercenariesguild.net/gpp/data-gppr2223.pk3 )
Copy 'ui' from src/gamelogic/gpp into the 'gpp' folder


    
