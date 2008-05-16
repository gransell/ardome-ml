
#
#
#

libdir = $(OPENIMAGELIB_PLUGINPATH)

lib_LTLIBRARIES = libopenimagelib_3D_lightmap.la

libopenimagelib_3D_lightmap_la_SOURCES = \
	config.hpp							 \
	3D_lightmap.cpp						 \
	3D_lightmap_plugin.hpp				 \
	3D_lightmap_plugin.cpp

libopenimagelib_3D_lightmap_la_CXXFLAGS = \
	$(OLIB_CXXFLAGS) \
	$(BOOST_INCLUDE_PATH) \
	# end

libopenimagelib_3D_lightmap_la_LIBADD = \
	$(BOOST_FILESYSTEM_LIBS)

libopenimagelib_3D_lightmap_la_LDFLAGS = \
	$(OLIB_LDFLAGS)							\
	$(BOOST_LIB_PATH)					 \
	-version-info $(OL_LT_CURRENT):$(OL_LT_REVISION):$(OL_LT_AGE)

libopenimagelib_3D_lightmap_ladir = $(OPENIMAGELIB_PLUGINPATH)
libopenimagelib_3D_lightmap_la_DATA = 3D_lightmap_plugin.opl

nobase_dist_libopenimagelib_3D_lightmap_la_DATA = 3D_lightmap_plugin.opl
