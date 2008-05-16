
#
# Check for zlib
#

AC_DEFUN([AC_CHECK_ZLIB],[
	AC_ARG_ENABLE(zlib, AC_HELP_STRING([--disable-zlib], [disable zlib dependent parts]),
		[enablezlib=$enableval],
		[enablezlib=yes])

	if test x$enablezlib = "xyes" ; then
		AC_CHECK_LIB(z, uncompress,
			[AC_CHECK_HEADER(zlib.h, [ac_have_zlib="yes" ZLIB_LIBS="-lz" AC_DEFINE(HAVE_ZLIB,1,[Define this if you have zlib support])],
			[AC_MSG_ERROR([*** zlib runtime is available but no development headers. Please check your zlib installation. ***])])],
			[AC_MSG_WARN([*** zlib runtime is not available. All zlib dependent parts will be disabled. ***])])

		if test x$ac_have_zlib = "xyes"; then
			ac_use_zlib=yes
		fi
	fi

	AC_SUBST(ZLIB_LIBS)
	AM_CONDITIONAL([HAVE_ZLIB], [test x"$ac_use_zlib" = xyes])

	if test x$ac_use_zlib = "xyes"; then
		ifelse([$1], , :, [$1])
	else
		ifelse([$2], , :, [$2])
	fi
])
