dnl Process this file with autoconf to produce a configure script

dnl ------------------------------------------------
dnl Initialization and Versioning
dnl ------------------------------------------------

AC_INIT(libtheora,[1.2.0alpha1+svn])

AC_CANONICAL_HOST

AM_CONFIG_HEADER([config.h])
AC_CONFIG_SRCDIR([lib/fdct.c])
AM_INIT_AUTOMAKE

dnl we use doc_DATA in doc/Makefile.am which requires autoconf >= 2.60
dnl to define docdir for us.
AC_PREREQ(2.60)

m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

dnl Library versioning
dnl CURRENT, REVISION, AGE
dnl - library source changed -> increment REVISION
dnl - interfaces added/removed/changed -> increment CURRENT, REVISION = 0
dnl - interfaces added -> increment AGE
dnl - interfaces removed -> AGE = 0

TH_LIB_CURRENT=4
TH_LIB_REVISION=0
TH_LIB_AGE=4
AC_SUBST(TH_LIB_CURRENT)
AC_SUBST(TH_LIB_REVISION)
AC_SUBST(TH_LIB_AGE)

THDEC_LIB_CURRENT=2
THDEC_LIB_REVISION=5
THDEC_LIB_AGE=1
AC_SUBST(THDEC_LIB_CURRENT)
AC_SUBST(THDEC_LIB_REVISION)
AC_SUBST(THDEC_LIB_AGE)

THENC_LIB_CURRENT=3
THENC_LIB_REVISION=0
THENC_LIB_AGE=2
AC_SUBST(THENC_LIB_CURRENT)
AC_SUBST(THENC_LIB_REVISION)
AC_SUBST(THENC_LIB_AGE)

dnl Extra linker options (for version script)
THEORA_LDFLAGS=""

dnl --------------------------------------------------  
dnl Check for programs
dnl --------------------------------------------------  

AM_PROG_AS

dnl save $CFLAGS since AC_PROG_CC likes to insert "-g -O2"
dnl if $CFLAGS is blank
cflags_save="$CFLAGS"
AC_PROG_CC
AC_PROG_CPP
CFLAGS="$cflags_save"

AM_PROG_CC_C_O
AC_LIBTOOL_WIN32_DLL
AM_PROG_LIBTOOL

dnl Add parameters for aclocal
AC_CONFIG_MACRO_DIR([m4])

dnl Check for doxygen
AC_ARG_ENABLE([doc],
	AS_HELP_STRING([--disable-doc], [Do not build API documentation]),
	[ac_enable_doc=$enableval], [ac_enable_doc=auto])

if test "x$ac_enable_doc" != "xno"; then
	AC_CHECK_PROG(HAVE_DOXYGEN, doxygen, true, false)

	if test "x$HAVE_DOXYGEN" = "xfalse" -a "x$ac_enable_doc" = "xyes"; then
		AC_MSG_ERROR([*** API documentation explicitly requested but Doxygen not found])
	fi
else
	HAVE_DOXYGEN=false
fi
AM_CONDITIONAL(HAVE_DOXYGEN,$HAVE_DOXYGEN)
if test $HAVE_DOXYGEN = "false"; then
        AC_MSG_WARN([*** doxygen not found, API documentation will not be built])
fi

dnl Check for tools used to build the format specification
BUILD_SPEC="false"
ac_build_spec=yes
AC_ARG_ENABLE(spec,
     AS_HELP_STRING([--disable-spec], [Do not build the specification]),
     [
       if test "x$enableval" = "xno"; then
         ac_build_spec=$enableval
       fi
     ], [
       ac_build_spec=yes
     ] )
if test "x$ac_build_spec" = "xyes"; then
  AC_CHECK_PROG(HAVE_PDFLATEX, pdflatex, yes)
  AC_CHECK_PROG(HAVE_BIBTEX, bibtex, yes)
  AC_CHECK_PROG(HAVE_TRANSFIG, fig2dev, yes)
  if test -r doc/spec/spec.tex; then
   if test "x$HAVE_PDFLATEX" = "xyes"; then
    if test "x$HAVE_BIBTEX" = "xyes"; then
      if test "x$HAVE_TRANSFIG" = "xyes"; then
        tex_pkg_list=`fgrep usepackage doc/spec/spec.tex | grep \{ | grep -v ltablex`
        tex_pkg_ok="yes"
        for pkg_line in $tex_pkg_list; do
	  pkg_name=`echo $pkg_line | sed -e 's/.*{\(.*\)}.*/\1/'`
	  AC_MSG_CHECKING([for Tex package $pkg_name])
          cat >conftest.tex <<_ACEOF
\\documentclass{book}
$pkg_line
\\begin{document}
Hello World.
\\end{document}
_ACEOF
	  if pdflatex -interaction batchmode -halt-on-error conftest < /dev/null > /dev/null 2>&1; then
            AC_MSG_RESULT([ok])
          else
	    tex_pkg_ok="no"
	    AC_MSG_RESULT([no])
          fi
        done
        if test -w conftest.tex; then rm conftest.tex; fi
        if test -w conftest.tex; then rm conftest.aux; fi
        if test -w conftest.pdf; then rm conftest.pdf; fi
        if test "x$tex_pkg_ok" = "xyes"; then
          BUILD_SPEC="true"
        fi
      fi
    fi
   fi
  fi
fi
AM_CONDITIONAL(BUILD_SPEC, $BUILD_SPEC)
if test $BUILD_SPEC = "false"; then
	AC_MSG_WARN([*** Format Specification will not built.])
fi

dnl Check for valgrind
VALGRIND_ENVIRONMENT=""
ac_enable_valgrind=no
AC_ARG_ENABLE(valgrind-testing,
     AS_HELP_STRING([--enable-valgrind-testing], [Enable running of tests inside Valgrind]),
     [ ac_enable_valgrind=$enableval ], [ ac_enable_valgrind=no] )

if test "x${ac_enable_valgrind}" = xyes ; then
  if test "x${enable_shared}" = xyes ; then
    VALGRIND_ENVIRONMENT="libtool --mode=execute "
  fi

  AC_CHECK_PROG(HAVE_VALGRIND, valgrind, yes, no)
  if test "x$HAVE_VALGRIND" = xyes ; then
    VALGRIND_ENVIRONMENT="$VALGRIND_ENVIRONMENT valgrind -q --leak-check=yes --show-reachable=yes --num-callers=100"
    AC_SUBST(VALGRIND_ENVIRONMENT)
    TESTS_INFO="Type 'make check' to run test suite. Tests will be run under:
  ${VALGRIND_ENVIRONMENT}"
  else
    TESTS_INFO="Type 'make check' to run test suite (Valgrind not found)"
  fi
else
  TESTS_INFO="Type 'make check' to run test suite (Valgrind testing not enabled)"
fi

dnl --------------------------------------------------
dnl Set build flags based on environment
dnl --------------------------------------------------

dnl Set some host options

cflags_save="$CFLAGS"
if test -z "$GCC"; then
        case $host in 
        *)
                DEBUG="-g -DDEBUG"
                CFLAGS="-O"
                PROFILE="-g -p -DDEBUG" ;;
        esac
else

        case $host in 
        *)
                DEBUG="-g -Wall -Wno-parentheses -DDEBUG -D__NO_MATH_INLINES"
                CFLAGS="-Wall -Wno-parentheses -O3 -fforce-addr -fomit-frame-pointer -finline-functions -funroll-loops"
                PROFILE="-Wall -Wno-parentheses -pg -g -O3 -fno-inline-functions -DDEBUG";;
        esac
fi
CFLAGS="$CFLAGS $cflags_save"

cpu_x86_64=no
cpu_x86_32=no
cpu_arm=no
cpu_c64x=no
AC_ARG_ENABLE(asm,
    AS_HELP_STRING([--disable-asm], [Disable assembly optimizations]),
    [ ac_enable_asm=$enableval ], [ ac_enable_asm=yes] )

if test "x${ac_enable_asm}" = xyes; then
  cpu_optimization="no optimization for your platform, please send a patch"
  case $host_cpu in
  i[[3456]]86)
    cpu_x86_32=yes
    cpu_optimization="32 bit x86"
    AC_DEFINE([OC_X86_ASM], [],  [make use of x86 asm optimization])
    if test "x$host_vendor" = "xapple"; then
      THEORA_LDFLAGS="$THEORA_LDFLAGS  -Wl,-read_only_relocs,suppress"
    fi
      ;;
  x86_64)
    cpu_x86_64=yes
    cpu_optimization="64 bit x86"
    AC_DEFINE([OC_X86_ASM], [],  [make use of x86 asm optimization])
    AC_DEFINE([OC_X86_64_ASM], [],  [make use of x86_64 asm optimization])
    ;;
  arm*)
    cpu_arm=yes
    cpu_optimization="ARM"
    AC_DEFINE([OC_ARM_ASM], [],  [make use of arm asm optimization])
    AC_ARG_ENABLE(asflag-probe,
        AS_HELP_STRING([--disable-asflag-probe], [Disable instructions not supported by the default ASFLAGS (ARM only).]),
        [ ac_enable_asflag_probe=$enableval ], [ ac_enable_asflag_probe=yes] )

    dnl our ARM assembly requires perl to run the arm2gnu reformatter
    AC_CHECK_PROG([HAVE_PERL], perl, yes, no)
    if test "x$HAVE_PERL" = "xno"; then
      AC_MSG_WARN([*** ARM assembly requires perl -- disabling optimizations])
      cpu_arm=no
      cpu_optimization="(missing perl dependency for ARM)"
    fi

    dnl AC_TRY_ASSEMBLE uses CFLAGS instead of CCASFLAGS
    save_CFLAGS="$CFLAGS"
    ARM_CCASFLAGS=
    dnl Test for instruction set support with the default CCASFLAGS.
    AS_ASM_ARM_NEON([HAVE_ARM_ASM_NEON=1],[HAVE_ARM_ASM_NEON=0])
    AS_ASM_ARM_MEDIA([HAVE_ARM_ASM_MEDIA=1],[HAVE_ARM_ASM_MEDIA=0])
    AS_ASM_ARM_EDSP([HAVE_ARM_ASM_EDSP=1],[HAVE_ARM_ASM_EDSP=0])
    dnl gas will not assemble instructions unless the architecture explicitly
    dnl  supports it (unlike on x86).
    dnl Try to speculatively add ASFLAGS to enable usage of these instructions
    dnl  at assembly time (actual support is detected at runtime).
    dnl If the user has already specified -march or -mcpu flags, this may give
    dnl  some spurious warnings (use --disable-asflag-probe to avoid this if
    dnl  you don't want run-time support for instructions not available on the
    dnl  architecture you specified).
    dnl Order here is important.
    if test "x${ac_enable_asflag_probe}" = xyes; then
      if test x$HAVE_ARM_ASM_NEON != x1 ; then
        dnl Try to set some flags to enable NEON instructions.
        AC_MSG_NOTICE([trying custom CCASFLAGS to enable NEON instructions...])
        ARM_CCASFLAGS="-mfpu=neon -march=armv7-a"
        CFLAGS="$save_CFLAGS $ARM_CCASFLAGS"
        AS_ASM_ARM_NEON([HAVE_ARM_ASM_NEON=1],[HAVE_ARM_ASM_NEON=0])
        if test x$HAVE_ARM_ASM_NEON != x1 ; then
          ARM_CCASFLAGS=
          CFLAGS="$save_CFLAGS"
        fi
      fi
      if test x$HAVE_ARM_ASM_MEDIA != x1 ; then
        dnl Try to set some flags to enable ARMv6 media instructions.
        AC_MSG_NOTICE([trying custom CCASFLAGS to enable ARMv6 media instructions...])
        ARM_CCASFLAGS="-march=armv6j"
        CFLAGS="$save_CFLAGS $ARM_CCASFLAGS"
        AS_ASM_ARM_MEDIA([HAVE_ARM_ASM_MEDIA=1],[HAVE_ARM_ASM_MEDIA=0])
        if test x$HAVE_ARM_ASM_MEDIA != x1 ; then
          ARM_CCASFLAGS=
          CFLAGS="$save_CFLAGS"
        fi
      fi
      if test x$HAVE_ARM_ASM_EDSP != x1 ; then
        dnl Try to set some flags to enable EDSP instructions.
        AC_MSG_NOTICE([trying custom CCASFLAGS to enable EDSP compilation...])
        ARM_CCASFLAGS="-march=armv5e"
        CFLAGS="$save_CFLAGS $ARM_CCASFLAGS"
        AS_ASM_ARM_EDSP([HAVE_ARM_ASM_EDSP=1],[HAVE_ARM_ASM_EDSP=0])
        if test x$HAVE_ARM_ASM_MEDIA != x1 ; then
          ARM_CCASFLAGS=
          CFLAGS="$save_CFLAGS"
        fi
      fi
    fi

    dnl Only enable if we passed the perl test above
    if test x$cpu_arm = xyes; then
      if test x$HAVE_ARM_ASM_EDSP = x1 ; then
        AC_DEFINE(OC_ARM_ASM_EDSP, 1,
          [Define if assembler supports EDSP instructions])
        cpu_optimization="$cpu_optimization (EDSP)"
      fi
      AC_SUBST(HAVE_ARM_ASM_EDSP)
      if test x$HAVE_ARM_ASM_MEDIA = x1 ; then
        AC_DEFINE(OC_ARM_ASM_MEDIA, 1,
          [Define if assembler supports ARMv6 media instructions])
        cpu_optimization="$cpu_optimization (Media)"
      fi
      AC_SUBST(HAVE_ARM_ASM_MEDIA)
      if test x$HAVE_ARM_ASM_NEON = x1 ; then
        AC_DEFINE(OC_ARM_ASM_NEON, 1,
          [Define if compiler supports NEON instructions])
        cpu_optimization="$cpu_optimization (NEON)"
      fi
      AC_SUBST(HAVE_ARM_ASM_NEON)
    fi

    CFLAGS="$save_CFLAGS"
    CCASFLAGS="$CCASFLAGS $ARM_CCASFLAGS"
    ;;
  tic6x)
    cpu_c64x=yes
    cpu_optimization="TI C64x+"
    AC_DEFINE([OC_C64X_ASM], [],  [make use of c64x+ asm optimization])
    ;;
  esac
else
  cpu_optimization="disabled"
fi
AM_CONDITIONAL([CPU_x86_64], [test x$cpu_x86_64 = xyes])
AM_CONDITIONAL([CPU_x86_32], [test x$cpu_x86_32 = xyes])
AM_CONDITIONAL([CPU_arm], [test x$cpu_arm = xyes])
AM_CONDITIONAL([CPU_c64x], [test x$cpu_c64x = xyes])

# Test whenever ld supports -version-script
AC_PROG_LD
AC_PROG_LD_GNU
AC_MSG_CHECKING([how to control symbol export])

THDEC_VERSION_ARG=""
THENC_VERSION_ARG=""
TH_VERSION_ARG=""
if test "x$lt_cv_prog_gnu_ld" = "xyes"; then
 case "$host_os" in
  *mingw*)
    THEORA_LDFLAGS="$THEORA_LDFLAGS -no-undefined"
    THDEC_VERSION_ARG="-export-symbols \$(top_srcdir)/win32/xmingw32/libtheoradec-all.def"
    THENC_VERSION_ARG="-export-symbols \$(top_srcdir)/win32/xmingw32/libtheoraenc-all.def"
    THENC_VERSION_ARG="$THENC_VERSION_ARG -ltheoradec"
    THC_VERSION_ARG="-export-symbols \$(top_srcdir)/win32/libtheora.def"
    AC_MSG_RESULT([-export-symbols])
    ;;
  linux* | solaris* | gnu* | k*bsd*-gnu)
    THDEC_VERSION_ARG='-Wl,--version-script=$(srcdir)/Version_script-dec'
    THENC_VERSION_ARG='-Wl,--version-script=$(srcdir)/Version_script-enc'
    TH_VERSION_ARG='-Wl,--version-script=$(srcdir)/Version_script'
    AC_MSG_RESULT([--version-script])
    ;;
  *)
    # build without versioning
    AC_MSG_RESULT([no])
    ;;
 esac
else
 case "$host_os" in
  darwin*)
    THDEC_VERSION_ARG='-Wl,-exported_symbols_list,$(srcdir)/theoradec.exp'
    THENC_VERSION_ARG='-Wl,-exported_symbols_list,$(srcdir)/theoraenc.exp'
    TH_VERSION_ARG='-Wl,-exported_symbols_list,$(srcdir)/theora.exp'
    AC_MSG_RESULT([-exported_symbols_list])
    ;;
  *)
    # build without versioning
    AC_MSG_RESULT([no])
    ;;
 esac
fi
 
THEORADEC_LDFLAGS="$THEORA_LDFLAGS $THDEC_VERSION_ARG"
THEORAENC_LDFLAGS="$THEORA_LDFLAGS $THENC_VERSION_ARG"
THEORA_LDFLAGS="$THEORA_LDFLAGS $TH_VERSION_ARG"
AC_SUBST(THEORADEC_LDFLAGS)
AC_SUBST(THEORAENC_LDFLAGS)
AC_SUBST(THEORA_LDFLAGS)

dnl --------------------------------------------------
dnl Checks for support libraries and headers
dnl --------------------------------------------------

dnl check for Ogg
HAVE_OGG=no

dnl first check through pkg-config since it's more flexible

dnl check for pkg-config itself so we don't try the m4 macro without pkg-config
AC_CHECK_PROG(HAVE_PKG_CONFIG, pkg-config, yes)
if test "x$HAVE_PKG_CONFIG" = "xyes"
then
  PKG_CHECK_MODULES(OGG, ogg >= 1.1, HAVE_OGG=yes, HAVE_OGG=no)
fi
if test "x$HAVE_OGG" = "xno"
then
  dnl fall back to the old school test
  XIPH_PATH_OGG(, AC_MSG_ERROR([
    libogg is required to build this package!
    please see http://www.xiph.org/ for how to
    obtain a copy.
  ]))
  cflags_save=$CFLAGS
  libs_save=$LIBS
  CFLAGS="$CFLAGS $OGG_CFLAGS"
  LIBS="$LIBS $OGG_LIBS"
  AC_CHECK_FUNC(oggpackB_read, , [
    AC_MSG_ERROR([newer libogg version (1.1 or later) required])
  ])
  CFLAGS=$cflags_save
  LIBS=$libs_save
fi


dnl check for Vorbis
HAVE_VORBIS=no

dnl first check through pkg-config since it's more flexible

if test "x$HAVE_PKG_CONFIG" = "xyes"
then
  PKG_CHECK_MODULES(VORBIS, vorbis >= 1.0.1, HAVE_VORBIS=yes, HAVE_VORBIS=no)
  dnl also set VORBISENC_LIBS since an examples needs it
  dnl the old .m4 sets this to a value to use on top of VORBIS_LIBS,
  dnl so we do the same here.
  VORBISENC_LIBS="-lvorbisenc"
  AC_SUBST(VORBISENC_LIBS)
fi
if test "x$HAVE_VORBIS" = "xno"
then
  dnl fall back to the old school test
  XIPH_PATH_VORBIS(HAVE_VORBIS=yes, HAVE_VORBIS=no)
fi

dnl check for SDL
HAVE_SDL=no
if test "x$HAVE_PKG_CONFIG" = "xyes"
then
  PKG_CHECK_MODULES(SDL, sdl, HAVE_SDL=yes, [
    HAVE_SDL=no
    AC_MSG_WARN([*** Unable to find SDL -- Not compiling example players ***])])
fi

dnl check for OSS
HAVE_OSS=no
AC_CHECK_HEADERS([sys/soundcard.h soundcard.h machine/soundcard.h],[
  HAVE_OSS=yes
  break
])
if test x$HAVE_OSS != xyes; then
  AC_MSG_WARN([OSS audio support not found -- not compiling player_example])
fi

dnl OpenBSD needs -lossaudio to use the oss interface
OSS_LIBS=
case "$host_os" in
  openbsd*)
    OSS_LIBS='-lossaudio'
    ;;
esac
AC_SUBST(OSS_LIBS)

dnl check for libpng
HAVE_PNG=no
if test "x$HAVE_PKG_CONFIG" = "xyes"
then
  PKG_CHECK_MODULES(PNG, libpng, HAVE_PNG=yes, HAVE_PNG=no)
fi
AC_SUBST(PNG_CFLAGS)
AC_SUBST(PNG_LIBS)

dnl check for libtiff
HAVE_TIFF=no
TIFF_CFLAGS=''
TIFF_LIBS=''
AC_CHECK_LIB([tiff], [TIFFReadRGBAImage], [
  TIFF_LIBS='-ltiff'
  AC_CHECK_HEADER([tiffio.h], [
    HAVE_TIFF=yes
  ])
])
AC_SUBST(HAVE_TIFF)
AC_SUBST(TIFF_CFLAGS)
AC_SUBST(TIFF_LIBS)

dnl check for libcairo
HAVE_CAIRO=no
AC_ARG_ENABLE(telemetry,
    AS_HELP_STRING([--enable-telemetry], [Enable debugging output controls]),
    [ ac_enable_telemetry=$enableval ], [ ac_enable_telemetry=no] )

if test "x${ac_enable_telemetry}" = xyes; then
   if test "x$HAVE_PKG_CONFIG" = "xyes"
   then
     PKG_CHECK_MODULES(CAIRO, cairo, HAVE_CAIRO=yes, HAVE_CAIRO=no)
     AC_DEFINE([HAVE_CAIRO], [],  [libcairo is available for visual debugging output])  
   fi
   if test x$HAVE_CAIRO != xyes; then
     AC_MSG_WARN([libcairo not found -- not compiling telemetry output support ])
   fi
   AC_SUBST(CAIRO_CFLAGS)
   AC_SUBST(CAIRO_LIBS)
fi

dnl --------------------------------------------------
dnl Overall build configuration options
dnl --------------------------------------------------

dnl Configuration option for building of encoding support.

ac_enable_encode=yes
AC_ARG_ENABLE(encode,
     AS_HELP_STRING([--disable-encode], [Disable encoding support]),
     [ ac_enable_encode=$enableval ], [ ac_enable_encode=yes] )

if test "x${ac_enable_encode}" != xyes ; then
    AC_DEFINE([THEORA_DISABLE_ENCODE], [],
  [Define to exclude encode support from the build])
else
    if test x$HAVE_VORBIS = xyes; then
      BUILDABLE_EXAMPLES="$BUILDABLE_EXAMPLES encoder_example\$(EXEEXT)"
    else
      AC_MSG_NOTICE([Vorbis missing, cannot build example encoder])
    fi
fi
AM_CONDITIONAL(THEORA_DISABLE_ENCODE, [test "x${ac_enable_encode}" != xyes])

dnl Configuration option for examples

ac_enable_examples=yes
AC_ARG_ENABLE(examples,
     AS_HELP_STRING([--disable-examples], [Disable examples]),
     [ ac_enable_examples=$enableval ], [ ac_enable_examples=yes] )
AM_CONDITIONAL(THEORA_ENABLE_EXAMPLES, [test "x${ac_enable_examples}" != xno])

dnl --------------------------------------------------
dnl Check for headers
dnl --------------------------------------------------

dnl none here

dnl --------------------------------------------------
dnl Check for typedefs, structures, etc
dnl --------------------------------------------------

dnl none

dnl --------------------------------------------------
dnl Check for library functions
dnl --------------------------------------------------

dnl OpenBSD needs -lcompat for ftime() used by dump_video.c
AC_CHECK_LIB([compat], [ftime], [COMPAT_LIBS='-lcompat'])
AC_SUBST(COMPAT_LIBS)

dnl substitute the included getopt if the system doesn't support long options
AC_CHECK_FUNC(getopt_long,
              [GETOPT_OBJS=''],
              [GETOPT_OBJS='getopt.$(OBJEXT) getopt1.$(OBJEXT)'])
AC_SUBST(GETOPT_OBJS)

if test x$HAVE_SDL = xyes -a x$HAVE_OSS = xyes -a x$HAVE_VORBIS = xyes; then
  BUILDABLE_EXAMPLES="$BUILDABLE_EXAMPLES player_example\$(EXEEXT)"
fi
if test x$HAVE_PNG = xyes; then
  BUILDABLE_EXAMPLES="$BUILDABLE_EXAMPLES png2theora\$(EXEEXT)"
fi
if test x$HAVE_TIFF = xyes; then
  BUILDABLE_EXAMPLES="$BUILDABLE_EXAMPLES tiff2theora\$(EXEEXT)"
fi
AC_SUBST(BUILDABLE_EXAMPLES)

dnl --------------------------------------------------
dnl Do substitutions
dnl --------------------------------------------------

AC_SUBST(DEBUG)
AC_SUBST(PROFILE)

AC_OUTPUT([
  Makefile 
  lib/Makefile
  lib/arm/armopts.s
  include/Makefile include/theora/Makefile
  examples/Makefile
  doc/Makefile doc/Doxyfile doc/spec/Makefile
  tests/Makefile
  m4/Makefile
  libtheora.spec
  theora.pc
  theora-uninstalled.pc
  theoradec.pc
  theoradec-uninstalled.pc
  theoraenc.pc
  theoraenc-uninstalled.pc
])

AS_AC_EXPAND(LIBDIR, ${libdir})
AS_AC_EXPAND(INCLUDEDIR, ${includedir})
AS_AC_EXPAND(BINDIR, ${bindir})
AS_AC_EXPAND(DOCDIR, ${docdir})

if test $HAVE_DOXYGEN = "false"; then
  doc_build="no"
else
  doc_build="yes"
fi
if test $BUILD_SPEC = "false"; then
  spec_build="no"
else
  spec_build="yes"
fi

AC_MSG_RESULT([
------------------------------------------------------------------------
  $PACKAGE $VERSION:  Automatic configuration OK.

  General configuration:

    Encoding support: ........... ${ac_enable_encode}
    Assembly optimization: ...... ${cpu_optimization}
    Debugging telemetry: ........ ${ac_enable_telemetry}
    Build example code: ......... ${ac_enable_examples}
    API Documentation: .......... ${doc_build}
    Format Documentation: ....... ${spec_build}

  Installation paths:

    libtheora: ................... ${LIBDIR}
    C header files: .............. ${INCLUDEDIR}/theora
    Documentation: ............... ${DOCDIR}

  Building:

    Type 'make' to compile $PACKAGE.

    Type 'make install' to install $PACKAGE.

    ${TESTS_INFO}

  Example programs will be built but not installed.
------------------------------------------------------------------------
])

