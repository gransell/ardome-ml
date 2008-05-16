
#
#
#

libdir = $(OPENIMAGELIB_PLUGINPATH)

lib_LTLIBRARIES = libopenimagelib_dpx.la

libopenimagelib_dpx_la_SOURCES = 	\
	config.hpp			\
	dpx.cpp				\
	dpx_plugin.hpp			\
	dpx_plugin.cpp

libopenimagelib_dpx_la_CXXFLAGS = 	\
	$(OLIB_CXXFLAGS) 		\
	$(BOOST_INCLUDE_PATH)

libopenimagelib_dpx_la_LIBADD = 	\
	$(BOOST_FILESYSTEM_LIBS)

libopenimagelib_dpx_la_LDFLAGS = 	\
	$(OLIB_LDFLAGS)			\
	$(BOOST_LIB_PATH)		\
	-version-info $(OL_LT_CURRENT):$(OL_LT_REVISION):$(OL_LT_AGE)

libopenimagelib_dpx_ladir = $(OPENIMAGELIB_PLUGINPATH)
libopenimagelib_dpx_la_DATA = dpx_plugin.opl

nobase_dist_libopenimagelib_dpx_la_DATA = dpx_plugin.opl
