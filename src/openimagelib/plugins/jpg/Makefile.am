
#
#
#

libdir = $(OPENIMAGELIB_PLUGINPATH)

if HAVE_LIBJPEG
lib_LTLIBRARIES = libopenimagelib_jpg.la

libopenimagelib_jpg_la_SOURCES = \
	config.hpp			\
	jpg.cpp				\
	jpg_plugin.hpp		\
	jpg_plugin.cpp

libopenimagelib_jpg_la_CXXFLAGS = \
	$(OLIB_CXXFLAGS) \
	$(BOOST_INCLUDE_PATH) \
	# end

libopenimagelib_jpg_la_LIBADD = 	\
	$(BOOST_FILESYSTEM_LIBS)		\
	$(JPEG_LIBS)					\
	$(top_builddir)/src/openimagelib/il/libopenimagelib_il.la

libopenimagelib_jpg_la_LDFLAGS = 	\
	$(OLIB_LDFLAGS)			\
	$(BOOST_LIB_PATH)		\
	-version-info $(OL_LT_CURRENT):$(OL_LT_REVISION):$(OL_LT_AGE)

libopenimagelib_jpg_ladir = $(OPENIMAGELIB_PLUGINPATH)
libopenimagelib_jpg_la_DATA = jpg_plugin.opl

nobase_dist_libopenimagelib_jpg_la_DATA = jpg_plugin.opl

endif
