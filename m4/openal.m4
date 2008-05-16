
#
# Check for Creative OpenAL runtime support.
#

AC_DEFUN([AC_CHECK_OPENAL_RUNTIME],[
	AC_ARG_WITH(openalprefix, AC_HELP_STRING([--with-openalprefix], [OpenAL installation prefix directory]),,
				with_openalprefix="")

	AC_ARG_ENABLE(openal, AC_HELP_STRING([--disable-openal], [disable Creative OpenAL dependent parts]),
		[enableopenal=$enableval],
		[enableopenal=yes])

	AC_ARG_ENABLE(openalframework, AC_HELP_STRING([--enable-openalframework], [enable use of Creative OpenAL framework (OSX only)]),
		[enableopenalframework=$enableval],
		[enableopenalframework=no])

	OPENAL_PREFIX="$with_openalprefix"
	OPENAL_INCLUDE_PATH=""
	OPENAL_LIB_PATH=""
	OPENAL_LIBS=""

	if test x$enableopenal = "xyes" ; then
		if test x$enableopenalframework = "xno" ; then
			AS_IF([test "x$with_openalprefix" != "x" ],
				[ AC_MSG_WARN([*** OpenAL prefix is user defined. Current value is $with_openalprefix. ***])
			  		OPENAL_INCLUDE="$with_openalprefix/include/"
			  		OPENAL_INCLUDE_PATH="-I$OPENAL_INCLUDE"
			  		OPENAL_LIB_PATH="-L$with_openalprefix/$OL_LIBNAME"],
			  		AC_MSG_RESULT([*** OpenAL prefix is not defined. Will use system defaults. ***]))

			save_CXXFLAGS="$CXXFLAGS"
			save_LDFLAGS="$LDFLAGS"
			save_LIBS="$LIBS"

			CXXFLAGS="$save_CXXFLAGS $OPENAL_INCLUDE_PATH"
			LDFLAGS="$save_LDFLAGS $OPENAL_LIB_PATH"
			LIBS="$save_LIBS -lpthread"
		
			AC_CHECK_LIB(openal, alEnable,
				[AC_CHECK_HEADER(AL/al.h,
					[ac_have_openal="yes" OPENAL_LIBS="-lopenal" AC_DEFINE(HAVE_OPENAL,1,[Define this if you have Creative OpenAL runtime support])],
					[AC_MSG_ERROR([*** Creative OpenAL runtime is available but no development headers. Please check your OpenAL installation ***])])],
				[AC_MSG_WARN([*** Creative OpenAL runtime is not available. All OpenAL dependent parts will be disabled ***])])

			if test x$ac_have_openal = "xyes"; then
				ac_use_openal=yes
				OPENAL_LIBS="-lopenal -lpthread"
			fi
			
			CXXFLAGS="$save_CXXFLAGS"
			LDFLAGS="$save_LDFLAGS"
			LIBS="$save_LIBS"
		
		else
			AC_MSG_NOTICE([*** OpenAL framework defaults on OSX. ***])
			OPENAL_INCLUDE_PATH="-F/System/Library/Frameworks/OpenAL.Framework"
			OPENAL_LIBS="-Xlinker -framework -Xlinker OpenAL"
			ac_use_openal=yes
			ac_use_openalframework=yes
			AC_DEFINE(HAVE_OPENAL,1,[Define this if you have  Creative OpenAL runtime support])
			AC_DEFINE(HAVE_OPENALFRAMEWORK,1,[Define this to use Creative OpenAL framework (OSX only)])
		fi
	fi

	AC_SUBST(OPENAL_INCLUDE_PATH)
	AC_SUBST(OPENAL_LIB_PATH)
	AC_SUBST(OPENAL_LIBS)
	AM_CONDITIONAL(HAVE_OPENAL, [test x$ac_use_openal = "xyes"])

	if test x$ac_use_openal = "xyes"; then
		ifelse([$1], , :, [$1])
	else
		ifelse([$2], , :, [$2])
	fi
])
