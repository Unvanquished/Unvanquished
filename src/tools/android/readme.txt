vs-android v0.93 - 13th November 2011
=====================================

vs-android is intended to provide a collection of scripts and utilities to support integrated development of
Android NDK C/C++ software under Microsoft Visual Studio.

Currently vs-android only works under Visual Studio 2010. Earlier versions lack the MSBuild integration with 
the C/C++ compilation systems.


Required Support SDKs
=====================

Cygwin is not required at all to use vs-android, thankfully!

At a bare minimum the Android NDK needs to be installed. This will allow compilation of C/C++ code:
* http://developer.android.com/sdk/ndk/index.html


In order to build an apk package to run on an Android device, you'll also require:

The Android SDK:
* http://developer.android.com/sdk/index.html

The Java JDK (the x86 version, *not* the x64 one!):
* http://www.oracle.com/technetwork/java/javase/downloads/index.html

Apache Ant:
* http://ant.apache.org/



Documentation
=============

Documentation for vs-android can be found here:
  * http://code.google.com/p/vs-android/



Version History
===============

v0.93 - 13th November 2011

  * NDK r7 was a breaking change for vs-android. This version now requires r7 or newer to be installed.
  * Fixed breaking changes to the location of STL libraries. Also fixed new linking issues introduced by STL changes.
  * Removed support for defunct arm 4.4.0 toolset.
  * Added support for android-14, Android API v4.0.
  * Added support for the dynamic (shared) version of the GNU libstdc++ STL.
  * Tested against newest JDK - jdk-7u1-windows-i586.
  * Added support for building assembly files. '.s' and '.asm' extensions will be treated as assembly code.
  * Correct passing of ANT_OPTS to the Ant Build step. Thanks to 'mark.bozeman'.
  * Corrected expected apk name for release builds.
  * Added to Ant Build property page; the ability to add extra flags to the calls to adb.
  * Fixed bug with arm arch preprocessor defines not making it onto the command line.
  * Fixed bad quote removal on paths, in the C# code. Thanks to 'hoesing@kleinbottling'.
  * Removed stlport project from Google Code - This was an oversight by Google in the r6 NDK, prebuilt is back again.
  * Updated sample projects to work with R15 SDK tools.


v0.92 - 3rd August 2011

  * Fixed jump-to-line clickable errors. Reworked code to use regular expressions instead. Tried a number 
    of different compiler/linker warnings and errors and all seems to be good
  * Default warning level is now 'Normal Warnings', instead of 'Disable All Warnings'. Whoops!
  * Fixed rtti-related warnings when compiling .c files with 'Enable All Warnings (-Wall)', turned on.


v0.91 - 2nd August 2011

  * Windows debugger is now usable. Fixed 'CLRSupport' error.
  * Error checking to ensure the 32-bit JDK is used.
  * Added JVM Heap options to Ant Build step. Initial and Maximum sizes are able to be set there now.
  * Added asafhel...@gmail's 'clickable errors from compiler' C# code.
  * Modified clickable errors code to also work with #include errors, which specify the column number too.
  * Added clickable error support to linker too.


v0.9 - 20th July 2011

  * Major update, hence the skip in numbers. Closing in on a v1.0 release.
  * Verified working with Android NDK r5b, r5c, and r6.
  * Much of vs-android functionality moved from MSBuild script to C# tasks. Similar approach now to Microsoft's
    existing Win32 setup.
  * Dependency checking rewritten to use tracking log files.
  * Dependency issues fixed, dependency checking also now far quicker.
  * Android Property sheets now completely replace the Microsoft ones, no more rafts of unused sheets.
  * Property sheets populated with many options. Switches are no longer hard-coded within vs-android script.
  * STL support added. Choice between 'None', 'Minimal', 'libstdc++', and 'stlport'.
  * Support for x86 compilation with r6 NDK.
  * Full support for v7-a arm architecture, as well as the existing v5.
  * Support for Android API directories other than just 'android-9'.
  * Separated support for 'dynamic libraries' and 'applications'. Applications build to apk files.
  * Response files used in build, no more command-line length limitations.
  * Deploy and run within Visual Studio, adb is now invoked by vs-android.
  * 'Echo command lines' feature fixed.
  * All support SDK/libs (NDK, SDK, Ant, JDK) are okay living in directories with spaces in them now.
  * All bugs logged within Google Code are addressed.


v0.21 - 10th Feb 2011

  * Fixed issues with the 'ant build' step.
  * Added a sensible error message if the NDK envvar isn't set, or is set incorrectly.


v0.2 - 1st Feb 2011

  * Changed default preprocessor symbols to work the same way Microsoft's stuff does. Should fix any 
    issues with intellisense as well.
  * Added support for scanning header dependencies


v0.1 - 30th Jan 2011

  * Initial version.
  * All major functionality present, barring header dependency checking. 



Contributors
============

asafhel...@gmail.com - Initial 'clickable errors from compiler' C# code.
mark.bozeman - Fix for Ant Build to correctly pass ANT_OPTS.
hoesing@kleinbottling - Fix for the bad quote removal on paths, in the C# code.



References
==========

"Inside the Microsoft Build Engine: Using MSBuild and Team Foundation Build"
Authors: Sayed Ibrahim Hashimi, William Bartholomew

"Microsoft C/C++ Visual Studio 2010 Build Implementation"
Located here: %ProgramFiles(x86)%\MSBuild\Microsoft.Cpp



License
=======

vs-android is released under the zlib license.
http://en.wikipedia.org/wiki/Zlib_License



Copyright (c) 2011 Gavin Pugh http://www.gavpugh.com/

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

   1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.

   2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

   3. This notice may not be removed or altered from any source
   distribution.
