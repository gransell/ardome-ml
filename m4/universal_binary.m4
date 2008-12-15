
#
# OS X universal binary support
#

AC_DEFUN([AC_CHECK_UNIVERSAL_BINARY_SUPPORT],[
	AC_ARG_ENABLE(universalbinaries, AC_HELP_STRING([--enable-universalbinaries],[build universal binaries on OS X [[default=no]]]),
		[enableuniversalbinaries=${enableval}],
		[enableuniversalbinaries=no])

	case $host in
		*-apple-darwin*)
			AC_MSG_CHECKING(if building OS X universal binaries)
			if test x$enableuniversalbinaries != "xno" ; then
				if test x$enable_dependency_tracking != "xno" ; then
					AC_MSG_ERROR([--enable-universalbinaries requires --disable-dependency-tracking])
				fi
				AC_MSG_RESULT(yes)
				CXXFLAGS="$CXXFLAGS -isysroot /Developer/SDKs/MacOSX10.4u.sdk -arch ppc -arch i386"
			else
				AC_MSG_RESULT(no)
			fi
		;;	
	esac	
])

