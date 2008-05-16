
#
#
#

libdir = $(OPENIMAGELIB_PLUGINPATH)

lib_LTLIBRARIES = libopenimagelib_dds.la

libopenimagelib_dds_la_SOURCES = 	\
	config.hpp			\
	dds.cpp				\
	dds_plugin.hpp			\
	dds_plugin.cpp

libopenimagelib_dds_la_CXXFLAGS = 	\
	$(OLIB_CXXFLAGS) 		\
	$(BOOST_INCLUDE_PATH)

libopenimagelib_dds_la_LIBADD = 	\
	$(BOOST_FILESYSTEM_LIBS)

libopenimagelib_dds_la_LDFLAGS = 	\
	$(OLIB_LDFLAGS)			\
	$(BOOST_LIB_PATH)		\
	-version-info $(OL_LT_CURRENT):$(OL_LT_REVISION):$(OL_LT_AGE)

libopenimagelib_dds_ladir = $(OPENIMAGELIB_PLUGINPATH)
libopenimagelib_dds_la_DATA = dds_plugin.opl

nobase_dist_libopenimagelib_dds_la_DATA = dds_plugin.opl
