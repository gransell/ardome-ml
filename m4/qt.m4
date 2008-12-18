
#
# Check for Qt
#

AC_DEFUN([AC_CHECK_QT],[
	AC_ARG_WITH(qtinclude, AC_HELP_STRING([--with-qtinclude], [Qt include directory]),,with_qtinclude="/usr/include/qt3")
	AC_ARG_WITH(qtlib, AC_HELP_STRING([--with-qtlib], [Qt lib directory]),,with_qtlib="/usr/lib")

	QT_INCLUDE_PATH="-I${with_qtinclude}"
	QT_LIB_PATH="-L${with_qtlib}"
	QT_LIBS=""

	save_CPPFLAGS="$CPPFLAGS"
	CPPFLAGS="$CPPFLAGS ${QT_INCLUDE_PATH}"

	save_CXXFLAGS="$CXXFLAGS"
	CXXFLAGS="$CXXFLAGS ${QT_INCLUDE_PATH}"
		
	save_LDFLAGS="$LDFLAGS"
	LDFLAGS="$LDFLAGS ${QT_LIB_PATH}"
		
	save_LIBS="$LIBS"
	LIBS="$LIBS -lqt-mt"

	AC_MSG_CHECKING(for QApplication in -lqt-mt)

	AC_LANG(C++)
	AC_TRY_LINK([#include <qapplication.h>], [int argc; QApplication a( argc, NULL )],
		[AC_MSG_RESULT(yes)
		 AC_CHECK_HEADER(qapplication.h,
			[ac_have_qt="yes" QT_LIBS="-lqt-mt" AC_DEFINE(HAVE_QT,1,[Define this if you have Qt runtime support])],
			[AC_MSG_ERROR([*** Qt runtime is available but no development headers. Please check your Qt installation ***])])],
		[AC_MSG_RESULT(no) 
		 AC_MSG_WARN([*** Qt runtime is not available. All Qt dependent parts will be disabled ***])])

	if test x$ac_have_qt = "xyes"; then
		ac_use_qt=yes
	fi

	CPPFLAGS="$save_CPPFLAGS"
	CXXFLAGS="$save_CXXFLAGS"
	LDFLAGS="$save_LDFLAGS"
	LIBS="$save_LIBS"

	AC_SUBST(QT_INCLUDE_PATH)
	AC_SUBST(QT_LIB_PATH)
	AC_SUBST(QT_LIBS)
	AM_CONDITIONAL(HAVE_QT, [test x$ac_use_qt = "xyes"])

	if test x$ac_use_qt = "xyes"; then
		ifelse([$1], , :, [$1])
	else
		ifelse([$2], , :, [$2])
	fi
])
