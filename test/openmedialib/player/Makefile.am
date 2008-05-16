
if HAVE_GL_GLUT_H

check_PROGRAMS = player

player_SOURCES = 	\
	player.cpp

player_CXXFLAGS = 										  \
	$(OLIB_CXXFLAGS)									  \
	$(BOOST_INCLUDE_PATH)								  \
	$(GLEW_INCLUDE_PATH) 								  \
	-DOPENIMAGELIB_PLUGINS=\"$(OPENIMAGELIB_PLUGINPATH)\" \
	-DOPENMEDIALIB_PLUGINS=\"$(OPENMEDIALIB_PLUGINPATH)\"

player_LDFLAGS =		\
	$(OLIB_LDFLAGS)							\
	$(BOOST_LIB_PATH)   \
	$(GLEW_LIB_PATH)	\
	$(BOOST_LIB_RPATH)	\
	$(GLEW_LIB_RPATH)

player_LDADD = 													\
	$(OPENGL_LIBS) 												\
	$(OPENGL_GLU_LIBS) 											\
	$(GLEW_LIBS)												\
	$(GLUT_LIBS)												\
	$(BOOST_FILESYSTEM_LIBS)									\
	$(top_builddir)/src/openpluginlib/pl/libopenpluginlib_pl.la	\
	$(top_builddir)/src/openimagelib/il/libopenimagelib_il.la	\
	$(top_builddir)/src/openmedialib/ml/libopenmedialib_ml.la	\
	$(DL_LIBS)

endif
