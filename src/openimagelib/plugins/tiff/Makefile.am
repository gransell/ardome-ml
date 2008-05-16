
#
#
#

libdir = $(OPENIMAGELIB_PLUGINPATH)

if HAVE_LIBTIFF

lib_LTLIBRARIES = libopenimagelib_tiff.la

libopenimagelib_tiff_la_SOURCES = 	\
	config.hpp			\
	tiff.cpp			\
	tiff_plugin.hpp		\
	tiff_plugin.cpp

libopenimagelib_tiff_la_CXXFLAGS = 	\
	$(OLIB_CXXFLAGS) 		\
	$(BOOST_INCLUDE_PATH)

libopenimagelib_tiff_la_LIBADD = 	\
	$(BOOST_FILESYSTEM_LIBS)		\
	$(TIFF_LIBS)

libopenimagelib_tiff_la_LDFLAGS = 			\
	$(OLIB_LDFLAGS)							\
	$(BOOST_LIB_PATH)			 			\
	-version-info $(OL_LT_CURRENT):$(OL_LT_REVISION):$(OL_LT_AGE)

libopenimagelib_tiff_ladir = $(OPENIMAGELIB_PLUGINPATH)
libopenimagelib_tiff_la_DATA = tiff_plugin.opl

nobase_dist_libopenimagelib_tiff_la_DATA = tiff_plugin.opl

endif
