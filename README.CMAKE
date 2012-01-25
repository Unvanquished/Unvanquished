In order to build with CMake on Linux, do the following:

    In the build directory:

    1) Type "ccmake .", and press "c"
    2) If you do not have libwebp, disable that.
    3) Look for MYSQL_LIBRARY. Enter the path to your mysql library (normally
    found in /usr/lib/libmysqlclient.so)
    4) Press "t" to toggle advanced options, and specify the location of
    libGLEW.so (normally found in /usr/lib/libGLEW.so).
    5) Press "g" to generate the Makefile.
    6) Type "make" to compile
    7) Enjoy :)

