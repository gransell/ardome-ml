
#
# Check for bzip2
#

AC_DEFUN([AC_CHECK_BZIP2],[
	AC_ARG_ENABLE(bzip2, AC_HELP_STRING([--disable-bzip2], [disable bzip2 dependent parts]),
		[enablebzip2=$enableval],
		[enablebzip2=yes])

	if test x$enablebzip2 = "xyes" ; then
		AC_CHECK_LIB(bz2, BZ2_bzDecompress,
			[AC_CHECK_HEADER(bzlib.h, [ac_have_bzip2="yes" BZIP2_LIBS="-lz" AC_DEFINE(HAVE_BZIP2,1,[Define this if you have bzip2 support])],
			[AC_MSG_ERROR([*** bzip2 runtime is available but no development headers. Please check your bzip2 installation. ***])])],
			[AC_MSG_WARN([*** bzip2 runtime is not available. All bzip2 dependent parts will be disabled. ***])])

		if test x$ac_have_bzip2 = "xyes"; then
			ac_use_bzip2=yes
		fi
	fi

	AC_SUBST(BZIP2_LIBS)
	
	AM_CONDITIONAL(HAVE_BZIP2, [test x$ac_use_bzip2 = "xyes"])

	if test x$ac_use_bzip2 = "xyes"; then
		ifelse([$1], , :, [$1])
	else
		ifelse([$2], , :, [$2])
	fi
])
