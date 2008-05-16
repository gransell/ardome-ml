
#
# Check for Python
#

AC_DEFUN([AC_CHECK_PYTHON],[
	AC_ARG_ENABLE(python, AC_HELP_STRING([--disable-python], [disable python dependent parts]),
		[enablepython=$enableval],
		[enablepython=yes])
	AC_ARG_WITH(pythonversion, AC_HELP_STRING([--with-pythonversion],[Python current version]),,with_pythonversion="2.4")
	AC_ARG_WITH(pythonprefix, AC_HELP_STRING([--with-pythonprefix], [Python prefix path]),,with_pythonprefix="/usr")

	PYTHON_INCLUDE="$with_pythonprefix/include/python$with_pythonversion"
	PYTHON_INCLUDE_PATH="-I$PYTHON_INCLUDE"
	PYTHON_VERSION="$with_pythonversion"

	if test x$enablepython = "xyes" ; then
		AC_CHECK_HEADER(${PYTHON_INCLUDE}/Python.h, [ac_have_python="yes" AC_DEFINE(HAVE_PYTHON,1,[Define this if you have Python support])],
		[AC_MSG_WARN([*** no Python development headers. All Python dependent parts will be disabled. ***])])

		if test x$ac_have_python = "xyes"; then
			ac_use_python=yes
		fi
	fi

	AC_SUBST(PYTHON_VERSION)
	AC_SUBST(PYTHON_INCLUDE_PATH)
	AM_CONDITIONAL([HAVE_PYTHON], [test x"$ac_use_python" = xyes])

	if test x"$ac_use_python" = "xyes"; then
		ifelse([$1], , :, [$1])
	else
		ifelse([$2], , :, [$2])
	fi
])
