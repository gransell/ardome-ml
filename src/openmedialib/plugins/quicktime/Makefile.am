
#
#
#

libdir = $(OPENMEDIALIB_PLUGINPATH)

if HAVE_QUICKTIME

lib_LTLIBRARIES = libopenmedialib_quicktime.la

libopenmedialib_quicktime_la_SOURCES = \
	quicktime_audiotrack.h			\
	quicktime_audiotrack.cpp			\
	quicktime_plugin.cpp			\
	quicktime_track.h			\
	quicktime_track.cpp			\
	quicktime_input.h			\
	quicktime_input.cpp			\
	quicktime_store.h			\
	quicktime_store.cpp			\
	quicktime_videotrack.h			\
	quicktime_videotrack.cpp

libopenmedialib_quicktime_la_CXXFLAGS = \
	$(OLIB_CXXFLAGS) \
	$(BOOST_INCLUDE_PATH) 

libopenmedialib_quicktime_la_LIBADD = \
	$(BOOST_FILESYSTEM_LIBS)	\
	$(QUICKTIME_LIBS)

libopenmedialib_quicktime_la_LDFLAGS = \
	$(OLIB_LDFLAGS)							\
	$(BOOST_LIB_PATH)			 \
	-version-info $(OL_LT_CURRENT):$(OL_LT_REVISION):$(OL_LT_AGE) \
	-framework QuartzCore

libopenmedialib_quicktime_ladir = $(OPENMEDIALIB_PLUGINPATH)
libopenmedialib_quicktime_la_DATA = quicktime_plugin.opl

nobase_dist_libopenmedialib_quicktime_la_DATA = quicktime_plugin.opl

endif

