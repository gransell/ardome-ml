
#
# Configure OpenImageLib
#

AC_DEFUN([AC_CHECK_OPENIMAGELIB],[
	AC_ARG_WITH(openimagelib, AC_HELP_STRING([--with-openimagelib], 
				[configure with OpenImageLib support (default=yes)]),with_openimagelib="yes",[])
	
	AS_IF([test "x$with_openimagelib" == "xyes"],
		AC_MSG_WARN([*** OpenImageLib is not configured. Some functionality may not be available. ***]),
		ac_use_openimagelib=yes AC_DEFINE(HAVE_OPENIMAGELIB, 1, [Define this for OpenImageLib support])
	)
	
	if test x$ac_use_openimagelib = "xyes"; then
		dnl libpng support
		AC_PATH_PROG(LIBPNG_CONFIG, libpng-config, no)
		if test "$LIBPNG_CONFIG" = "no" ; then
			AC_CHECK_LIB(png, png_create_read_struct,
				 		 have_libpng=yes PNG_LIBS="-lpng",
				 		 AC_MSG_RESULT([*** PNG support is not available ***]))
		else
			PNG_CFLAGS=`$LIBPNG_CONFIG --cflags`
			PNG_LIBS=`$LIBPNG_CONFIG --libs`
			have_libpng="yes"
			AC_DEFINE(HAVE_LIBPNG,1,[Define this for PNG support])
		fi

		AM_CONDITIONAL(HAVE_LIBPNG, test x"$have_libpng" = "xyes")
		AC_SUBST(PNG_CFLAGS)
		AC_SUBST(PNG_LIBS)

		dnl libjpeg support
		AC_CHECK_LIB(jpeg, jpeg_read_header,
			 		 have_libjpeg=yes JPEG_LIBS="-ljpeg",
			 		 AC_MSG_RESULT([*** JPEG support is not available ***]))
		AM_CONDITIONAL(HAVE_LIBJPEG, test x"$have_libjpeg" = "xyes")
		AC_SUBST(JPEG_LIBS)

		dnl libtiff support
		AC_CHECK_LIB(tiff, TIFFOpen,
			 		 have_libtiff=yes TIFF_LIBS="-ltiff",
			 		 AC_MSG_RESULT([*** TIFF support is not available ***]))
		AM_CONDITIONAL(HAVE_LIBTIFF, test x"$have_libtiff" = "xyes")
		AC_SUBST(TIFF_LIBS)
	fi

	AM_CONDITIONAL(HAVE_OPENIMAGELIB, test x"$ac_use_openimagelib" = "xyes")

	if test x$ac_use_openimagelib = "xyes"; then
		ifelse([$1], , :, [$1])
	else
		ifelse([$2], , :, [$2])
	fi
])
