dnl
dnl	Configure.ac for ardome-ml
dnl

AC_INIT
AM_CONFIG_HEADER([openlibraries_global_config.hpp])

dnl
dnl	Require autoconf version 2.56
dnl
AC_PREREQ(2.56)

dnl
dnl Versioning
dnl
OL_MAJOR=0
OL_MINOR=0
OL_SUB=1

OL_LT_CURRENT=0
OL_LT_REVISION=4
OL_LT_AGE=0

AC_SUBST(OL_MAJOR)
AC_SUBST(OL_MINOR)
AC_SUBST(OL_SUB)

AC_SUBST(OL_LT_CURRENT)
AC_SUBST(OL_LT_REVISION)
AC_SUBST(OL_LT_AGE)

dnl
AM_INIT_AUTOMAKE("ardome_ml", 0.0.1)

dnl Check for programs
AC_PROG_CXX
AC_PROG_OBJC
AC_HEADER_STDC
AC_PROG_MAKE_SET
AC_PROG_INSTALL
AC_PROG_LN_S

dnl Check if specific olibs should be disabled

AC_ARG_ENABLE(pool, AC_HELP_STRING([--enable-pool], [enable memory pooling]), [enablepool=$enableval], [enablepool=no])
if test "$enablepool" = "yes" ; then
	AC_DEFINE(HAVE_POOLING, 1,[Define this if you want memory pooling])
fi

dnl libtool
AC_LIBTOOL_DLOPEN
AC_DISABLE_STATIC
AC_PROG_LIBTOOL
AC_SUBST(LIBTOOL_DEPS)

dnl dl support
AC_CHECK_LIB(dl, dlopen,
			 have_dl=yes DL_LIBS="-ldl",
			 AC_MSG_ERROR([*** dynamic loading support is not available. Compilation will fail. ***]))
AC_SUBST(DL_LIBS)

dnl determine libdir name. tests for 64bit. (from xine-lib).
AC_MSG_CHECKING([if compiler is 64bit])
case $host in
	*-*-linux*)
		# Test if the compiler is 64bit
		echo 'int i;' > conftest.$ac_ext
		ol_cv_cc_64bit_output=no
		if AC_TRY_EVAL(ac_compile); then
			case `/usr/bin/file conftest.$ac_objext` in
				*"ELF 64"*)
				ol_cv_cc_64bit_output=yes
				;;
			esac
 		fi
 	rm -rf conftest*
 	;;
esac

case $host_cpu:$ol_cv_cc_64bit_output in
	powerpc64:yes | x86_64:yes)
		OL_LIBNAME="lib64"
		if test "$libdir" = '${exec_prefix}/lib'; then
			libdir='${exec_prefix}/lib64'
		fi
 		;;
	*:*)
 		OL_LIBNAME="lib"
 		;;
esac
AC_MSG_RESULT([$OL_LIBNAME])
AC_SUBST(OL_LIBNAME)

dnl C math library.
AC_CHECK_LIB(m, sin)

dnl zlib support
AC_CHECK_ZLIB( )

dnl bzip2 support
AC_CHECK_BZIP2( )

dnl check for X11
AC_PATH_XTRA
if test x"$no_x" != "xyes"; then
    AC_DEFINE(HAVE_X11,1,[Define this if you have X11R6 installed])
fi
AM_CONDITIONAL(HAVE_X11, [test x"$no_x" != "xyes"])

AC_ARG_WITH( install-name, 
			 AC_HELP_STRING([--with-install-name=@loader_path], [specify os/x install_name prefix]),
			 [withinstallname=$withval], [withinstallname=@loader_path])

dnl OpenGL, GLU, GLUT support
OIL_EXTRA_LIBS=""
OML_EXTRA_LIBS=""
OPL_EXTRA_LIBS=""
AC_MSG_CHECKING([the OpenGL environment])
case $host in
	*-*-linux*)
		AM_CONDITIONAL(HAVE_LINUX, [true])
		AM_CONDITIONAL(HAVE_GL_GL_H, [false])
		AM_CONDITIONAL(HAVE_GL_GLUT_H, [false])
	;;
	*-apple-darwin*)
		AM_CONDITIONAL(HAVE_LINUX, [false])
		OCL_EXTRA_LIBS="-Wl,-install_name -Wl,$withinstallname/libopencorelib_cl.dylib -Xlinker -framework -Xlinker ExceptionHandling -Xlinker -framework -Xlinker Foundation -Xlinker -framework -Xlinker CoreServices"
		OIL_EXTRA_LIBS="-Wl,-install_name -Wl,$withinstallname/libopenimagelib_il.dylib"
		OML_EXTRA_LIBS="-Wl,-install_name -Wl,$withinstallname/libopenmedialib_ml.dylib"
		OPL_EXTRA_LIBS="-Wl,-install_name -Wl,$withinstallname/libopenpluginlib_pl.dylib -Xlinker -framework -Xlinker Carbon"
		AM_CONDITIONAL(HAVE_GL_GL_H, [false])
		AM_CONDITIONAL(HAVE_GL_GLUT_H, [false])
	;;
	*)
		AM_CONDITIONAL(HAVE_LINUX, [true])
		AC_MSG_RESULT([OpenGL configuration not tested.])
		AM_CONDITIONAL(HAVE_GL_GL_H, [false])
		AM_CONDITIONAL(HAVE_GL_GLUT_H, [false])
	;;
esac
AC_SUBST(OCL_EXTRA_LIBS)
AC_SUBST(OIL_EXTRA_LIBS)
AC_SUBST(OML_EXTRA_LIBS)
AC_SUBST(OPL_EXTRA_LIBS)

dnl Boost support
AC_LANG([C++])
AC_CHECK_BOOST()

dnl libxml2 support
AC_PATH_PROG(LIBXML2_CONFIG, xml2-config, no)
if test "x$LIBXML2_CONFIG" = "xno" ; then
	AC_MSG_RESULT([*** libxml2 support is not available ***])
else
	XML2_CXXFLAGS=`$LIBXML2_CONFIG --cflags`
	XML2_LIBS=`$LIBXML2_CONFIG --libs`
	have_libxml2="yes"
	AC_DEFINE(HAVE_LIBXML2,1,[Define this for libxml2 support])
fi

AM_CONDITIONAL(HAVE_LIBXML2, test x"$have_libxml2" = "xyes")
AC_SUBST(XML2_CXXFLAGS)
AC_SUBST(XML2_LIBS)

dnl OpenImageLib support
AC_MSG_NOTICE([*** Configuring OpenImageLib ***])
AC_CHECK_OPENIMAGELIB( )

AC_CHECK_QUICKTIME( )

SWAB_AUDIO=""
AC_ARG_ENABLE(swab-audio, AC_HELP_STRING([--enable-swab-audio], [enable byte swapping of audio samples]),
		[enableswabaudio=$enableval], [enableswabaudio=no])
if test x$enableswabaudio = "xyes" ; then
	SWAB_AUDIO="-DSWAB_AUDIO"
fi
AC_SUBST(SWAB_AUDIO)

dnl include paths
INCLUDES='-I$(top_srcdir)/src'
AC_SUBST(INCLUDES)

dnl OpenLibraries common flags
dnl NOTE: we could use $(var) instead of @var@ if we want late-expansion in the generated makefiles.
OLIB_CXXFLAGS='-Wall -Wconversion'
OLIB_CORE_CXXFLAGS=''

AC_ARG_ENABLE(mmx, AC_HELP_STRING([--enable-mmx], [enable mmx]), [enablemmx=$enableval], [enablemmx=no])
if test "$enablemmx" = "yes" ; then
	OLIB_CXXFLAGS+=" -DHAVE_MMX"
fi

case $host in
	*-*-linux*)
		OLIB_CORE_CXXFLAGS="-DOLIB_USE_UTF8 -DOLIB_ON_LINUX"
		OLIB_CXXFLAGS+=" -O3 -fomit-frame-pointer"
	;;
	*-apple-darwin*)
		OLIB_CORE_CXXFLAGS="-DOLIB_USE_UTF8"
		OLIB_CXXFLAGS+=" -O3 -fasm-blocks -fomit-frame-pointer"
	;;
esac


AC_SUBST(OLIB_CXXFLAGS)
AC_SUBST(OLIB_CORE_CXXFLAGS)

OLIB_LDFLAGS=''
OLIB_CXXFLAGS=''

case $host in
	*-*-linux*)
		OLIB_LDFLAGS='-Wl,-export-dynamic'
	;;
	*-apple-darwin*)
		OLIB_LDFLAGS=''
	;;
esac

AC_SUBST(OLIB_LDFLAGS)

AC_ARG_ENABLE(staticavformat, AC_HELP_STRING([--enable-staticavformat], [enable static avformat]), [enablestaticavformat=$enableval], [enablestaticavformat=no])

dnl Check for ffmpeg's libavformat
AC_CHECK_PROG(HAVE_PKG_CONFIG, pkg-config, yes)
if test "x$HAVE_PKG_CONFIG" = "xyes" ; then
  PKG_CHECK_MODULES(LIBAVFORMAT, libavformat, libavformat="yes", libavformat="no")
  if test x"$libavformat" = x"no"; then
	AC_MSG_RESULT([*** ffmpeg development environment is not available ***])
  else
	AC_DEFINE(HAVE_LIBAVFORMAT,1,[Define this if you have libavformat])
	AC_SUBST(LIBAVFORMAT_CFLAGS)
    if test "$enablestaticavformat" = "yes"; then
	  AC_MSG_RESULT([*** ffmpeg using static libs ***])
	  LIBAVFORMAT_LIBS=""
    fi
     AC_SUBST(LIBAVFORMAT_LIBS)
  fi
fi
AM_CONDITIONAL(HAVE_LIBAVFORMAT, [test x"$libavformat" = "xyes"])

dnl caca support
AC_ARG_ENABLE(caca, AC_HELP_STRING([--disable-caca], [disable caca dependent parts]), [enablecaca=$enableval], [enablecaca=yes])
if test "$enablecaca" = "yes" ; then
	AC_PATH_PROG(CACA_CONFIG, caca-config, no)
	if test "x$CACA_CONFIG" = "xno" ; then
		AC_MSG_RESULT([*** caca support is not available ***])
	else
		AC_DEFINE(HAVE_LIBCACA,1,[Define this for caca support])
		CACA_CXXFLAGS=`$CACA_CONFIG --cflags`
		CACA_LIBS=`$CACA_CONFIG --plugin-libs`
		AC_SUBST(CACA_CXXFLAGS)
		AC_SUBST(CACA_LIBS)
		have_libcaca="yes"
	fi
fi
AM_CONDITIONAL(HAVE_LIBCACA, [test x"$have_libcaca" = "xyes"])

dnl mlt support
AC_ARG_ENABLE(mlt, AC_HELP_STRING([--disable-mlt], [disable mlt dependent parts]), [enablemlt=$enableval], [enablemlt=yes])
if test "$enablemlt" = "yes" ; then
	AC_PATH_PROG(MLT_CONFIG, mlt-config, no)
	if test "x$MLT_CONFIG" = "xno" ; then
		AC_MSG_RESULT([*** mlt support is not available ***])
	else
		AC_DEFINE(HAVE_LIBMLT,1,[Define this for mlt support])
		MLT_CXXFLAGS=`$MLT_CONFIG --cflags`
		MLT_LIBS="-lmlt++ `$MLT_CONFIG --libs`"
		AC_SUBST(MLT_CXXFLAGS)
		AC_SUBST(MLT_LIBS)
		have_libmlt="yes"
	fi
fi
AM_CONDITIONAL(HAVE_LIBMLT, [test x"$have_libmlt" = "xyes"])

dnl sdl support
AC_PATH_PROG(SDL_CONFIG, sdl-config, no)
if test "x$SDL_CONFIG" = "xno" ; then
	AC_MSG_RESULT([*** sdl support is not available ***])
else
	AC_DEFINE(HAVE_LIBSDL,1,[Define this for sdl support])
	SDL_CXXFLAGS=`$SDL_CONFIG --cflags`
	SDL_LIBS=`$SDL_CONFIG --libs`
	AC_SUBST(SDL_CXXFLAGS)
	AC_SUBST(SDL_LIBS)
	have_libsdl="yes"
fi
AM_CONDITIONAL(HAVE_LIBSDL, [test x"$have_libsdl" = "xyes"])

dnl qimage support
AC_CHECK_QT( )

dnl Check for ILM OpenEXR
if test "x$HAVE_PKG_CONFIG" = "xyes" ; then
  PKG_CHECK_MODULES(OPENEXR, OpenEXR, OpenEXR="yes", OpenEXR="no")
  if test x"$OpenEXR" = x"no"; then
	AC_MSG_RESULT([*** ILM OpenEXR development environment is not available ***])
  else
	AC_DEFINE(HAVE_OPENEXR,1,[Define this if you have ILM OpenEXR])
	AC_SUBST(OPENEXR_CFLAGS)
	AC_SUBST(OPENEXR_LIBS)
  fi
fi
AM_CONDITIONAL(HAVE_OPENEXR, [test x"$OpenEXR" = "xyes"])

dnl Check for openal
AC_CHECK_OPENAL_RUNTIME( )

dnl Check for gpl components
AC_CHECK_GPL( )

dnl Check for Python
AC_CHECK_PYTHON( )

dnl Universal Binary Support (OS X only)
AC_CHECK_UNIVERSAL_BINARY_SUPPORT( )

dnl Fast Math support
AC_CHECK_FAST_MATH( )

SHARED_EXT="so"
case $host in
	*-apple-darwin*)
		SHARED_EXT="dylib"
	;;
esac
AC_SUBST(SHARED_EXT)

dnl include, lib and plugin path
OPENLIBRARIES_INCLUDEPATH="$includedir/ardome-ml-$OL_MAJOR.$OL_MINOR.$OL_SUB"
OPENLIBRARIES_LIBPATH="$libdir/ardome-ml-$OL_MAJOR.$OL_MINOR.$OL_SUB"
OPENLIBRARIES_DATAPATH="$datadir/ardome-ml-$OL_MAJOR.$OL_MINOR.$OL_SUB"
OPENCORELIB_LIBPATH="$libdir"
OPENIMAGELIB_LIBPATH="$libdir"
OPENIMAGELIB_PLUGINPATH="${OPENLIBRARIES_LIBPATH}/openimagelib/plugins"
OPENMEDIALIB_LIBPATH="$libdir"
OPENMEDIALIB_PLUGINPATH="${OPENLIBRARIES_LIBPATH}/openmedialib/plugins"
OPENPLUGINLIB_LIBPATH="$libdir"
AC_SUBST(OPENCORELIB_LIBPATH)
AC_SUBST(OPENIMAGELIB_LIBPATH)
AC_SUBST(OPENIMAGELIB_PLUGINPATH)
AC_SUBST(OPENMEDIALIB_LIBPATH)
AC_SUBST(OPENMEDIALIB_PLUGINPATH)
AC_SUBST(OPENLIBRARIES_INCLUDEPATH)
AC_SUBST(OPENLIBRARIES_LIBPATH)
AC_SUBST(OPENLIBRARIES_DATAPATH)
AC_SUBST(OPENPLUGINLIB_LIBPATH)

#
# RPATHS
#

OPENCORELIB_LDFLAGS="-L$libdir -lopencorelib_cl"
AC_SUBST(OPENCORELIB_LDFLAGS)

OPENMEDIALIB_LDFLAGS="-L$libdir -lopenmedialib_ml"
AC_SUBST(OPENMEDIALIB_LDFLAGS)

OPENIMAGELIB_LDFLAGS="-L$libdir -lopenimagelib_il"
AC_SUBST(OPENIMAGELIB_LDFLAGS)

OPENPLUGINLIB_LDFLAGS="-L$libdir -lopenpluginlib_pl"
AC_SUBST(OPENPLUGINLIB_LDFLAGS)

dnl
dnl Output configuration files
dnl
AC_CONFIG_FILES([
Makefile
src/Makefile
src/opencorelib/Makefile
src/opencorelib/cl/Makefile
src/openpluginlib/Makefile
src/openpluginlib/pl/Makefile
src/openpluginlib/py/Makefile
src/openimagelib/Makefile
src/openimagelib/il/Makefile
src/openimagelib/plugins/Makefile
src/openimagelib/plugins/3D_lightmap/Makefile
src/openimagelib/plugins/dds/Makefile
src/openimagelib/plugins/dpx/Makefile
src/openimagelib/plugins/exr/Makefile
src/openimagelib/plugins/hdr/Makefile
src/openimagelib/plugins/jpg/Makefile
src/openimagelib/plugins/png/Makefile
src/openimagelib/plugins/qim/Makefile
src/openimagelib/plugins/quicktime/Makefile
src/openimagelib/plugins/sgi/Makefile
src/openimagelib/plugins/tga/Makefile
src/openimagelib/plugins/tiff/Makefile
src/openimagelib/py/Makefile
src/openmedialib/Makefile
src/openmedialib/ml/Makefile
src/openmedialib/plugins/Makefile
src/openmedialib/plugins/template/Makefile
src/openmedialib/plugins/avformat/Makefile
src/openmedialib/plugins/oil/Makefile
src/openmedialib/plugins/caca/Makefile
src/openmedialib/plugins/openal/Makefile
src/openmedialib/plugins/mlt/Makefile
src/openmedialib/plugins/sdl/Makefile
src/openmedialib/plugins/gensys/Makefile
src/openmedialib/plugins/quicktime/Makefile
src/openmedialib/py/Makefile
test/Makefile
test/openpluginlib/Makefile
test/openpluginlib/discovery/Makefile
test/openpluginlib/enumerate/Makefile
test/openpluginlib/gettimeofday/Makefile
test/openpluginlib/gpu_timer_query/Makefile
test/openpluginlib/initialization/Makefile
test/openpluginlib/rdtsc/Makefile
test/openpluginlib/pcos/Makefile
test/openpluginlib/pcos/key/Makefile
test/openpluginlib/pcos/property/Makefile
test/openpluginlib/pcos/subject-observer/Makefile
test/openpluginlib/pcos/property_container/Makefile
test/openimagelib/Makefile
test/openimagelib/GL/Makefile
test/openimagelib/GL/_2D/Makefile
test/openimagelib/GL/_2D_compressed/Makefile
test/openimagelib/GL/_2D_compressed_cubemap/Makefile
test/openimagelib/GL/_2D_crop/Makefile
test/openimagelib/GL/_2D_crop2/Makefile
test/openimagelib/GL/_2D_exr/Makefile
test/openimagelib/GL/_2D_photoshop/Makefile
test/openimagelib/GL/_2D_scale/Makefile
test/openimagelib/GL/_2D_sgi/Makefile
test/openimagelib/Serialization/Makefile
test/openimagelib/Serialization/png_storer/Makefile
test/openmedialib/Makefile
test/openmedialib/player/Makefile
test/openmedialib/store/Makefile
ardome_ml.pc
])
AC_OUTPUT
