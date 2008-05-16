
#
#
#

check_PROGRAMS = _2D_compressed

_2D_compressed_SOURCES = 	\
	_2D_compressed.cpp

_2D_compressed_CXXFLAGS = 	\
	$(OLIB_CXXFLAGS) 		\
	$(BOOST_INCLUDE_PATH) 	\
	$(GLEW_INCLUDE_PATH) 	\
	-DOPENIMAGELIB_PLUGINS=\"$(OPENIMAGELIB_PLUGINPATH)\"

_2D_compressed_LDFLAGS = 	\
	$(OLIB_LDFLAGS)			\
	$(BOOST_LIB_PATH)		\
	$(GLEW_LIB_PATH)		\
	$(BOOST_LIB_RPATH)		\
	$(GLEW_LIB_RPATH)

_2D_compressed_LDADD = 											\
	$(OPENGL_LIBS) 												\
	$(OPENGL_GLU_LIBS) 											\
	$(GLEW_LIBS)												\
	$(GLUT_LIBS)												\
	$(BOOST_FILESYSTEM_LIBS)									\
	$(top_builddir)/src/openpluginlib/pl/libopenpluginlib_pl.la	\
	$(top_builddir)/src/openimagelib/il/libopenimagelib_il.la	\
	$(DL_LIBS)
