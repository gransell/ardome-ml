
#
# Enable GPL components.
#

AC_DEFUN([AC_CHECK_GPL],[
	AC_ARG_ENABLE(gpl, AC_HELP_STRING([--enable-gpl], [enable GPL components (openlibraries will be GPL)]),
		[enablegpl=$enableval],
		[enablegpl=no])

	AM_CONDITIONAL(USE_GPL_COMPONENTS, [test x$enablegpl = "xyes"])
	
	if test x$enablegpl = "xyes"; then
		ifelse([$1], , :, [$1])
	else
		ifelse([$2], , :, [$2])
	fi
])
