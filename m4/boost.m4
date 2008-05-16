
#
# Check for Boost
#

AC_DEFUN([AC_CHECK_BOOST],[
	AC_ARG_WITH(boostprefix, 	AC_HELP_STRING([--with-boostprefix], [Boost installation prefix directory]),,with_boostprefix="")
	AC_ARG_WITH(boostversion, 	AC_HELP_STRING([--with-boostversion],[Boost current version]),,with_boostversion="")
	AC_ARG_WITH(boosttoolset, 	AC_HELP_STRING([--with-boosttoolset],[Boost toolset (e.g. gcc)]),,with_boosttoolset="")
	AC_ARG_WITH(boostruntime, 	AC_HELP_STRING([--with-boostruntime],[Boost runtime (e.g. mt)]),,with_boostruntime="")
	AC_ARG_WITH(boostthreadruntime,	AC_HELP_STRING([--with-boostthreadruntime],[Boost thread runtime (e.g. mt)]),,with_boostthreadruntime="")

	BOOST_PREFIX="$with_boostprefix"

	BOOST_INCLUDE_PATH=""
	BOOST_LIB_PATH=""

	AS_IF([test "x$with_boostversion" == "x"],
		  AC_MSG_WARN([*** Boost version is not defined. Will use system defaults. ***])
		  with_boostversion="" BOOST_INCLUDE_VERSION="" BOOST_LIB_VERSION="",
		  BOOST_INCLUDE_VERSION="boost-$with_boostversion" BOOST_LIB_VERSION="-$with_boostversion"
		  AC_MSG_RESULT([*** Boost version is user defined. Current value is $with_boostversion. ***]))

	AS_IF([test "x$with_boosttoolset" == "x"],
		  AC_MSG_WARN([*** Boost toolset is not defined. Will use system defaults. ***])
		  with_boosttoolset="" BOOST_LIB_TOOLSET="",
		  BOOST_LIB_TOOLSET="-$with_boosttoolset"
		  AC_MSG_RESULT([*** Boost toolset is user defined. Current value is $with_boosttoolset. ***]))

	AS_IF([test "x$with_boostruntime" == "x"],
		  AC_MSG_WARN([*** Boost runtime is not defined. Will use system defaults. ***])
		  with_boostruntime="" BOOST_LIB_RUNTIME="",
		  BOOST_LIB_RUNTIME="-$with_boostruntime"
		  AC_MSG_RESULT([*** Boost runtime is user defined. Current value is $with_boostruntime. ***]))

	AS_IF([test "x$with_boostthreadruntime" == "x"],
		  AC_MSG_WARN([*** Boost thread runtime is not defined. Will use system defaults. ***])
		  with_boostthreadruntime="" BOOST_LIB_THREADRUNTIME="",
		  BOOST_LIB_THREADRUNTIME="-$with_boostthreadruntime"
		  AC_MSG_RESULT([*** Boost thread runtime is user defined. Current value is $with_boostthreadruntime. ***]))

	AS_IF([test "x$with_boostprefix" != "x"],
			AC_MSG_RESULT([*** Boost prefix is user defined. Current value is $with_boostprefix. ***])

			BOOST_INCLUDE="$with_boostprefix/include/$BOOST_INCLUDE_VERSION/"
			BOOST_INCLUDE_PATH="-I$BOOST_INCLUDE"
			BOOST_LIB_PATH="-L$with_boostprefix/$OL_LIBNAME"			
	,,)

	BOOST_LIBNAMESUFFIX="$BOOST_LIB_TOOLSET$BOOST_LIB_RUNTIME$BOOST_LIB_VERSION"

	BOOST_FILESYSTEM_LIBS="-lboost_filesystem$BOOST_LIB_TOOLSET$BOOST_LIB_RUNTIME$BOOST_LIB_VERSION"
	BOOST_IOSTREAMS_LIBS="-lboost_iostreams$BOOST_LIB_TOOLSET$BOOST_LIB_RUNTIME$BOOST_LIB_VERSION"
	BOOST_REGEX_LIBS="-lboost_regex$BOOST_LIB_TOOLSET$BOOST_LIB_RUNTIME$BOOST_LIB_VERSION"
	BOOST_THREAD_LIBS="-lboost_thread$BOOST_LIB_TOOLSET$BOOST_LIB_THREADRUNTIME$BOOST_LIB_VERSION"
	BOOST_PYTHON_LIBS="-lboost_python$BOOST_LIB_TOOLSET$BOOST_LIB_RUNTIME$BOOST_LIB_VERSION"

	save_CPPFLAGS="$CXXFLAGS"
	CPPFLAGS="$CPPFLAGS ${BOOST_INCLUDE_PATH}"

	AC_CHECK_HEADER(boost/shared_ptr.hpp,
		[ac_have_boost_shared_ptr="yes"],
			AC_MSG_ERROR([*** Boost shared_ptr support is not available. Please install Boost to proceed. ***]))

	# this is not correct but it will have to do for now.
	AC_CHECK_HEADER(boost/filesystem/fstream.hpp,
			[ac_have_boost_filesystem="yes"
			 AC_DEFINE(HAVE_BOOST_FILESYSTEM,1,[Define this if you have Boost filesystem support])],
			 AC_MSG_ERROR([*** Boost filesystem support is not available. Please install Boost to proceed. ***]))

	CPPFLAGS="$save_CPPFLAGS"

	if test x$ac_have_boost_shared_ptr = "xyes" ; then
		ac_use_boost_shared_ptr=yes
	fi

	if test x$ac_have_boost_filesystem = "xyes" ; then
		ac_use_filesystem=yes
	fi

	AC_SUBST(BOOST_PREFIX)
	AC_SUBST(BOOST_INCLUDE_VERSION)
	AC_SUBST(BOOST_LIB_VERSION)
	AC_SUBST(BOOST_LIB_TOOLSET)
	AC_SUBST(BOOST_LIB_RUNTIME)
	AC_SUBST(BOOST_INCLUDE_PATH)
	AC_SUBST(BOOST_LIB_PATH)

	AC_SUBST(BOOST_FILESYSTEM_LIBS)
	AM_CONDITIONAL(HAVE_BOOST_FILESYSTEM, [test x$ac_use_boost_filesystem = "xyes"])

	AC_SUBST(BOOST_IOSTREAMS_LIBS)
	AC_SUBST(BOOST_REGEX_LIBS)
	AC_SUBST(BOOST_THREAD_LIBS)
	AC_SUBST(BOOST_PYTHON_LIBS)

	if test x$ac_use_shared_ptr = "xyes" && test x$ac_use_filesystem = "xyes"; then
		ifelse([$1], , :, [$1])
	else
		ifelse([$2], , :, [$2])
	fi
])
