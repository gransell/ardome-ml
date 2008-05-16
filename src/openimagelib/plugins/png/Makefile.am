
#
#
#

libdir = $(OPENIMAGELIB_PLUGINPATH)

if HAVE_LIBPNG
lib_LTLIBRARIES = libopenimagelib_png.la

libopenimagelib_png_la_SOURCES = \
	config.hpp			\
	png.cpp				\
	png_plugin.hpp		\
	png_plugin.cpp

libopenimagelib_png_la_CXXFLAGS = \
	$(PNG_CFLAGS) \
	$(OLIB_CXXFLAGS) \
	$(BOOST_INCLUDE_PATH)

libopenimagelib_png_la_LIBADD = \
	$(BOOST_FILESYSTEM_LIBS) 	\
	$(PNG_LIBS)

libopenimagelib_png_la_LDFLAGS = \
	$(OLIB_LDFLAGS)							\
	$(BOOST_LIB_PATH)			 \
	-version-info $(OL_LT_CURRENT):$(OL_LT_REVISION):$(OL_LT_AGE)

libopenimagelib_png_ladir = $(OPENIMAGELIB_PLUGINPATH)
libopenimagelib_png_la_DATA = png_plugin.opl

nobase_dist_libopenimagelib_png_la_DATA = png_plugin.opl

endif
