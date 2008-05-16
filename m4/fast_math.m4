
#
# Enable/Disable fast math support.
#

AC_DEFUN([AC_CHECK_FAST_MATH],[
	AC_ARG_ENABLE(fastmath, AC_HELP_STRING([--enable-fastmath], [disable fast math support]),
		[enablefastmath=$enableval],
		[enablefastmath=yes])

	AC_MSG_CHECKING(for fast math support)
	if test x$enablefastmath = "xyes" ; then
		AC_DEFINE(HAVE_FAST_MATH,1,[Define this to enable fast math support])
		AC_MSG_RESULT(yes)
	else
		AC_MSG_RESULT(no)
	fi

	if test x$enablefastmath = "xyes"; then
		ifelse([$1], , :, [$1])
	else
		ifelse([$2], , :, [$2])
	fi
])
