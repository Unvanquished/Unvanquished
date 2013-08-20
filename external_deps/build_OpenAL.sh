#!/bin/sh
# Create result directories
mkdir -p include
mkdir -p win32
mkdir -p win64

# Build OpenAL, but we cheat by using precompiled binaries instead
cd openal-soft-1.15.1-bin
cp -r include/AL ../include
cp lib/Win32/libOpenAL32.dll.a ../win32
cp lib/Win64/libOpenAL32.dll.a ../win64
