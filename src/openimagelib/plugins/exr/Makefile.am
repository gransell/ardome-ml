
#
#
#

libdir = $(OPENIMAGELIB_PLUGINPATH)

if HAVE_OPENEXR
lib_LTLIBRARIES = libopenimagelib_exr.la

libopenimagelib_exr_la_SOURCES = \
	config.hpp			\
	exr.cpp				\
	exr_plugin.hpp		\
	exr_plugin.cpp

libopenimagelib_exr_la_CXXFLAGS = \
	$(OLIB_CXXFLAGS) \
	$(BOOST_INCLUDE_PATH) \
	$(OPENEXR_CFLAGS)

libopenimagelib_exr_la_LIBADD = \
	$(BOOST_FILESYSTEM_LIBS) 	\
	$(OPENEXR_LIBS)

libopenimagelib_exr_la_LDFLAGS = \
	$(OLIB_LDFLAGS)							\
	$(BOOST_LIB_PATH)			 \
	-version-info $(OL_LT_CURRENT):$(OL_LT_REVISION):$(OL_LT_AGE)

libopenimagelib_exr_ladir = $(OPENIMAGELIB_PLUGINPATH)
libopenimagelib_exr_la_DATA = exr_plugin.opl

nobase_dist_libopenimagelib_exr_la_DATA = exr_plugin.opl

endif
