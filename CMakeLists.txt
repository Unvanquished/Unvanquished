#
#  CMakeLists.txt
#
# cmake required (http://www.cmake.org/)
#
cmake_minimum_required( VERSION 2.8 )
set( CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
set( CMAKE_C_FLAGS "-msse2 -pipe" CACHE STRING "" )
set( CMAKE_C_FLAGS_DEBUG "-g" CACHE STRING "" )
set( CMAKE_C_FLAGS_MINSIZEREL "-DNDEBUG -Os" CACHE STRING "" )
set( CMAKE_C_FLAGS_RELEASE "-DNDEBUG -O3 -ffast-math -fomit-frame-pointer -fno-strict-aliasing" CACHE STRING "" )
set( CMAKE_C_FLAGS_RELWITHDEBINFO "-DNDEBUG -O3 -ffast-math -fomit-frame-pointer -fno-strict-aliasing -g" CACHE STRING "" )
set( CMAKE_CXX_FLAGS ${CMAKE_C_FLAGS} CACHE STRING "" )
set( CMAKE_CXX_FLAGS_DEBUG ${CMAKE_C_FLAGS_DEBUG} CACHE STRING "" )
set( CMAKE_CXX_FLAGS_MINSIZEREL ${CMAKE_C_FLAGS_MINSIZEREL} CACHE STRING "" )
set( CMAKE_CXX_FLAGS_RELEASE ${CMAKE_C_FLAGS_RELEASE} CACHE STRING "" )
set( CMAKE_CXX_FLAGS_RELWITHDEBINFO ${CMAKE_C_FLAGS_RELWITHDEBINFO} CACHE STRING "" )
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)
set(CMAKE_BUILD_TYPE Release CACHE STRING
	"Build configuration (Debug, Release, RelWithDebInfo, MinSizeRel)")

#this line has to appear before 'PROJECT' in order to be able to disable incremental linking
SET(MSVC_INCREMENTAL_DEFAULT ON)

project( Daemon C CXX ASM )
set(CMAKE_ASM_COMPILER ${CMAKE_C_COMPILER})
set(CMAKE_ASM_COMPILE_OBJECT ${CMAKE_C_COMPILE_OBJECT})
set( CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "" ) # Need this to get rid of -rdynamic linker flag set by default by cmake

# options
option( BUILD_CLIENT         "Build Daemon client"                   1 )
option( BUILD_OLD_RENDERER   "Build old renderer library"              1 )
option( BUILD_NEW_RENDERER   "Build new renderer library"              1 )
option( BUILD_SERVER         "Build Daemonded server"                1 )
option( BUILD_GAME_SO_ET     "Build game logic shared libraries for ET" 0 )
option( BUILD_GAME_SO_TREM   "Build game logic shared libs for Trem"   0 )
option( BUILD_GAME_SO_GPP    "Build GPP Tremulous game logic libs"     1 )
option( BUILD_MASTER         "Build master server"                     1 )
option( BUILD_OWMAP          "Build Mapping tool"                      0 )
option( USE_OPENAL           "Use OpenAL"                              1 )
if( USE_OPENAL )
  option( USE_OPENAL_DLOPEN  "Use dlopen to load OpenAL"               1 )
endif()
option( USE_CURL             "Use curl"                                1 )
if( USE_CURL )
  option( USE_CURL_DLOPEN    "Use dlopen to load curl"                 1 )
option( USE_LLVM             "Use LLVM"                                0 ) 
endif()
if( WIN32 )
  option( USE_SYSTEM_HEADERS "Use local headers"                       0 )
else()
  option( USE_SYSTEM_HEADERS "Use local headers"                       1 )
endif()
option( USE_CPUINFO    "Use CPUInfo (if compile fails, disable this"   1 ) 
option( USE_INTERNAL_ZLIB    "Use internal zlib"                       0 )
option( USE_CURSES           "Use the curses console"                  1 )
option( USE_CODEC_VORBIS     "Compile with support for Vorbis"         1 )
if( USE_CODEC_VORBIS )
  option( USE_CIN_THEORA     "Compile with support for Theora"         1 )
endif()
option( USE_MUMBLE           "Compile with support for Mumble"         1 )
option( USE_PHYSICS	     "Compile with Newton Physics"             0 )
option( USE_WEBP	     "Compile with WebP loader"                1 )
option( USE_MYSQL	     "Compile with MYSQL support"              0 )
option( USE_VOIP	     "Compile with VOIP support"               1 )
option( USE_CIN_XVID	     "Encode videos with XViD"                 1 )

# package info
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Daemon Unvanquished client")
set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README.txt")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/GPL.txt")
include(CPack)

set( MOUNT_DIR   ${CMAKE_SOURCE_DIR}/src )
set( GAME_DIR	 ${CMAKE_SOURCE_DIR}/src/gamelogic/etmain )
set( BASE_DIR	 ${CMAKE_SOURCE_DIR}/src/gamelogic/base )
set( GPP_DIR	 ${CMAKE_SOURCE_DIR}/src/gamelogic/gpp )
set( LIB_DIR     ${CMAKE_SOURCE_DIR}/src/libs )
find_package(PkgConfig)
# 
 if( WIN32 )
   add_definitions( -D_CRT_SECURE_NO_WARNINGS )
 endif()

set(CONSOLE "tty")
if(WIN32)
	set(PLATFORM "win32")
	set(CONSOLE "win32")
elseif(UNIX)
  if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    add_definitions( -DMACOS_X -DMACOS_X_ACCELERATION_HACK -D USE_FILE32API -DMACOS_X_CURSOR_HACK )
  endif()
  set(PLATFORM "unix")
endif()

#Determine arch
if( ${CMAKE_SYSTEM_PROCESSOR} MATCHES "x86_64" )
  set( BUILD_ARCH "x86_64" )
elseif( ${CMAKE_SYSTEM_PROCESSOR} MATCHES "i.86" )
  set( BUILD_ARCH "i386" )
else()
  set( BUILD_ARCH ${CMAKE_SYSTEM_PROCESSOR} )
endif()

######################
# Define Build Files #
######################

set( JPEGLIST
  ${MOUNT_DIR}/libs/jpeg/jcapimin.c
  ${MOUNT_DIR}/libs/jpeg/jchuff.c
  ${MOUNT_DIR}/libs/jpeg/jcinit.c
  ${MOUNT_DIR}/libs/jpeg/jccoefct.c
  ${MOUNT_DIR}/libs/jpeg/jccolor.c
  ${MOUNT_DIR}/libs/jpeg/jfdctflt.c
  ${MOUNT_DIR}/libs/jpeg/jcdctmgr.c
  ${MOUNT_DIR}/libs/jpeg/jcmainct.c
  ${MOUNT_DIR}/libs/jpeg/jcmarker.c
  ${MOUNT_DIR}/libs/jpeg/jcmaster.c
  ${MOUNT_DIR}/libs/jpeg/jcomapi.c
  ${MOUNT_DIR}/libs/jpeg/jcparam.c
  ${MOUNT_DIR}/libs/jpeg/jcprepct.c
  ${MOUNT_DIR}/libs/jpeg/jcsample.c
  ${MOUNT_DIR}/libs/jpeg/jdapimin.c
  ${MOUNT_DIR}/libs/jpeg/jdapistd.c
  ${MOUNT_DIR}/libs/jpeg/jdatasrc.c
  ${MOUNT_DIR}/libs/jpeg/jdcoefct.c
  ${MOUNT_DIR}/libs/jpeg/jdcolor.c
  ${MOUNT_DIR}/libs/jpeg/jddctmgr.c
  ${MOUNT_DIR}/libs/jpeg/jdhuff.c
  ${MOUNT_DIR}/libs/jpeg/jdinput.c
  ${MOUNT_DIR}/libs/jpeg/jdmainct.c
  ${MOUNT_DIR}/libs/jpeg/jdmarker.c
  ${MOUNT_DIR}/libs/jpeg/jdmaster.c
  ${MOUNT_DIR}/libs/jpeg/jdpostct.c
  ${MOUNT_DIR}/libs/jpeg/jdsample.c
  ${MOUNT_DIR}/libs/jpeg/jdtrans.c
  ${MOUNT_DIR}/libs/jpeg/jerror.c
  ${MOUNT_DIR}/libs/jpeg/jidctflt.c
  ${MOUNT_DIR}/libs/jpeg/jmemmgr.c
  ${MOUNT_DIR}/libs/jpeg/jmemnobs.c
  ${MOUNT_DIR}/libs/jpeg/jutils.c
)

set( PNGLIST
  ${MOUNT_DIR}/libs/libpng/png.c
  ${MOUNT_DIR}/libs/libpng/pngset.c
  ${MOUNT_DIR}/libs/libpng/pngget.c
  ${MOUNT_DIR}/libs/libpng/pngrutil.c
  ${MOUNT_DIR}/libs/libpng/pngtrans.c
  ${MOUNT_DIR}/libs/libpng/pngwutil.c
  ${MOUNT_DIR}/libs/libpng/pngread.c
  ${MOUNT_DIR}/libs/libpng/pngrio.c
  ${MOUNT_DIR}/libs/libpng/pngwio.c
  ${MOUNT_DIR}/libs/libpng/pngwrite.c
  ${MOUNT_DIR}/libs/libpng/pngrtran.c
  ${MOUNT_DIR}/libs/libpng/pngwtran.c
  ${MOUNT_DIR}/libs/libpng/pngmem.c
  ${MOUNT_DIR}/libs/libpng/pngerror.c
  ${MOUNT_DIR}/libs/libpng/pngpread.c
)

set( ZLIBLIST
  ${MOUNT_DIR}/libs/zlib/adler32.c
  ${MOUNT_DIR}/libs/zlib/compress.c
  ${MOUNT_DIR}/libs/zlib/crc32.c
  ${MOUNT_DIR}/libs/zlib/gzio.c
  ${MOUNT_DIR}/libs/zlib/uncompr.c
  ${MOUNT_DIR}/libs/zlib/deflate.c
  ${MOUNT_DIR}/libs/zlib/trees.c
  ${MOUNT_DIR}/libs/zlib/zutil.c
  ${MOUNT_DIR}/libs/zlib/inflate.c
  ${MOUNT_DIR}/libs/zlib/infback.c
  ${MOUNT_DIR}/libs/zlib/inftrees.c
  ${MOUNT_DIR}/libs/zlib/inffast.c
)

set( OGGLIST
  ${MOUNT_DIR}/libs/ogg/src/bitwise.c
  ${MOUNT_DIR}/libs/ogg/src/framing.c
)

set( VORBISLIST
  ${MOUNT_DIR}/libs/vorbis/lib/analysis.c 
  ${MOUNT_DIR}/libs/vorbis/lib/bitrate.c
  ${MOUNT_DIR}/libs/vorbis/lib/codebook.c
  ${MOUNT_DIR}/libs/vorbis/lib/floor0.c
  ${MOUNT_DIR}/libs/vorbis/lib/info.c
  ${MOUNT_DIR}/libs/vorbis/lib/lpc.c
  ${MOUNT_DIR}/libs/vorbis/lib/mapping0.c
  ${MOUNT_DIR}/libs/vorbis/lib/psy.c
  ${MOUNT_DIR}/libs/vorbis/lib/registry.c
  ${MOUNT_DIR}/libs/vorbis/lib/sharedbook.c
  ${MOUNT_DIR}/libs/vorbis/lib/synthesis.c
  ${MOUNT_DIR}/libs/vorbis/lib/vorbisenc.c
  ${MOUNT_DIR}/libs/vorbis/lib/window.c
  #${MOUNT_DIR}/libs/vorbis/lib/barkmel.c
  ${MOUNT_DIR}/libs/vorbis/lib/block.c
  ${MOUNT_DIR}/libs/vorbis/lib/envelope.c
  ${MOUNT_DIR}/libs/vorbis/lib/floor1.c
  ${MOUNT_DIR}/libs/vorbis/lib/lookup.c
  ${MOUNT_DIR}/libs/vorbis/lib/lsp.c
  ${MOUNT_DIR}/libs/vorbis/lib/mdct.c
  #${MOUNT_DIR}/libs/vorbis/lib/psytune.c
  ${MOUNT_DIR}/libs/vorbis/lib/res0.c
  ${MOUNT_DIR}/libs/vorbis/lib/smallft.c
  #${MOUNT_DIR}/libs/vorbis/lib/tone.c
  ${MOUNT_DIR}/libs/vorbis/lib/vorbisfile.c
)

set( THEORALIST
  ${MOUNT_DIR}/libs/theora/lib/dec/fragment.c
  ${MOUNT_DIR}/libs/theora/lib/dec/decinfo.c
  ${MOUNT_DIR}/libs/theora/lib/dec/internal.c
  ${MOUNT_DIR}/libs/theora/lib/dec/decode.c
  ${MOUNT_DIR}/libs/theora/lib/dec/state.c
  ${MOUNT_DIR}/libs/theora/lib/dec/bitpack.c
  ${MOUNT_DIR}/libs/theora/lib/dec/quant.c
  ${MOUNT_DIR}/libs/theora/lib/dec/x86/mmxstate.c
  ${MOUNT_DIR}/libs/theora/lib/dec/x86/x86state.c
  ${MOUNT_DIR}/libs/theora/lib/dec/x86/mmxidct.c
  ${MOUNT_DIR}/libs/theora/lib/dec/x86/mmxfrag.c
  ${MOUNT_DIR}/libs/theora/lib/dec/huffdec.c
  ${MOUNT_DIR}/libs/theora/lib/dec/thinfo.c
  ${MOUNT_DIR}/libs/theora/lib/dec/x86_vc/mmxstate.c
  ${MOUNT_DIR}/libs/theora/lib/dec/x86_vc/x86state.c
  ${MOUNT_DIR}/libs/theora/lib/dec/x86_vc/mmxidct.c
  ${MOUNT_DIR}/libs/theora/lib/dec/x86_vc/mmxfrag.c
  ${MOUNT_DIR}/libs/theora/lib/dec/x86_vc/mmxloopfilter.c
  ${MOUNT_DIR}/libs/theora/lib/dec/idct.c
  ${MOUNT_DIR}/libs/theora/lib/dec/decapiwrapper.c
  ${MOUNT_DIR}/libs/theora/lib/dec/apiwrapper.c
  ${MOUNT_DIR}/libs/theora/lib/dec/dequant.c
  ${MOUNT_DIR}/libs/theora/lib/enc/pb.c
  ${MOUNT_DIR}/libs/theora/lib/enc/mcomp.c
  ${MOUNT_DIR}/libs/theora/lib/enc/x86_64/dsp_mmxext.c
  ${MOUNT_DIR}/libs/theora/lib/enc/x86_64/idct_mmx.c
  ${MOUNT_DIR}/libs/theora/lib/enc/x86_64/dct_decode_mmx.c
  ${MOUNT_DIR}/libs/theora/lib/enc/x86_64/recon_mmx.c
  ${MOUNT_DIR}/libs/theora/lib/enc/x86_64/fdct_mmx.c
  ${MOUNT_DIR}/libs/theora/lib/enc/x86_64/dsp_mmx.c
  ${MOUNT_DIR}/libs/theora/lib/enc/frinit.c
  ${MOUNT_DIR}/libs/theora/lib/enc/reconstruct.c
  ${MOUNT_DIR}/libs/theora/lib/enc/dct.c
  ${MOUNT_DIR}/libs/theora/lib/enc/encoder_idct.c
  ${MOUNT_DIR}/libs/theora/lib/enc/encoder_toplevel.c
  ${MOUNT_DIR}/libs/theora/lib/enc/encode.c
  ${MOUNT_DIR}/libs/theora/lib/enc/blockmap.c
  ${MOUNT_DIR}/libs/theora/lib/enc/dsp.c
  ${MOUNT_DIR}/libs/theora/lib/enc/encoder_huffman.c
  ${MOUNT_DIR}/libs/theora/lib/enc/scan.c
  #${MOUNT_DIR}/libs/theora/lib/enc/x86_32_vs/recon_mmx.c # Non-portable inline asm
  #${MOUNT_DIR}/libs/theora/lib/enc/x86_32_vs/fdct_mmx.c
  #${MOUNT_DIR}/libs/theora/lib/enc/x86_32_vs/dsp_mmx.c
  ${MOUNT_DIR}/libs/theora/lib/enc/pp.c
  ${MOUNT_DIR}/libs/theora/lib/enc/x86_32/dsp_mmxext.c
  ${MOUNT_DIR}/libs/theora/lib/enc/x86_32/idct_mmx.c
  ${MOUNT_DIR}/libs/theora/lib/enc/x86_32/dct_decode_mmx.c
  ${MOUNT_DIR}/libs/theora/lib/enc/x86_32/recon_mmx.c
  ${MOUNT_DIR}/libs/theora/lib/enc/x86_32/fdct_mmx.c
  ${MOUNT_DIR}/libs/theora/lib/enc/x86_32/dsp_mmx.c
  ${MOUNT_DIR}/libs/theora/lib/enc/misc_common.c
  ${MOUNT_DIR}/libs/theora/lib/enc/encapiwrapper.c
  ${MOUNT_DIR}/libs/theora/lib/enc/encoder_quant.c
  #${MOUNT_DIR}/libs/theora/lib/enc/encoder_disabled.c # Just stubs
  ${MOUNT_DIR}/libs/theora/lib/enc/frarray.c
  ${MOUNT_DIR}/libs/theora/lib/enc/dct_encode.c
  ${MOUNT_DIR}/libs/theora/lib/enc/dct_decode.c
  ${MOUNT_DIR}/libs/theora/lib/cpu.c
)

set( SHAREDLIST
 ${MOUNT_DIR}/engine/qcommon/q_shared.c
 ${MOUNT_DIR}/engine/qcommon/q_math.c
)

set( OPENEXRLIST
  ${MOUNT_DIR}/libs/openexr/half.cpp
)

set( RENDERERLIST
  ${MOUNT_DIR}/engine/renderer/tr_animation.c
  ${MOUNT_DIR}/engine/renderer/tr_animation_mdm.c
  ${MOUNT_DIR}/engine/renderer/tr_animation_mds.c
  ${MOUNT_DIR}/engine/renderer/tr_backend.c
  ${MOUNT_DIR}/engine/renderer/tr_bsp.c
  ${MOUNT_DIR}/engine/renderer/tr_cmds.c
  ${MOUNT_DIR}/engine/renderer/tr_cmesh.c
  ${MOUNT_DIR}/engine/renderer/tr_curve.c
  ${MOUNT_DIR}/engine/renderer/tr_decals.c
  ${MOUNT_DIR}/engine/renderer/tr_flares.c
  ${MOUNT_DIR}/engine/renderer/tr_font.c
  ${MOUNT_DIR}/engine/renderer/tr_image.c
  ${MOUNT_DIR}/engine/rendererGL/tr_image_webp.c
  ${MOUNT_DIR}/engine/renderer/tr_init.c
  ${MOUNT_DIR}/engine/renderer/tr_light.c
  ${MOUNT_DIR}/engine/renderer/tr_main.c
  ${MOUNT_DIR}/engine/renderer/tr_marks.c
  ${MOUNT_DIR}/engine/renderer/tr_mesh.c
  ${MOUNT_DIR}/engine/renderer/tr_model.c
  ${MOUNT_DIR}/engine/renderer/tr_model_md5.c
  ${MOUNT_DIR}/engine/renderer/tr_noise.c
  ${MOUNT_DIR}/engine/renderer/tr_scene.c
  ${MOUNT_DIR}/engine/renderer/tr_shade.c
  ${MOUNT_DIR}/engine/renderer/tr_shade_calc.c
  ${MOUNT_DIR}/engine/renderer/tr_shader.c
  ${MOUNT_DIR}/engine/renderer/tr_shadows.c
  ${MOUNT_DIR}/engine/renderer/tr_skin.c
  ${MOUNT_DIR}/engine/renderer/tr_sky.c
  ${MOUNT_DIR}/engine/renderer/tr_surface.c
  ${MOUNT_DIR}/engine/renderer/tr_world.c
)

set( RENDERERGLLIST
  ${MOUNT_DIR}/engine/rendererGL/gl_shader.cpp
  ${MOUNT_DIR}/engine/rendererGL/tr_animation_mdm.c
  ${MOUNT_DIR}/engine/rendererGL/tr_animation.c
  ${MOUNT_DIR}/engine/rendererGL/tr_backend.cpp
  ${MOUNT_DIR}/engine/rendererGL/tr_bsp.c
  ${MOUNT_DIR}/engine/rendererGL/tr_cmds.c
  ${MOUNT_DIR}/engine/rendererGL/tr_curve.c
  ${MOUNT_DIR}/engine/rendererGL/tr_decals.c
  ${MOUNT_DIR}/engine/rendererGL/tr_fbo.c
  ${MOUNT_DIR}/engine/rendererGL/tr_flares.c
  ${MOUNT_DIR}/engine/rendererGL/tr_fog.c
  ${MOUNT_DIR}/engine/rendererGL/tr_font.c
  ${MOUNT_DIR}/engine/rendererGL/tr_image.c
  ${MOUNT_DIR}/engine/rendererGL/tr_image_dds.c
  ${MOUNT_DIR}/engine/rendererGL/tr_image_exr.cpp
  ${MOUNT_DIR}/engine/rendererGL/tr_image_jpg.c
  ${MOUNT_DIR}/engine/rendererGL/tr_image_png.c
  ${MOUNT_DIR}/engine/rendererGL/tr_image_tga.c
  ${MOUNT_DIR}/engine/rendererGL/tr_image_webp.c
  ${MOUNT_DIR}/engine/rendererGL/tr_init.cpp
  ${MOUNT_DIR}/engine/rendererGL/tr_light.c
  ${MOUNT_DIR}/engine/rendererGL/tr_main.c
  ${MOUNT_DIR}/engine/rendererGL/tr_marks.c
  ${MOUNT_DIR}/engine/rendererGL/tr_mesh.c
  ${MOUNT_DIR}/engine/rendererGL/tr_model.c
  ${MOUNT_DIR}/engine/rendererGL/tr_model_md3.c
  ${MOUNT_DIR}/engine/rendererGL/tr_model_md5.c
  ${MOUNT_DIR}/engine/rendererGL/tr_model_mdc.c
  ${MOUNT_DIR}/engine/rendererGL/tr_model_mdm.c
  ${MOUNT_DIR}/engine/rendererGL/tr_model_psk.c
  ${MOUNT_DIR}/engine/rendererGL/tr_model_skel.c
  ${MOUNT_DIR}/engine/rendererGL/tr_noise.c
  ${MOUNT_DIR}/engine/rendererGL/tr_scene.c
  ${MOUNT_DIR}/engine/rendererGL/tr_shade.cpp
  ${MOUNT_DIR}/engine/rendererGL/tr_shade_calc.c
  ${MOUNT_DIR}/engine/rendererGL/tr_shader.c
  ${MOUNT_DIR}/engine/rendererGL/tr_shadows.c
  ${MOUNT_DIR}/engine/rendererGL/tr_skin.c
  ${MOUNT_DIR}/engine/rendererGL/tr_sky.cpp
  ${MOUNT_DIR}/engine/rendererGL/tr_surface.c
  ${MOUNT_DIR}/engine/rendererGL/tr_vbo.c
  ${MOUNT_DIR}/engine/rendererGL/tr_world.cpp
)

set( SERVERLIST
  ${MOUNT_DIR}/engine/server/sv_bot.c
  ${MOUNT_DIR}/engine/server/sv_ccmds.c
  ${MOUNT_DIR}/engine/server/sv_client.c
  ${MOUNT_DIR}/engine/server/sv_game.c
  ${MOUNT_DIR}/engine/server/sv_init.c
  ${MOUNT_DIR}/engine/server/sv_main.c
  ${MOUNT_DIR}/engine/server/sv_net_chan.c
  ${MOUNT_DIR}/engine/server/sv_snapshot.c
  ${MOUNT_DIR}/engine/server/sv_world.c
)

set( QCOMMONLIST
  ${MOUNT_DIR}/engine/asm/ftola.c
  ${MOUNT_DIR}/engine/asm/snapvector.c
  ${MOUNT_DIR}/engine/qcommon/cm_load.c
  ${MOUNT_DIR}/engine/qcommon/cm_trisoup.c
  ${MOUNT_DIR}/engine/qcommon/cm_patch.c
  ${MOUNT_DIR}/engine/qcommon/cm_polylib.c
  ${MOUNT_DIR}/engine/qcommon/cm_test.c
  ${MOUNT_DIR}/engine/qcommon/cm_trace.c
  ${MOUNT_DIR}/engine/qcommon/cmd.c
  ${MOUNT_DIR}/engine/qcommon/common.c
  ${MOUNT_DIR}/engine/qcommon/crypto.c
  ${MOUNT_DIR}/engine/qcommon/cvar.c
  ${MOUNT_DIR}/engine/qcommon/files.c
  ${MOUNT_DIR}/engine/qcommon/htable.c
  ${MOUNT_DIR}/engine/qcommon/huffman.c
  ${MOUNT_DIR}/engine/qcommon/md4.c
  ${MOUNT_DIR}/engine/qcommon/md5.c
  ${MOUNT_DIR}/engine/qcommon/msg.c
  ${MOUNT_DIR}/engine/qcommon/net_chan.c
  ${MOUNT_DIR}/engine/qcommon/net_ip.c
  ${MOUNT_DIR}/engine/qcommon/parse.c
  ${MOUNT_DIR}/engine/qcommon/unzip.c
  ${MOUNT_DIR}/engine/qcommon/ioapi.c
  ${MOUNT_DIR}/engine/qcommon/vm.c
  ${MOUNT_DIR}/engine/qcommon/vm_x86.c
  ${MOUNT_DIR}/engine/qcommon/vm_interpreted.c
)

if( USE_LLVM )
  set( QCOMMONLIST ${QCOMMONLIST} ${MOUNT_DIR}/engine/qcommon/vm_llvm.cpp )
  add_definitions( -DUSE_LLVM )
endif( )

if( USE_PHYSICS )
  set( QCOMMONLIST ${QCOMMONLIST} ${MOUNT_DIR}/engine/qcommon/cm_physics.cpp )
endif( )

if( USE_MYSQL )
  set( DATABASELIST
    ${MOUNT_DIR}/engine/database/db_main.c
    ${MOUNT_DIR}/engine/database/db_mysql.c
  )
  find_package( MySQL REQUIRED )
  add_definitions( -DET_MYSQL )
else()
  set( DATABASELIST )
endif()

set( CLIENTLIST
  ${MOUNT_DIR}/engine/client/cin_ogm.c
  ${MOUNT_DIR}/engine/client/cl_avi.c
  ${MOUNT_DIR}/engine/client/cl_cgame.c
  ${MOUNT_DIR}/engine/client/cl_cin.c
  ${MOUNT_DIR}/engine/client/cl_console.c
  ${MOUNT_DIR}/engine/client/cl_input.c
  ${MOUNT_DIR}/engine/client/cl_irc.c
  ${MOUNT_DIR}/engine/client/cl_keys.c
  ${MOUNT_DIR}/engine/client/cl_main.c
  ${MOUNT_DIR}/engine/client/cl_net_chan.c
  ${MOUNT_DIR}/engine/client/cl_parse.c
  ${MOUNT_DIR}/engine/client/cl_scrn.c
  ${MOUNT_DIR}/engine/client/cl_ui.c
  ${MOUNT_DIR}/engine/client/snd_adpcm.c
  ${MOUNT_DIR}/engine/client/snd_codec.c
  ${MOUNT_DIR}/engine/client/snd_codec_ogg.c
  ${MOUNT_DIR}/engine/client/snd_codec_wav.c
  ${MOUNT_DIR}/engine/client/snd_dma.c
  ${MOUNT_DIR}/engine/client/snd_load.c
  ${MOUNT_DIR}/engine/client/snd_mem.c
  ${MOUNT_DIR}/engine/client/snd_mix.c
  ${MOUNT_DIR}/engine/client/snd_wavelet.c
)

if( USE_MUMBLE )
  set( CLIENTLIST ${CLIENTLIST} ${MOUNT_DIR}/engine/client/libmumblelink.c )
endif( )

set( ALLIST
  ${MOUNT_DIR}/engine/snd_openal/qal.c
  ${MOUNT_DIR}/engine/snd_openal/snd_al_buffers.c
  ${MOUNT_DIR}/engine/snd_openal/snd_al_main.c
  ${MOUNT_DIR}/engine/snd_openal/snd_al_music.c
  ${MOUNT_DIR}/engine/snd_openal/snd_al_sources.c
  ${MOUNT_DIR}/engine/snd_openal/snd_al_stream.c
)


if( USE_CPUINFO )
  set( CPUINFOLIST
  ${MOUNT_DIR}/libs/cpuinfo/CPUInfo.c
  )
  add_definitions( -DNOCPUINFO )
else()
  set( CPUINFOLIST )
endif()
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  set( SDLBASELIST
      ${MOUNT_DIR}/engine/sys/sys_unix.c
      ${MOUNT_DIR}/engine/sys/sys_osx.m
      ${MOUNT_DIR}/engine/sys/sys_main.c
      ${MOUNT_DIR}/engine/sys/con_log.c
      ${MOUNT_DIR}/engine/sys/con_${CONSOLE}.c
     )
else()
  set( SDLBASELIST
      ${MOUNT_DIR}/engine/sys/sys_${PLATFORM}.c
      ${MOUNT_DIR}/engine/sys/sys_main.c
      ${MOUNT_DIR}/engine/sys/con_log.c
      ${MOUNT_DIR}/engine/sys/con_${CONSOLE}.c
  )
endif()

if( USE_CURSES )
  set( SDLBASELIST ${SDLBASELIST} ${MOUNT_DIR}/engine/sys/con_curses.c )
endif()

set( SDLLIST
  ${MOUNT_DIR}/engine/sys/sdl_input.c
  ${MOUNT_DIR}/engine/sys/sdl_snd.c
)


set( LINUXSOURCESLIST
  ${MOUNT_DIR}/libs/glew/src/glew.c
)

set( MACSOURCESLIST
  ${MOUNT_DIR}/engine/mac/mac_main.cpp
  ${MOUNT_DIR}/engine/mac/mac_input.cpp
  ${MOUNT_DIR}/engine/mac/mac_glimp.cpp
  ${MOUNT_DIR}/engine/mac/mac_files.cpp
  ${MOUNT_DIR}/engine/mac/mac_net.cpp
  ${MOUNT_DIR}/engine/mac/mac_console.c
  ${MOUNT_DIR}/engine/mac/mac_snddma.c
  ${MOUNT_DIR}/engine/mac/CarbonMouse.cpp
  ${MOUNT_DIR}/engine/mac/mac_event.cpp
  ${MOUNT_DIR}/engine/mac/MacPrefs.cpp
  ${MOUNT_DIR}/engine/mac/PickMonitor/pickmonitor.cpp
  ${MOUNT_DIR}/engine/mac/PickMonitor/userpane.cpp
  ${MOUNT_DIR}/engine/mac/mac_qgl.c
  ${MOUNT_DIR}/engine/mac/mac_specific.cpp
  ${MOUNT_DIR}/engine/mac/AGLUtils.cpp
  ${MOUNT_DIR}/engine/mac/CDrawSprocket.cpp
)


set( BOTLIBLIST
  ${MOUNT_DIR}/engine/botlib/be_aas_bspq3.c
  ${MOUNT_DIR}/engine/botlib/be_aas_cluster.c
  ${MOUNT_DIR}/engine/botlib/be_aas_debug.c
  ${MOUNT_DIR}/engine/botlib/be_aas_entity.c
  ${MOUNT_DIR}/engine/botlib/be_aas_file.c
  ${MOUNT_DIR}/engine/botlib/be_aas_main.c
  ${MOUNT_DIR}/engine/botlib/be_aas_move.c
  ${MOUNT_DIR}/engine/botlib/be_aas_optimize.c
  ${MOUNT_DIR}/engine/botlib/be_aas_reach.c
  ${MOUNT_DIR}/engine/botlib/be_aas_route.c
  ${MOUNT_DIR}/engine/botlib/be_aas_routealt.c
  ${MOUNT_DIR}/engine/botlib/be_aas_routetable.c
  ${MOUNT_DIR}/engine/botlib/be_aas_sample.c
  ${MOUNT_DIR}/engine/botlib/be_ai_char.c
  ${MOUNT_DIR}/engine/botlib/be_ai_chat.c
  ${MOUNT_DIR}/engine/botlib/be_ai_gen.c
  ${MOUNT_DIR}/engine/botlib/be_ai_goal.c
  ${MOUNT_DIR}/engine/botlib/be_ai_move.c
  ${MOUNT_DIR}/engine/botlib/be_ai_weap.c
  ${MOUNT_DIR}/engine/botlib/be_ai_weight.c
  ${MOUNT_DIR}/engine/botlib/be_ea.c
  ${MOUNT_DIR}/engine/botlib/be_interface.c
  ${MOUNT_DIR}/engine/botlib/l_crc.c
  ${MOUNT_DIR}/engine/botlib/l_libvar.c
  ${MOUNT_DIR}/engine/botlib/l_log.c
  ${MOUNT_DIR}/engine/botlib/l_memory.c
  ${MOUNT_DIR}/engine/botlib/l_precomp.c
  ${MOUNT_DIR}/engine/botlib/l_script.c
  ${MOUNT_DIR}/engine/botlib/l_struct.c
)

set( QAGAMELIST
  ${GAME_DIR}/src/game/bg_animation.c
  ${GAME_DIR}/src/game/bg_animgroup.c
  ${GAME_DIR}/src/game/bg_campaign.c
  ${GAME_DIR}/src/game/bg_character.c
  ${GAME_DIR}/src/game/bg_classes.c
  ${GAME_DIR}/src/game/bg_misc.c
  ${GAME_DIR}/src/game/bg_pmove.c
  ${GAME_DIR}/src/game/bg_slidemove.c
  ${GAME_DIR}/src/game/bg_sscript.c
  ${GAME_DIR}/src/game/bg_stats.c
  ${GAME_DIR}/src/game/bg_tracemap.c
  ${GAME_DIR}/src/game/g_active.c
  ${GAME_DIR}/src/game/g_admin.c
  ${GAME_DIR}/src/game/g_alarm.c
  ${GAME_DIR}/src/game/g_antilag.c
  ${GAME_DIR}/src/game/g_buddy_list.c
  ${GAME_DIR}/src/game/g_character.c
  ${GAME_DIR}/src/game/g_client.c
  ${GAME_DIR}/src/game/g_cmds.c
  ${GAME_DIR}/src/game/g_cmds_ext.c
  ${GAME_DIR}/src/game/g_combat.c
  ${GAME_DIR}/src/game/g_config.c
  ${GAME_DIR}/src/game/g_fireteams.c
  ${GAME_DIR}/src/game/g_items.c
  ${GAME_DIR}/src/game/g_main.c
  ${GAME_DIR}/src/game/g_match.c
  ${GAME_DIR}/src/game/g_mem.c
  ${GAME_DIR}/src/game/g_misc.c
  ${GAME_DIR}/src/game/g_missile.c
  ${GAME_DIR}/src/game/g_mover.c
  ${GAME_DIR}/src/game/g_multiview.c
  ${GAME_DIR}/src/game/g_props.c
  ${GAME_DIR}/src/game/g_referee.c
  ${GAME_DIR}/src/game/g_script.c
  ${GAME_DIR}/src/game/g_script_actions.c
  ${GAME_DIR}/src/game/g_session.c
  ${GAME_DIR}/src/game/g_spawn.c
  ${GAME_DIR}/src/game/g_stats.c
  ${GAME_DIR}/src/game/g_svcmds.c
  ${GAME_DIR}/src/game/g_api.c
  ${GAME_DIR}/src/game/g_systemmsg.c
  ${GAME_DIR}/src/game/g_target.c
  ${GAME_DIR}/src/game/g_team.c
  ${GAME_DIR}/src/game/g_teammapdata.c
  ${GAME_DIR}/src/game/g_trigger.c
  ${GAME_DIR}/src/game/g_utils.c
  ${GAME_DIR}/src/game/g_vote.c
  ${GAME_DIR}/src/game/g_weapon.c
  ${MOUNT_DIR}/engine/qcommon/q_math.c
  ${MOUNT_DIR}/engine/qcommon/q_shared.c
)

set( CGAMELIST
  ${GAME_DIR}/src/cgame/cg_main.c
  ${GAME_DIR}/src/game/bg_animation.c
  ${GAME_DIR}/src/game/bg_animgroup.c
  ${GAME_DIR}/src/game/bg_character.c
  ${GAME_DIR}/src/game/bg_classes.c
  ${GAME_DIR}/src/game/bg_misc.c
  ${GAME_DIR}/src/game/bg_pmove.c
  ${GAME_DIR}/src/game/bg_slidemove.c
  ${GAME_DIR}/src/game/bg_sscript.c
  ${GAME_DIR}/src/game/bg_stats.c
  ${GAME_DIR}/src/game/bg_tracemap.c
  ${GAME_DIR}/src/cgame/cg_atmospheric.c
  ${GAME_DIR}/src/cgame/cg_character.c
  ${GAME_DIR}/src/cgame/cg_commandmap.c
  ${GAME_DIR}/src/cgame/cg_consolecmds.c
  ${GAME_DIR}/src/cgame/cg_debriefing.c
  ${GAME_DIR}/src/cgame/cg_draw.c
  ${GAME_DIR}/src/cgame/cg_drawtools.c
  ${GAME_DIR}/src/cgame/cg_effects.c
  ${GAME_DIR}/src/cgame/cg_ents.c
  ${GAME_DIR}/src/cgame/cg_event.c
  ${GAME_DIR}/src/cgame/cg_fireteamoverlay.c
  ${GAME_DIR}/src/cgame/cg_fireteams.c
  ${GAME_DIR}/src/cgame/cg_flamethrower.c
  ${GAME_DIR}/src/cgame/cg_info.c
  ${GAME_DIR}/src/cgame/cg_limbopanel.c
  ${GAME_DIR}/src/cgame/cg_loadpanel.c
  ${GAME_DIR}/src/cgame/cg_localents.c
  ${GAME_DIR}/src/cgame/cg_marks.c
  ${GAME_DIR}/src/cgame/cg_missionbriefing.c
  ${GAME_DIR}/src/cgame/cg_multiview.c
  ${GAME_DIR}/src/cgame/cg_newDraw.c
  ${GAME_DIR}/src/cgame/cg_particles.c
  ${GAME_DIR}/src/cgame/cg_players.c
  ${GAME_DIR}/src/cgame/cg_playerstate.c
  ${GAME_DIR}/src/cgame/cg_polybus.c
  ${GAME_DIR}/src/cgame/cg_popupmessages.c
  ${GAME_DIR}/src/cgame/cg_predict.c
  ${GAME_DIR}/src/cgame/cg_scoreboard.c
  ${GAME_DIR}/src/cgame/cg_servercmds.c
  ${GAME_DIR}/src/cgame/cg_snapshot.c
  ${GAME_DIR}/src/cgame/cg_sound.c
  ${GAME_DIR}/src/cgame/cg_spawn.c
  ${GAME_DIR}/src/cgame/cg_statsranksmedals.c
  ${GAME_DIR}/src/cgame/cg_api.c
  ${GAME_DIR}/src/cgame/cg_trails.c
  ${GAME_DIR}/src/cgame/cg_view.c
  ${GAME_DIR}/src/cgame/cg_weapons.c
  ${GAME_DIR}/src/cgame/cg_window.c
  ${GAME_DIR}/src/ui/ui_shared.c
  ${MOUNT_DIR}/engine/qcommon/q_math.c
  ${MOUNT_DIR}/engine/qcommon/q_shared.c
)

set( OMNIBOTLIST
  ${GAME_DIR}/omnibot/common/BotLoadLibrary.cpp
  ${GAME_DIR}/omnibot/et/g_etbot_interface.cpp
)

set( UILIST
  ${GAME_DIR}/src/ui/ui_atoms.c
  ${GAME_DIR}/src/ui/ui_gameinfo.c
  ${GAME_DIR}/src/ui/ui_loadpanel.c
  ${GAME_DIR}/src/ui/ui_main.c
  ${GAME_DIR}/src/ui/ui_players.c
  ${GAME_DIR}/src/ui/ui_shared.c
  ${GAME_DIR}/src/ui/ui_api.c
  ${GAME_DIR}/src/ui/ui_util.c
  ${GAME_DIR}/src/game/bg_campaign.c
  ${GAME_DIR}/src/game/bg_classes.c
  ${GAME_DIR}/src/game/bg_misc.c
  ${MOUNT_DIR}/engine/qcommon/q_math.c
  ${MOUNT_DIR}/engine/qcommon/q_shared.c
)

set( CGAMETREM
  ${BASE_DIR}/src/game/bg_misc.c
  ${BASE_DIR}/src/game/bg_pmove.c
  ${BASE_DIR}/src/game/bg_slidemove.c
  ${BASE_DIR}/src/ui/ui_shared.c
  ${BASE_DIR}/src/cgame/cg_animation.c
  ${BASE_DIR}/src/cgame/cg_animmapobj.c
  ${BASE_DIR}/src/cgame/cg_attachment.c
  ${BASE_DIR}/src/cgame/cg_buildable.c
  ${BASE_DIR}/src/cgame/cg_consolecmds.c
  ${BASE_DIR}/src/cgame/cg_draw.c
  ${BASE_DIR}/src/cgame/cg_drawtools.c
  ${BASE_DIR}/src/cgame/cg_ents.c
  ${BASE_DIR}/src/cgame/cg_event.c
  ${BASE_DIR}/src/cgame/cg_main.c
  ${BASE_DIR}/src/cgame/cg_marks.c
  ${BASE_DIR}/src/cgame/cg_mem.c
  ${BASE_DIR}/src/cgame/cg_particles.c
  ${BASE_DIR}/src/cgame/cg_players.c
  ${BASE_DIR}/src/cgame/cg_playerstate.c
  ${BASE_DIR}/src/cgame/cg_predict.c
  ${BASE_DIR}/src/cgame/cg_ptr.c
  ${BASE_DIR}/src/cgame/cg_scanner.c
  ${BASE_DIR}/src/cgame/cg_servercmds.c
  ${BASE_DIR}/src/cgame/cg_snapshot.c
  ${BASE_DIR}/src/cgame/cg_trails.c
  ${BASE_DIR}/src/cgame/cg_tutorial.c
  ${BASE_DIR}/src/cgame/cg_view.c
  ${BASE_DIR}/src/cgame/cg_weapons.c
  ${BASE_DIR}/src/cgame/cg_api.c
  ${MOUNT_DIR}/engine/qcommon/q_shared.c
  ${MOUNT_DIR}/engine/qcommon/q_math.c
)

set( GAMETREM
  ${BASE_DIR}/src/game/bg_misc.c
  ${BASE_DIR}/src/game/bg_pmove.c
  ${BASE_DIR}/src/game/bg_slidemove.c
  ${BASE_DIR}/src/game/g_active.c
  ${BASE_DIR}/src/game/g_buildable.c
  ${BASE_DIR}/src/game/g_client.c
  ${BASE_DIR}/src/game/g_cmds.c
  ${BASE_DIR}/src/game/g_combat.c
  ${BASE_DIR}/src/game/g_main.c
  ${BASE_DIR}/src/game/g_maprotation.c
  ${BASE_DIR}/src/game/g_mem.c
  ${BASE_DIR}/src/game/g_misc.c
  ${BASE_DIR}/src/game/g_missile.c
  ${BASE_DIR}/src/game/g_mover.c
  ${BASE_DIR}/src/game/g_physics.c
  ${BASE_DIR}/src/game/g_ptr.c
  ${BASE_DIR}/src/game/g_session.c
  ${BASE_DIR}/src/game/g_spawn.c
  ${BASE_DIR}/src/game/g_svcmds.c
  ${BASE_DIR}/src/game/g_target.c
  ${BASE_DIR}/src/game/g_team.c
  ${BASE_DIR}/src/game/g_trigger.c
  ${BASE_DIR}/src/game/g_utils.c
  ${BASE_DIR}/src/game/g_weapon.c
  ${BASE_DIR}/src/game/g_api.c
  ${MOUNT_DIR}/engine/qcommon/q_shared.c
  ${MOUNT_DIR}/engine/qcommon/q_math.c
)

set( UITREM
  ${BASE_DIR}/src/ui/ui_atoms.c
  ${BASE_DIR}/src/ui/ui_gameinfo.c
  ${BASE_DIR}/src/ui/ui_main.c
  ${BASE_DIR}/src/ui/ui_players.c
  ${BASE_DIR}/src/ui/ui_shared.c
  ${BASE_DIR}/src/game/bg_misc.c
  ${MOUNT_DIR}/engine/qcommon/q_shared.c
  ${MOUNT_DIR}/engine/qcommon/q_math.c
  ${BASE_DIR}/src/ui/ui_api.c
)
set( CGAMEGPP
 ${GPP_DIR}/src/cgame/cg_main.c
 ${GPP_DIR}/src/game/bg_misc.c
 ${GPP_DIR}/src/game/bg_pmove.c
 ${GPP_DIR}/src/game/bg_slidemove.c
 ${GPP_DIR}/src/game/bg_lib.c
 ${GPP_DIR}/src/game/bg_alloc.c
 ${GPP_DIR}/src/game/bg_voice.c
 ${GPP_DIR}/src/cgame/cg_consolecmds.c
 ${GPP_DIR}/src/cgame/cg_buildable.c
 ${GPP_DIR}/src/cgame/cg_animation.c
 ${GPP_DIR}/src/cgame/cg_animmapobj.c
 ${GPP_DIR}/src/cgame/cg_draw.c
 ${GPP_DIR}/src/cgame/cg_drawtools.c
 ${GPP_DIR}/src/cgame/cg_ents.c
 ${GPP_DIR}/src/cgame/cg_event.c
 ${GPP_DIR}/src/cgame/cg_marks.c
 ${GPP_DIR}/src/cgame/cg_players.c
 ${GPP_DIR}/src/cgame/cg_playerstate.c
 ${GPP_DIR}/src/cgame/cg_predict.c
 ${GPP_DIR}/src/cgame/cg_servercmds.c
 ${GPP_DIR}/src/cgame/cg_snapshot.c
 ${GPP_DIR}/src/cgame/cg_view.c
 ${GPP_DIR}/src/cgame/cg_weapons.c
 ${GPP_DIR}/src/cgame/cg_scanner.c
 ${GPP_DIR}/src/cgame/cg_attachment.c
 ${GPP_DIR}/src/cgame/cg_trails.c
 ${GPP_DIR}/src/cgame/cg_particles.c
 ${GPP_DIR}/src/cgame/cg_tutorial.c
 ${GPP_DIR}/src/ui/ui_shared.c
 ${MOUNT_DIR}/engine/qcommon/q_math.c
 ${MOUNT_DIR}/engine/qcommon/q_shared.c
 ${GPP_DIR}/src/cgame/cg_api.c
)

set( GAMEGPP
 ${GPP_DIR}/src/game/g_main.c
 ${GPP_DIR}/src/game/bg_misc.c
 ${GPP_DIR}/src/game/bg_pmove.c
 ${GPP_DIR}/src/game/bg_slidemove.c
 ${GPP_DIR}/src/game/bg_lib.c
 ${GPP_DIR}/src/game/bg_alloc.c
 ${GPP_DIR}/src/game/bg_voice.c
 ${GPP_DIR}/src/game/g_active.c
 ${GPP_DIR}/src/game/g_client.c
 ${GPP_DIR}/src/game/g_cmds.c
 ${GPP_DIR}/src/game/g_combat.c
 ${GPP_DIR}/src/game/g_physics.c
 ${GPP_DIR}/src/game/g_buildable.c
 ${GPP_DIR}/src/game/g_misc.c
 ${GPP_DIR}/src/game/g_missile.c
 ${GPP_DIR}/src/game/g_mover.c
 ${GPP_DIR}/src/game/g_session.c
 ${GPP_DIR}/src/game/g_spawn.c
 ${GPP_DIR}/src/game/g_svcmds.c
 ${GPP_DIR}/src/game/g_target.c
 ${GPP_DIR}/src/game/g_team.c
 ${GPP_DIR}/src/game/g_trigger.c
 ${GPP_DIR}/src/game/g_utils.c
 ${GPP_DIR}/src/game/g_maprotation.c
 ${GPP_DIR}/src/game/g_weapon.c
 ${GPP_DIR}/src/game/g_admin.c
 ${GPP_DIR}/src/game/g_namelog.c
 ${MOUNT_DIR}/engine/qcommon/q_math.c
 ${MOUNT_DIR}/engine/qcommon/q_shared.c
 ${GPP_DIR}/src/game/g_api.c
)

set( UIGPP
 ${GPP_DIR}/src/ui/ui_main.c
 ${GPP_DIR}/src/ui/ui_atoms.c
 ${GPP_DIR}/src/ui/ui_shared.c
 ${GPP_DIR}/src/ui/ui_gameinfo.c
 ${GPP_DIR}/src/game/bg_misc.c
 ${GPP_DIR}/src/game/bg_lib.c
 ${MOUNT_DIR}/engine/qcommon/q_math.c
 ${MOUNT_DIR}/engine/qcommon/q_shared.c
 ${GPP_DIR}/src/ui/ui_api.c
)

if( USE_VOIP )
  set( SPEEXLIST
    ${MOUNT_DIR}/libs/libspeex/libspeex/bits.c
    ${MOUNT_DIR}/libs/libspeex/libspeex/buffer.c
    ${MOUNT_DIR}/libs/libspeex/libspeex/cb_search.c
    ${MOUNT_DIR}/libs/libspeex/libspeex/exc_10_16_table.c
    ${MOUNT_DIR}/libs/libspeex/libspeex/exc_10_32_table.c
    ${MOUNT_DIR}/libs/libspeex/libspeex/exc_20_32_table.c
    ${MOUNT_DIR}/libs/libspeex/libspeex/exc_5_256_table.c
    ${MOUNT_DIR}/libs/libspeex/libspeex/exc_5_64_table.c
    ${MOUNT_DIR}/libs/libspeex/libspeex/exc_8_128_table.c
    ${MOUNT_DIR}/libs/libspeex/libspeex/fftwrap.c
    ${MOUNT_DIR}/libs/libspeex/libspeex/filterbank.c
    ${MOUNT_DIR}/libs/libspeex/libspeex/filters.c
    ${MOUNT_DIR}/libs/libspeex/libspeex/gain_table.c
    ${MOUNT_DIR}/libs/libspeex/libspeex/gain_table_lbr.c
    ${MOUNT_DIR}/libs/libspeex/libspeex/hexc_10_32_table.c
    ${MOUNT_DIR}/libs/libspeex/libspeex/hexc_table.c
    ${MOUNT_DIR}/libs/libspeex/libspeex/high_lsp_tables.c
    ${MOUNT_DIR}/libs/libspeex/libspeex/jitter.c
    ${MOUNT_DIR}/libs/libspeex/libspeex/kiss_fft.c
    ${MOUNT_DIR}/libs/libspeex/libspeex/kiss_fftr.c
    ${MOUNT_DIR}/libs/libspeex/libspeex/lpc.c
    ${MOUNT_DIR}/libs/libspeex/libspeex/lsp.c
    ${MOUNT_DIR}/libs/libspeex/libspeex/lsp_tables_nb.c
    ${MOUNT_DIR}/libs/libspeex/libspeex/ltp.c
    ${MOUNT_DIR}/libs/libspeex/libspeex/mdf.c
    ${MOUNT_DIR}/libs/libspeex/libspeex/modes.c
    ${MOUNT_DIR}/libs/libspeex/libspeex/modes_wb.c
    ${MOUNT_DIR}/libs/libspeex/libspeex/nb_celp.c
    ${MOUNT_DIR}/libs/libspeex/libspeex/preprocess.c
    ${MOUNT_DIR}/libs/libspeex/libspeex/quant_lsp.c
    ${MOUNT_DIR}/libs/libspeex/libspeex/resample.c
    ${MOUNT_DIR}/libs/libspeex/libspeex/sb_celp.c
    ${MOUNT_DIR}/libs/libspeex/libspeex/smallft.c
    ${MOUNT_DIR}/libs/libspeex/libspeex/speex.c
    ${MOUNT_DIR}/libs/libspeex/libspeex/speex_callbacks.c
    ${MOUNT_DIR}/libs/libspeex/libspeex/speex_header.c
    ${MOUNT_DIR}/libs/libspeex/libspeex/stereo.c
    ${MOUNT_DIR}/libs/libspeex/libspeex/vbr.c
    ${MOUNT_DIR}/libs/libspeex/libspeex/vq.c
    ${MOUNT_DIR}/libs/libspeex/libspeex/window.c
   )
else()
  set( SPEEXLIST )
endif()

set( CRYPTOLIST
  ${MOUNT_DIR}/libs/nettle/nettle/bignum.c
  ${MOUNT_DIR}/libs/nettle/nettle/bignum-random.c
  ${MOUNT_DIR}/libs/nettle/nettle/buffer-init.c
  ${MOUNT_DIR}/libs/nettle/nettle/nettle-buffer.c
  ${MOUNT_DIR}/libs/nettle/nettle/nettle-internal.c
  ${MOUNT_DIR}/libs/nettle/nettle/rsa2sexp.c
  ${MOUNT_DIR}/libs/nettle/nettle/rsa.c
  ${MOUNT_DIR}/libs/nettle/nettle/rsa-decrypt.c
  ${MOUNT_DIR}/libs/nettle/nettle/rsa-encrypt.c
  ${MOUNT_DIR}/libs/nettle/nettle/rsa-keygen.c
  ${MOUNT_DIR}/libs/nettle/nettle/rsa-sign.c
  ${MOUNT_DIR}/libs/nettle/nettle/sexp2bignum.c
  ${MOUNT_DIR}/libs/nettle/nettle/sexp2rsa.c
  ${MOUNT_DIR}/libs/nettle/nettle/sexp.c
  ${MOUNT_DIR}/libs/nettle/nettle/sexp-format.c
)

if(  USE_CIN_XVID )
  add_definitions( -DUSE_CIN_XVID )
  find_package( xvid REQUIRED )
endif()
set( OWMAPLIST
  ${MOUNT_DIR}/tools/common/cmdlib.c
  ${MOUNT_DIR}/tools/common/imagelib.c
  ${MOUNT_DIR}/tools/common/inout.c
  ${MOUNT_DIR}/tools/common/mathlib.c
  ${MOUNT_DIR}/tools/common/md4.c
  ${MOUNT_DIR}/tools/common/mutex.c
  ${MOUNT_DIR}/tools/common/polylib.c
  ${MOUNT_DIR}/tools/common/scriplib.c
  ${MOUNT_DIR}/tools/common/threads.c
  ${MOUNT_DIR}/tools/common/unzip.c
  ${MOUNT_DIR}/tools/common/vfs.c
  ${MOUNT_DIR}/tools/common/ddslib.c
  ${MOUNT_DIR}/libs/picomodel/picointernal.c
  ${MOUNT_DIR}/libs/picomodel/picomodel.c
  ${MOUNT_DIR}/libs/picomodel/picomodules.c
  ${MOUNT_DIR}/libs/picomodel/pm_3ds.c
  ${MOUNT_DIR}/libs/picomodel/pm_ase.c
  ${MOUNT_DIR}/libs/picomodel/pm_fm.c
  ${MOUNT_DIR}/libs/picomodel/pm_lwo.c
  ${MOUNT_DIR}/libs/picomodel/pm_md2.c
  ${MOUNT_DIR}/libs/picomodel/pm_md3.c
  ${MOUNT_DIR}/libs/picomodel/pm_mdc.c
  ${MOUNT_DIR}/libs/picomodel/pm_ms3d.c
  ${MOUNT_DIR}/libs/picomodel/pm_obj.c
  ${MOUNT_DIR}/libs/picomodel/pm_terrain.c
  ${MOUNT_DIR}/libs/picomodel/lwo/clip.c
  ${MOUNT_DIR}/libs/picomodel/lwo/envelope.c
  ${MOUNT_DIR}/libs/picomodel/lwo/list.c
  ${MOUNT_DIR}/libs/picomodel/lwo/lwio.c
  ${MOUNT_DIR}/libs/picomodel/lwo/lwo2.c
  ${MOUNT_DIR}/libs/picomodel/lwo/lwob.c
  ${MOUNT_DIR}/libs/picomodel/lwo/pntspols.c
  ${MOUNT_DIR}/libs/picomodel/lwo/surface.c
  ${MOUNT_DIR}/libs/picomodel/lwo/vecmath.c
  ${MOUNT_DIR}/libs/picomodel/lwo/vmap.c
#   ${MOUNT_DIR}/libs/jpeg/jaricom.c
#   ${MOUNT_DIR}/libs/jpeg/jcapimin.c
#   ${MOUNT_DIR}/libs/jpeg/jcapistd.c
#   ${MOUNT_DIR}/libs/jpeg/jcarith.c
#   ${MOUNT_DIR}/libs/jpeg/jccoefct.c
#   ${MOUNT_DIR}/libs/jpeg/jccolor.c
#   ${MOUNT_DIR}/libs/jpeg/jcdctmgr.c
#   ${MOUNT_DIR}/libs/jpeg/jchuff.c
#   ${MOUNT_DIR}/libs/jpeg/jcinit.c
#   ${MOUNT_DIR}/libs/jpeg/jcmainct.c
#   ${MOUNT_DIR}/libs/jpeg/jcmarker.c
#   ${MOUNT_DIR}/libs/jpeg/jcmaster.c
#   ${MOUNT_DIR}/libs/jpeg/jcomapi.c
#   ${MOUNT_DIR}/libs/jpeg/jcparam.c
#   ${MOUNT_DIR}/libs/jpeg/jcprepct.c
#   ${MOUNT_DIR}/libs/jpeg/jcsample.c
#   ${MOUNT_DIR}/libs/jpeg/jctrans.c
#   ${MOUNT_DIR}/libs/jpeg/jdapimin.c
#   ${MOUNT_DIR}/libs/jpeg/jdapistd.c
#   ${MOUNT_DIR}/libs/jpeg/jdarith.c
#   ${MOUNT_DIR}/libs/jpeg/jdatadst.c
#   ${MOUNT_DIR}/libs/jpeg/jdatasrc.c
#   ${MOUNT_DIR}/libs/jpeg/jdcoefct.c
#   ${MOUNT_DIR}/libs/jpeg/jdcolor.c
#   ${MOUNT_DIR}/libs/jpeg/jddctmgr.c
#   ${MOUNT_DIR}/libs/jpeg/jdhuff.c
#   ${MOUNT_DIR}/libs/jpeg/jdinput.c
#   ${MOUNT_DIR}/libs/jpeg/jdmainct.c
#   ${MOUNT_DIR}/libs/jpeg/jdmarker.c
#   ${MOUNT_DIR}/libs/jpeg/jdmaster.c
#   ${MOUNT_DIR}/libs/jpeg/jdmerge.c
#   ${MOUNT_DIR}/libs/jpeg/jdpostct.c
#   ${MOUNT_DIR}/libs/jpeg/jdsample.c
#   ${MOUNT_DIR}/libs/jpeg/jdtrans.c
#   ${MOUNT_DIR}/libs/jpeg/jerror.c
#   ${MOUNT_DIR}/libs/jpeg/jfdctflt.c
#   ${MOUNT_DIR}/libs/jpeg/jfdctfst.c
#   ${MOUNT_DIR}/libs/jpeg/jfdctint.c
#   ${MOUNT_DIR}/libs/jpeg/jidctflt.c
#   ${MOUNT_DIR}/libs/jpeg/jidctfst.c
#   ${MOUNT_DIR}/libs/jpeg/jidctint.c
#   ${MOUNT_DIR}/libs/jpeg/jmemmgr.c
#   ${MOUNT_DIR}/libs/jpeg/jmemnobs.c
#   ${MOUNT_DIR}/libs/jpeg/jquant1.c
#   ${MOUNT_DIR}/libs/jpeg/jquant2.c
#   ${MOUNT_DIR}/libs/jpeg/jutils.c
  ${MOUNT_DIR}/libs/zlib/adler32.c
  ${MOUNT_DIR}/libs/zlib/compress.c
  ${MOUNT_DIR}/libs/zlib/crc32.c
  ${MOUNT_DIR}/libs/zlib/deflate.c
  ${MOUNT_DIR}/libs/zlib/gzio.c
  ${MOUNT_DIR}/libs/zlib/infback.c
  ${MOUNT_DIR}/libs/zlib/inffast.c
  ${MOUNT_DIR}/libs/zlib/inflate.c
  ${MOUNT_DIR}/libs/zlib/inftrees.c
  ${MOUNT_DIR}/libs/zlib/ioapi.c
  ${MOUNT_DIR}/libs/zlib/trees.c
  ${MOUNT_DIR}/libs/zlib/uncompr.c
  ${MOUNT_DIR}/libs/zlib/zutil.c
  ${MOUNT_DIR}/tools/owmap/brush.c
  ${MOUNT_DIR}/tools/owmap/fog.c
  ${MOUNT_DIR}/tools/owmap/brush_primit.c
  ${MOUNT_DIR}/tools/owmap/bsp.c
  ${MOUNT_DIR}/tools/owmap/bspfile_abstract.c
  ${MOUNT_DIR}/tools/owmap/bspfile_ibsp.c
  ${MOUNT_DIR}/tools/owmap/bspfile_rbsp.c
  ${MOUNT_DIR}/tools/owmap/bspfile_xbsp.c
  ${MOUNT_DIR}/tools/owmap/convert_ase.c
  ${MOUNT_DIR}/tools/owmap/convert_map.c
  ${MOUNT_DIR}/tools/owmap/decals.c
  ${MOUNT_DIR}/tools/owmap/facebsp.c
  ${MOUNT_DIR}/tools/owmap/gldraw.c
  ${MOUNT_DIR}/tools/owmap/model.c
  ${MOUNT_DIR}/tools/owmap/image.c
  ${MOUNT_DIR}/tools/owmap/leakfile.c
  ${MOUNT_DIR}/tools/owmap/light.c
  ${MOUNT_DIR}/tools/owmap/light_bounce.c
  ${MOUNT_DIR}/tools/owmap/light_trace.c
  ${MOUNT_DIR}/tools/owmap/light_ydnar.c
  ${MOUNT_DIR}/tools/owmap/lightmaps_ydnar.c
  ${MOUNT_DIR}/tools/owmap/main.c
  ${MOUNT_DIR}/tools/owmap/map.c
  ${MOUNT_DIR}/tools/owmap/mesh.c
  ${MOUNT_DIR}/tools/owmap/patch.c
  ${MOUNT_DIR}/tools/owmap/prtfile.c
  ${MOUNT_DIR}/tools/owmap/path_init.c
  ${MOUNT_DIR}/tools/owmap/portals.c
  ${MOUNT_DIR}/tools/owmap/shaders.c
  ${MOUNT_DIR}/tools/owmap/writebsp.c
  ${MOUNT_DIR}/tools/owmap/surface_abstract.c
  ${MOUNT_DIR}/tools/owmap/surface_extra.c
  ${MOUNT_DIR}/tools/owmap/surface_foliage.c
  ${MOUNT_DIR}/tools/owmap/surface_fur.c
  ${MOUNT_DIR}/tools/owmap/surface_meta.c
  ${MOUNT_DIR}/tools/owmap/tjunction.c
  ${MOUNT_DIR}/tools/owmap/tree.c
  ${MOUNT_DIR}/tools/owmap/vis.c
  ${MOUNT_DIR}/tools/owmap/visflow.c
)


####################
# End of file list #
####################

if( USE_PHYSICS )
  set( NEWTON_INCLUDES
    ${MOUNT_DIR}/libs/libnewton/include 
    ${MOUNT_DIR}/libs/libnewton/include/dAnimation 
    ${MOUNT_DIR}/libs/libnewton/include/dContainers 
    ${MOUNT_DIR}/libs/libnewton/include/dCustomJoints 
    ${MOUNT_DIR}/libs/libnewton/include/dMath
  )
endif()

if (WIN32)
	add_definitions(-DWINVER=0x501)
# 	set( OS_LIBRARIES m winmm wsock32 gdi32 ole32 ws2_32 psapi Iphlpapi advapi32 shell32 user32 )
	set( OS_LIBRARIES m winmm ws2_32 psapi )
	set( EXTRALIBS dinput dsound )
	set( OS_BUILD )
elseif (UNIX)
  if( CMAKE_SYSTEM_NAME STREQUAL "FreeBSD" )
    set( OS_LIBRARIES m z )
    set( OS_BUILD )
  elseif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set( OS_LIBRARIES dl m z )
    set( OS_LIBRARIES ${OS_LIBRARIES} "-framework AGL -framework OpenGL -framework Carbon -framework IOKit" )
	  set( OS_BUILD ${LINUXSOURCESLIST} )
  else()
    set( OS_LIBRARIES dl m z rt )
    set( OS_BUILD ${LINUXSOURCESLIST} )
  endif()
endif()

# GLEW
set(GLEW_INCLUDE_DIR
	"${MOUNT_DIR}/libs/glew/include")



###############################
# Build Game/Cgame/Ui DLL/.so #
###############################

  if( BUILD_GAME_SO_ET )
    add_library( game  SHARED ${SHAREDLIST} ${QAGAMELIST} )
    set_property( TARGET game PROPERTY COMPILE_DEFINITIONS GAME GAMEDLL USE_REFENTITY_ANIMATIONSYSTEM )
    set_target_properties( game PROPERTIES
      OUTPUT_NAME "qagame.mp.${BUILD_ARCH}"
      PREFIX ""
      LIBRARY_OUTPUT_DIRECTORY "etmain" )
    add_library( cgame SHARED ${SHAREDLIST} ${CGAMELIST} )
    set_property( TARGET cgame PROPERTY COMPILE_DEFINITIONS CGAME CGAMEDLL USE_REFENTITY_ANIMATIONSYSTEM )
    set_target_properties( cgame PROPERTIES
      OUTPUT_NAME "cgame.mp.${BUILD_ARCH}"
      PREFIX "" 
      LIBRARY_OUTPUT_DIRECTORY "etmain" )
    add_library( ui SHARED ${SHAREDLIST} ${UILIST} )
    set_property( TARGET ui PROPERTY COMPILE_DEFINITIONS UI UIDLL USE_REFENTITY_ANIMATIONSYSTEM )
    set_target_properties( ui PROPERTIES
      OUTPUT_NAME "ui.mp.${BUILD_ARCH}"
      PREFIX ""
      LIBRARY_OUTPUT_DIRECTORY "etmain" )
  install(TARGETS game cgame ui
          RUNTIME DESTINATION bin
          LIBRARY DESTINATION lib
          ARCHIVE DESTINATION lib
  )
  endif()

  if( BUILD_GAME_SO_TREM )
    add_library( tremgame  SHARED ${GAMETREM} )
    set_property( TARGET tremgame PROPERTY COMPILE_DEFINITIONS GAME GAMEDLL USE_REFENTITY_ANIMATIONSYSTEM )
    set_target_properties( tremgame PROPERTIES
      OUTPUT_NAME "qagame.mp.${BUILD_ARCH}"
      PREFIX ""
      LIBRARY_OUTPUT_DIRECTORY "base" )
    add_library( cgametrem SHARED ${CGAMETREM} )
    set_property( TARGET cgametrem PROPERTY COMPILE_DEFINITIONS CGAME CGAMEDLL USE_REFENTITY_ANIMATIONSYSTEM )
    set_target_properties( cgametrem PROPERTIES
      OUTPUT_NAME "cgame.mp.${BUILD_ARCH}"
      PREFIX "" 
      LIBRARY_OUTPUT_DIRECTORY "base" )
  add_library( uitrem SHARED ${UITREM} )
    set_property( TARGET uitrem PROPERTY COMPILE_DEFINITIONS UI UIDLL USE_REFENTITY_ANIMATIONSYSTEM )
    set_target_properties( uitrem PROPERTIES
      OUTPUT_NAME "ui.mp.${BUILD_ARCH}"
      PREFIX ""
      LIBRARY_OUTPUT_DIRECTORY "base" )
  endif()

 if( BUILD_GAME_SO_GPP )
    add_library( gppgame  SHARED ${GAMEGPP} )
    set_property( TARGET gppgame PROPERTY COMPILE_DEFINITIONS GAME GAMEDLL GPP USE_REFENTITY_ANIMATIONSYSTEM )
    set_target_properties( gppgame PROPERTIES
      OUTPUT_NAME "qagame.mp.${BUILD_ARCH}"
      PREFIX ""
      LIBRARY_OUTPUT_DIRECTORY "main" )
    add_library( gpptrem SHARED ${CGAMEGPP} )
    set_property( TARGET gpptrem PROPERTY COMPILE_DEFINITIONS CGAME CGAMEDLL GPP USE_REFENTITY_ANIMATIONSYSTEM )
    set_target_properties( gpptrem PROPERTIES
      OUTPUT_NAME "cgame.mp.${BUILD_ARCH}"
      PREFIX "" 
      LIBRARY_OUTPUT_DIRECTORY "main" )
  add_library( gppui SHARED ${UIGPP} )
    set_property( TARGET gppui PROPERTY COMPILE_DEFINITIONS UI UIDLL GPP USE_REFENTITY_ANIMATIONSYSTEM )
    set_target_properties( gppui PROPERTIES
      OUTPUT_NAME "ui.mp.${BUILD_ARCH}"
      PREFIX ""
      LIBRARY_OUTPUT_DIRECTORY "main" )
  endif()

###################
# Build Renderers #
###################

# set( GLEW_LIBRARY CACHE FILEPATH "Location of the GLEW Library" )
# set( MYSQL_LIBRARY CACHE FILEPATH "Location of the MYSQL library" )
# set( MYSQL_INCLUDES CACHE PATH ADVANCED "Location of the MYSQL includes" )
# set( GMP_LIBRARY CACHE FILEPATH "Location of the GMP Library" )
# set( GMP_INCLUDES CACHE PATH ADVANCED "Location of the GMP Includes" )
# set( XVID_LIBRARY CACHE FILEPATH "Location of the XviD library" )

if( USE_WEBP )
  find_package( WebP REQUIRED )
endif()

if( USE_PHYSICS )
  if( LINUX )
    if( ${CMAKE_SYSTEM_PROCESSOR} MATCHES "x86_64" )
      set( NEWTON_LIBRARY CACHE FILEPATH "${MOUNT_DIR}/libs/libnewton/libs/linux64/libNewton.a" )
    elseif(${CMAKE_SYSTEM_PROCESSOR} MATCHES "i[3-6]86")
      set( NEWTON_LIBRARY CACHE FILEPATH "${MOUNT_DIR}/libs/libnewton/libs/linux32/libNewton.a" )
    endif()
  else()
    set( NEWTON_LIBRARY CACHE FILEPATH "Location of the Newton library" )
  endif()
endif()
if( BUILD_OLD_RENDERER OR BUILD_NEW_RENDERER )
  
  find_package( OpenGL REQUIRED )
  if( USE_SYSTEM_HEADERS )
    find_package( SDL REQUIRED )
    find_package( JPEG  REQUIRED )
    find_package( PNG  REQUIRED )
    find_package( Freetype REQUIRED )
    find_package( GLEW REQUIRED )
    set( GLEW_INCLUDE_DIR ${MOUNT_DIR}/libs/glew/include )
  else()
    set( PNG_PNG_INCLUDE_DIR ${MOUNT_DIR}/libs/libpng )
    set( SDL_INCLUDE_DIR ${MOUNT_DIR}/libs/libsdl/include )
    set( JPEG_INCLUDE_DIR ${MOUNT_DIR}/libs/jpeg )
    set( FREETYPE_INCLUDE_DIRS ${MOUNT_DIR}/libs/freetype/include/freetype2 )
    set( GLEW_INCLUDE_DIR ${MOUNT_DIR}/libs/glew/include )
    mark_as_advanced( PNG_LIBRARY )
    set( PNG_LIBRARY CACHE FILEPATH "Location of the PNG library" )
    mark_as_advanced( FREETYPE_LIBRARY )
    set( FREETYPE_LIBRARY CACHE FILEPATH "Location of the Freetype library" )
    mark_as_advanced( FORCE GLEW_LIBRARY )
    mark_as_advanced( SDL_LIBRARY )
    set( SDL_LIBRARY CACHE FILEPATH "Location of the SDL Library" )
  endif()

  if( BUILD_NEW_RENDERER )
    add_library( rendererGL SHARED  ${SHAREDLIST} ${OPENEXRLIST} ${RENDERERGLLIST} ${MOUNT_DIR}/engine/sys/sdl_glimp.c ${MOUNT_DIR}/engine/sys/sdl_gamma.c ${MOUNT_DIR}/libs/libpng/pngerror.c ${MOUNT_DIR}/libs/jpeg/jdatasrc.c )
    include_directories( ${PNG_PNG_INCLUDE_DIR} ${OPENGL_INCLUDE_DIR} ${JPEG_INCLUDE_DIR} ${FREETYPE_INCLUDE_DIRS} ${GLEW_INCLUDE_DIR} ${SDL_INCLUDE_DIR} ${WEBP_INCLUDE_DIR} )
    set_property( TARGET rendererGL PROPERTY COMPILE_DEFINITIONS BUILD_FREETYPE FT_FREETYPE USE_XREAL_RENDERER USE_REFENTITY_ANIMATIONSYSTEM )
    target_link_libraries( rendererGL ${OS_LIBRARIES} ${WEBP_LIBRARY} ${PNG_LIBRARY} ${GLEW_LIBRARY} ${OPENGL_LIBRARY} ${JPEG_LIBRARY} ${FREETYPE_LIBRARY} ${SDL_LIBRARY} )
    set_target_properties( rendererGL PROPERTIES OUTPUT_NAME "librendererGL3${BUILD_ARCH}" PREFIX "" )
  endif()

  if( BUILD_OLD_RENDERER )
    add_library( renderer SHARED ${RENDERERLIST} ${MOUNT_DIR}/engine/sys/sdl_glimp.c ${MOUNT_DIR}/engine/sys/sdl_gamma.c ${SHAREDLIST} )
    include_directories( ${PNG_PNG_INCLUDE_DIR} ${OPENGL_INCLUDE_DIR} ${JPEG_INCLUDE_DIR} ${FREETYPE_INCLUDE_DIRS} ${GLEW_INCLUDE_DIR} ${SDL_INCLUDE_DIR} ${WEBP_INCLUDE_DIR} )
    set_property( TARGET renderer PROPERTY COMPILE_DEFINITIONS BUILD_FREETYPE USE_REFENTITY_ANIMATIONSYSTEM )
    target_link_libraries( renderer ${PNG_LIBRARY} ${WEBP_LIBRARY} ${SDL_LIBRARY} ${GLEW_LIBRARY} ${OPENGL_LIBRARY} ${JPEG_LIBRARY} ${FREETYPE_LIBRARY} )
    set_target_properties( renderer PROPERTIES OUTPUT_NAME "librendererGL${BUILD_ARCH}" PREFIX "" )
  endif()

  #Enable WEBP in code if WEBP is found
  if( WEBP_FOUND )
    set_property( TARGET renderer APPEND PROPERTY COMPILE_DEFINITIONS USE_WEBP )
  endif( )

endif()

################
# Build Client #
################

if( BUILD_CLIENT )
  find_package( OpenGL REQUIRED )
  if( USE_CURL )
    find_package( CURL REQUIRED )
  endif()
  if( USE_SYSTEM_HEADERS )
    find_package( JPEG )
    find_package( SDL REQUIRED )
    find_package( PNG )
    find_package( Freetype REQUIRED )
    find_package( GLEW REQUIRED )
    find_package( GMP REQUIRED )
  else() 
    set( PNG_PNG_INCLUDE_DIR ${MOUNT_DIR}/libs/libpng )
    set( SDL_INCLUDE_DIR ${MOUNT_DIR}/libs/libsdl/include )
    set( JPEG_INCLUDE_DIR ${MOUNT_DIR}/libs/jpeg )
    set( FREETYPE_INCLUDE_DIRS ${MOUNT_DIR}/libs/freetype/include/freetype2 )
    set( GLEW_INCLUDE_DIR ${MOUNT_DIR}/libs/glew/include )
endif()
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  link_directories( ${CMAKE_BINARY_DIR} /opt/local/lib )
else()
  link_directories( ${CMAKE_BINARY_DIR})
endif()

  if( NOT USE_INTERNAL_ZLIB )
    find_package(ZLIB REQUIRED)
    set( ZLIBLIST )
  endif()

  add_executable( client ${SERVERLIST} ${CLIENTLIST} ${SDLBASELIST}
      ${CPUINFOLIST} ${SDLLIST} ${OS_BUILD} ${QCOMMONLIST} ${SHAREDLIST}
      ${BOTLIBLIST} ${ZLIBLIST} ${DATABASELIST} ${MOUNT_DIR}/engine/qcommon/dl_main_curl.c ${SPEEXLIST} ${CRYPTOLIST} )

if( USE_OPENAL )
  find_package( OpenAL REQUIRED )
  add_definitions( -DUSE_OPENAL )
  if( USE_OPENAL_DLOPEN )
    add_definitions( -DUSE_OPENAL_DLOPEN )
  endif()
endif()

if( USE_CURL )
  find_package( CURL REQUIRED )
  add_definitions( -DUSE_CURL )
  if( USE_CURL_DLOPEN )
    add_definitions( -DUSE_CURL_DLOPEN )
  endif()
endif()

if( USE_CURSES )
  set( CURSES_NEED_NCURSES 1 )
  find_package( Curses REQUIRED )
  add_definitions( -DUSE_CURSES )
endif()

if( USE_VOIP )
  add_definitions( -DUSE_VOIP -DFLOATING_POINT )
endif()

if( USE_CODEC_VORBIS )
  find_package( Vorbis REQUIRED )
  find_package( Ogg REQUIRED )
  add_definitions( -DUSE_CODEC_VORBIS )
endif()



if( USE_CIN_THEORA )
  find_package( Theora REQUIRED )
  if( ${VORBIS_FOUND} AND ${THEORA_FOUND} )
    add_definitions( -DUSE_CIN_THEORA )
  else()
    message( FATAL_ERROR "Vorbis must be enabled to use CIN_THEORA" )
  endif()
endif()
     
 if( USE_MUMBLE )
   add_definitions( -DUSE_MUMBLE )
 endif()
 
  if( USE_PHYSICS )
    find_package( TinyXML REQUIRED )
    add_definitions( -DUSE_PHYSICS )
    add_dependencies( client newton )
    target_link_libraries( client ${NEWTON_LIBRARY} )
  endif( )
    
  add_dependencies( client rendererGL )
  include_directories( ${MOUNT_DIR}/libs/cpuinfo ${MOUNT_DIR}/libs/nettle ${MOUNT_DIR}/libs/gmp/include ${MOUNT_DIR}/libs/libspeex/include ${MOUNT_DIR}/libs/libspeex/libspeex ${PNG_PNG_INCLUDE_DIR} ${SDL_INCLUDE_DIR} ${OPENGL_INCLUDE_DIR} ${JPEG_INCLUDE_DIR} ${FREETYPE_INCLUDE_DIRS} ${GLEW_INCLUDE_DIR} ${CURL_INCLUDE_DIR} ${VORBIS_INCLUDE_DIR} ${OGG_INCLUDE_DIR} ${THEORA_INCLUDE_DIR} ${XML_INCLUDE_DIR} ${CURSES_INCLUDE_DIR} ${MYSQL_INCLUDE_DIR} ${GMP_INCLUDES} ${NEWTON_INCLUDES} ${LIB_DIR}/xvidcore/include )
  target_link_libraries( client ${OS_LIBRARIES} ${PNG_LIBRARY} ${SDL_LIBRARY} ${OPENGL_LIBRARY} ${JPEG_LIBRARY} ${FREETYPE_LIBRARY} ${GLEW_LIBRARY} ${CURL_LIBRARY} ${VORBIS_LIBRARY} ${OGG_LIBRARY} ${THEORA_LIBRARY} ${XML_LIBRARY} ${NEWTON_LIBRARY} ${MYSQL_LIBRARIES} ${CURSES_LIBRARY} ${GMP_LIBRARY} ${XVID_LIBRARY} )
  set_property( TARGET client PROPERTY COMPILE_DEFINITIONS USE_XREAL_RENDERER BOTLIB BOTLIB_STATIC COMPAT_ET USE_REFENTITY_ANIMATIONSYSTEM USE_CRYPTO )
  set_target_properties( client PROPERTIES OUTPUT_NAME "daemon.${BUILD_ARCH}" PREFIX "" LINKER_LANGUAGE CXX )
  install(TARGETS client
          RUNTIME DESTINATION bin
          LIBRARY DESTINATION lib
          ARCHIVE DESTINATION lib
  )
endif()

################
# Build OpenAL #
################

if( USE_OPENAL )
  find_package( OpenAL REQUIRED )
  add_library( al SHARED ${ALLIST} )
  target_link_libraries( al ${OPENAL_LIBRARY} )
  set_property( TARGET al PROPERTY COMPILE_DEFINITIONS )
  set_target_properties( al PROPERTIES OUTPUT_NAME "snd_openal" PREFIX "" )
endif()

################
# Build Server #
################

if( BUILD_SERVER )
  if( USE_CURL )
    find_package( CURL REQUIRED )
  endif()
  if( USE_SYSTEM_HEADERS )
    find_package( SDL REQUIRED )
  else() 
    set( SDL_INCLUDE_DIR ${MOUNT_DIR}/libs/libsdl/include )
  endif()
  link_directories( ${CMAKE_BINARY_DIR} )
  
  if( NOT USE_INTERNAL_ZLIB )
    find_package(ZLIB REQUIRED)
    set( ZLIBLIST )
  endif()

  add_executable( server ${SERVERLIST}  ${SDLBASELIST}
      ${CPUINFOLIST} ${QCOMMONLIST} ${SHAREDLIST}
      ${BOTLIBLIST} ${SPEEXLIST} ${ZLIBLIST} ${DATABASELIST} ${CRYPTOLIST} ${MOUNT_DIR}/engine/null/null_client.c ${MOUNT_DIR}/engine/null/null_input.c ${MOUNT_DIR}/engine/null/null_snddma.c ${MOUNT_DIR}/engine/qcommon/dl_main_stubs.c )

if( USE_CURL )
  find_package( CURL REQUIRED )
  add_definitions( -DUSE_CURL )
  if( USE_CURL_DLOPEN )
    add_definitions( -DUSE_CURL_DLOPEN )
  endif()
endif()

if( USE_CURSES )
  set( CURSES_NEED_NCURSES 1 )
  find_package( Curses REQUIRED )
  add_definitions( -DUSE_CURSES )
endif()

if( USE_VOIP )
  add_definitions( -DUSE_VOIP -DFLOATING_POINT )
endif()
 
  if( USE_PHYSICS )
    pkg_check_modules( XML tinyxml )
    if( XML_FOUND )
      add_definitions( -DUSE_PHYSICS )
    else( )
      set( XML_LIBRARY )
      set( XML_INCLUDE_DIR )
      set( NEWTON_LIBRARY )
      message( FATAL_ERROR "tinyxml is required to use physics but could not be found" )
    endif( )
  endif( )

  include_directories( ${MOUNT_DIR}/libs/cpuinfo ${MOUNT_DIR}/libs/nettle ${MOUNT_DIR}/libs/gmp/include ${MOUNT_DIR}/libs/libspeex/include ${MOUNT_DIR}/libs/libspeex/libspeex ${SDL_INCLUDE_DIR} ${CURL_INCLUDE_DIR} ${XML_INCLUDE_DIR} ${CURSES_INCLUDE_DIR} ${GMP_INCLUDES} ${NEWTON_INCLUDES} )
  target_link_libraries( server ${OS_LIBRARIES} ${SDL_LIBRARY} ${CURL_LIBRARY} ${XML_LIBRARY} ${NEWTON_LIBRARY} ${MYSQL_LIBRARY} ${CURSES_LIBRARY} ${GMP_LIBRARY} ${MYSQL_LIBRARIES} )
  set_property( TARGET server PROPERTY COMPILE_DEFINITIONS DEDICATED BOTLIB BOTLIB_STATIC COMPAT_ET USE_CRYPTO )
  set_target_properties( server PROPERTIES OUTPUT_NAME "daemonded.${BUILD_ARCH}" PREFIX "" LINKER_LANGUAGE CXX )
  install(TARGETS server
          RUNTIME DESTINATION bin
          LIBRARY DESTINATION lib
          ARCHIVE DESTINATION lib
  )
endif()

################
# Build OWMap  #
################
if( BUILD_OWMAP )
 find_package( OpenGL REQUIRED )
  if( USE_SYSTEM_HEADERS )
    find_package( SDL REQUIRED )
    find_package( JPEG  REQUIRED )
    find_package( PNG  REQUIRED )
  endif()
  add_executable( owmap ${OWMAPLIST} )
  pkg_check_modules( GLIB glib-2.0 )
  if( NOT GLIB_FOUND )
    message( FATAL_ERROR "Glib not found" )
  endif()
  include_directories( ${LIB_DIR}/picomodel ${LIB_DIR}/jpeg ${LIB_DIR}/zlib ${MOUNT_DIR}/tools/common ${GLIB_INCLUDE_DIRS} ${PNG_PNG_INCLUDE_DIR} ${SDL_INCLUDE_DIR} ${JPEG_INCLUDE_DIR} ${OPENGL_INCLUDE_DIR} )
  target_link_libraries( owmap ${OS_LIBRARIES} ${SDL_LIBRARY} ${GLIB_LIBRARIES} ${PNG_LIBRARY} ${JPEG_LIBRARY} ${OPENGL_LIBRARY} )
  set_property( TARGET owmap PROPERTY COMPILE_DEFINITIONS USE_OPENGL )
  set_target_properties( owmap PROPERTIES OUTPUT_NAME "daemonmap.${BUILD_ARCH}" PREFIX "" )
  install(TARGETS owmap
	    RUNTIME DESTINATION bin
	    LIBRARY DESTINATION lib
	    ARCHIVE DESTINATION lib
    )
endif()
