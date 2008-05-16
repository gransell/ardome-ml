
#
# Check for Apple's QuickTime runtime support.
#

AC_DEFUN([AC_CHECK_QUICKTIME],[
	AC_ARG_ENABLE(quicktime, AC_HELP_STRING([--disable-quicktime], [disable Apple's QuickTime dependent parts]),
		[enablequicktime=$enableval],
		[enablequicktime=yes])
		
	QUICKTIME_LIBS=""
	
	AC_MSG_CHECKING(the availability of Apple's QuickTime SDK)
	if test x$enablequicktime = "xyes" ; then
		save_LIBS="$LIBS"
		
		case $host in
			*-apple-darwin*)
				LIBS="-Xlinker -framework -Xlinker QuickTime"
				AC_TRY_LINK([#include <QuickTime/QTML.h>],[QTMLYieldCPU( ); return 0;], ac_have_quicktime="yes" AC_DEFINE(HAVE_QUICKTIME,1,[Define this if you have QuickTime support]))
			;;
		esac
	
		if test x$ac_have_quicktime = "xyes"; then
			ac_use_quicktime=yes
			QUICKTIME_LIBS="$LIBS"
		fi
	
		LIBS="$save_LIBS"
	fi

	AC_SUBST(QUICKTIME_LIBS)
	AM_CONDITIONAL(HAVE_QUICKTIME, [test x$ac_use_quicktime = "xyes"])

	if test x$ac_have_quicktime = "xyes"; then
		AC_MSG_RESULT(yes)
	else
		AC_MSG_RESULT(no)
	fi
	
	if test x$ac_use_quicktime = "xyes"; then
		ifelse([$1], , :, [$1])
	else
		ifelse([$2], , :, [$2])
	fi		
])
