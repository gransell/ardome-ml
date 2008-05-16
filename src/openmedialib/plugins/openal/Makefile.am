
#
#
#

libdir = $(OPENMEDIALIB_PLUGINPATH)

if HAVE_OPENAL
lib_LTLIBRARIES = libopenmedialib_openal.la

libopenmedialib_openal_la_SOURCES = \
	openal_plugin.cpp

libopenmedialib_openal_la_CXXFLAGS = 	\
	$(OLIB_CXXFLAGS) 				 	\
	$(BOOST_INCLUDE_PATH)				\
	$(OPENAL_INCLUDE_PATH)				\
	$(GLEW_INCLUDE_PATH)				\
	$(SWAB_AUDIO)

libopenmedialib_openal_la_LIBADD = 						  		\
	$(BOOST_FILESYSTEM_LIBS)								  	\
	$(OPENAL_LIBS)											  	\
	$(top_builddir)/src/openmedialib/ml/libopenmedialib_ml.la 	\
	$(top_builddir)/src/openimagelib/il/libopenimagelib_il.la 

libopenmedialib_openal_la_LDFLAGS = 							  \
	$(OLIB_LDFLAGS)							\
	$(BOOST_LIB_PATH)			 								  \
	$(OPENAL_LIB_PATH)			 								  \
	-version-info $(OL_LT_CURRENT):$(OL_LT_REVISION):$(OL_LT_AGE) 

libopenmedialib_openal_ladir = $(OPENMEDIALIB_PLUGINPATH)
libopenmedialib_openal_la_DATA = openal_plugin.opl

nobase_dist_libopenmedialib_openal_la_DATA = openal_plugin.opl

endif
