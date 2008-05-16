
#
#
#

libdir = $(OPENIMAGELIB_PLUGINPATH)

lib_LTLIBRARIES = libopenimagelib_hdr.la

libopenimagelib_hdr_la_SOURCES = \
	config.hpp			\
	hdr.cpp				\
	hdr_plugin.hpp		\
	hdr_plugin.cpp

libopenimagelib_hdr_la_CXXFLAGS = \
	$(OLIB_CXXFLAGS) \
	$(BOOST_INCLUDE_PATH) \
	# end

libopenimagelib_hdr_la_LIBADD = \
	$(BOOST_FILESYSTEM_LIBS)

libopenimagelib_hdr_la_LDFLAGS = \
	$(OLIB_LDFLAGS)							\
	$(BOOST_LIB_PATH)			 \
	-version-info $(OL_LT_CURRENT):$(OL_LT_REVISION):$(OL_LT_AGE)

libopenimagelib_hdr_ladir = $(OPENIMAGELIB_PLUGINPATH)
libopenimagelib_hdr_la_DATA = hdr_plugin.opl

nobase_dist_libopenimagelib_hdr_la_DATA = hdr_plugin.opl
