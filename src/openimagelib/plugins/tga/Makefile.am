
#
#
#

libdir = $(OPENIMAGELIB_PLUGINPATH)

lib_LTLIBRARIES = libopenimagelib_tga.la

libopenimagelib_tga_la_SOURCES = 	\
	config.hpp			\
	tga.cpp				\
	tga_plugin.hpp			\
	tga_plugin.cpp

libopenimagelib_tga_la_CXXFLAGS = 	\
	$(OLIB_CXXFLAGS) 		\
	$(BOOST_INCLUDE_PATH)

libopenimagelib_tga_la_LIBADD = 	\
	$(BOOST_FILESYSTEM_LIBS)

libopenimagelib_tga_la_LDFLAGS = 					\
	$(OLIB_LDFLAGS)							\
	$(BOOST_LIB_PATH)			 			\
	-version-info $(OL_LT_CURRENT):$(OL_LT_REVISION):$(OL_LT_AGE)

libopenimagelib_tga_ladir = $(OPENIMAGELIB_PLUGINPATH)
libopenimagelib_tga_la_DATA = tga_plugin.opl

nobase_dist_libopenimagelib_tga_la_DATA = tga_plugin.opl
