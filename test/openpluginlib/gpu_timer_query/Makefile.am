
#
#
#

if HAVE_GL_GL_H

check_PROGRAMS = gpu_timer_query

gpu_timer_query_SOURCES = 	\
	gpu_timer_query.cpp

gpu_timer_query_CXXFLAGS = 	\
	$(OLIB_CXXFLAGS)		\
	$(BOOST_INCLUDE_PATH)	\
	$(GLEW_INCLUDE_PATH)

gpu_timer_query_LDFLAGS =	\
	$(OLIB_LDFLAGS)			\
	$(BOOST_LIB_PATH)		\
	$(GLEW_LIB_PATH)		\
	$(BOOST_LIB_RPATH)		\
	$(GLEW_LIB_RPATH)

gpu_timer_query_LDADD = 										\
	$(OPENGL_LIBS) 												\
	$(OPENGL_GLU_LIBS) 											\
	$(GLEW_LIBS)												\
	$(GLUT_LIBS)												\
	$(BOOST_FILESYSTEM_LIBS)									\
	$(top_builddir)/src/openpluginlib/pl/libopenpluginlib_pl.la	\
	$(DL_LIBS)

endif
