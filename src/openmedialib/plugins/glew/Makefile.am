
#
#
#

libdir = $(OPENMEDIALIB_PLUGINPATH)

if HAVE_GL_GLUT_H
lib_LTLIBRARIES = libopenmedialib_glew.la

libopenmedialib_glew_la_SOURCES = \
	glew_plugin.cpp				  \
	# end

libopenmedialib_glew_la_CXXFLAGS = \
	$(GLEW_INCLUDE_PATH)		   \
	$(OLIB_CXXFLAGS) 			   \
	$(BOOST_INCLUDE_PATH) 		   \
	# end

libopenmedialib_glew_la_LIBADD = 						  \
	$(OPENGL_LIBS) 												\
	$(OPENGL_GLU_LIBS) 											\
	$(GLEW_LIBS)												\
	$(GLUT_LIBS)												\
	$(BOOST_FILESYSTEM_LIBS)								  \
	$(top_builddir)/src/openmedialib/ml/libopenmedialib_ml.la \
	$(top_builddir)/src/openimagelib/il/libopenimagelib_il.la \
	# end

libopenmedialib_glew_la_LDFLAGS = 							  \
	$(OLIB_LDFLAGS)							\
	$(BOOST_LIB_PATH)			 								  \
	$(GLEW_LIB_PATH)											  \
	-version-info $(OL_LT_CURRENT):$(OL_LT_REVISION):$(OL_LT_AGE)

libopenmedialib_glew_ladir = $(OPENMEDIALIB_PLUGINPATH)
libopenmedialib_glew_la_DATA = glew_plugin.opl

nobase_dist_libopenmedialib_glew_la_DATA = glew_plugin.opl

endif
