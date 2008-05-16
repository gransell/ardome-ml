
#
#
#

libdir = $(OPENIMAGELIB_PLUGINPATH)

lib_LTLIBRARIES = libopenimagelib_sgi.la

libopenimagelib_sgi_la_SOURCES = \
	config.hpp			\
	sgi.cpp				\
	sgi_plugin.hpp		\
	sgi_plugin.cpp

libopenimagelib_sgi_la_CXXFLAGS = \
	$(OLIB_CXXFLAGS) \
	$(BOOST_INCLUDE_PATH) \
	# end

libopenimagelib_sgi_la_LIBADD = 	\
	$(BOOST_FILESYSTEM_LIBS)

libopenimagelib_sgi_la_LDFLAGS = \
	$(OLIB_LDFLAGS)							\
	$(BOOST_LIB_PATH)			 \
	-version-info $(OL_LT_CURRENT):$(OL_LT_REVISION):$(OL_LT_AGE)

libopenimagelib_sgi_ladir = $(OPENIMAGELIB_PLUGINPATH)
libopenimagelib_sgi_la_DATA = sgi_plugin.opl

nobase_dist_libopenimagelib_sgi_la_DATA = sgi_plugin.opl
