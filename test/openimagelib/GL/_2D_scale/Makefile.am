
#
#
#

check_PROGRAMS = _2D_scale

_2D_scale_SOURCES = 	\
	_2D_scale.cpp

_2D_scale_CXXFLAGS = 				\
	$(OLIB_CXXFLAGS)		\
	$(BOOST_INCLUDE_PATH)	\
	$(GLEW_INCLUDE_PATH)

_2D_scale_LDFLAGS =			\
	$(OLIB_LDFLAGS)		\
	$(BOOST_LIB_PATH)	\
	$(GLEW_LIB_PATH)	\
	$(BOOST_LIB_RPATH)	\
	$(GLEW_LIB_RPATH)

_2D_scale_LDADD = 													\
	$(OPENGL_LIBS) 												\
	$(OPENGL_GLU_LIBS) 											\
	$(GLEW_LIBS)												\
	$(GLUT_LIBS)												\
	$(BOOST_FILESYSTEM_LIBS)									\
	$(top_builddir)/src/openpluginlib/pl/libopenpluginlib_pl.la	\
	$(top_builddir)/src/openimagelib/il/libopenimagelib_il.la	\
	$(DL_LIBS)
