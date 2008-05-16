
#
#
#

libdir = $(OPENIMAGELIB_PLUGINPATH)

if HAVE_QUICKTIME

lib_LTLIBRARIES = libopenimagelib_quicktime.la

libopenimagelib_quicktime_la_SOURCES = \
	config.hpp			\
	quicktime.cpp				\
	quicktime_plugin.hpp		\
	quicktime_plugin.cpp

libopenimagelib_quicktime_la_CXXFLAGS = \
	$(OLIB_CXXFLAGS) \
	$(BOOST_INCLUDE_PATH) 

libopenimagelib_quicktime_la_LIBADD = \
	$(BOOST_FILESYSTEM_LIBS)	\
	$(QUICKTIME_LIBS)

libopenimagelib_quicktime_la_LDFLAGS = \
	$(OLIB_LDFLAGS)							\
	$(BOOST_LIB_PATH)			 \
	-version-info $(OL_LT_CURRENT):$(OL_LT_REVISION):$(OL_LT_AGE)

libopenimagelib_quicktime_ladir = $(OPENIMAGELIB_PLUGINPATH)
libopenimagelib_quicktime_la_DATA = quicktime_plugin.opl

nobase_dist_libopenimagelib_quicktime_la_DATA = quicktime_plugin.opl

endif

