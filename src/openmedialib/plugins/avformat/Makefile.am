
#
#
#

libdir = $(OPENMEDIALIB_PLUGINPATH)

if HAVE_LIBAVFORMAT
lib_LTLIBRARIES = libopenmedialib_avformat.la

libopenmedialib_avformat_la_SOURCES = \
	avformat_plugin.cpp				  \
	# end

libopenmedialib_avformat_la_CXXFLAGS = \
	$(OLIB_CXXFLAGS) 				   \
	$(BOOST_INCLUDE_PATH) 			   \
	$(LIBAVFORMAT_CFLAGS)			   \
	# end

libopenmedialib_avformat_la_LIBADD = 						  \
	$(BOOST_FILESYSTEM_LIBS)								  \
	$(LIBAVFORMAT_LIBS)										  \
	$(top_builddir)/src/openmedialib/ml/libopenmedialib_ml.la \
	$(top_builddir)/src/openimagelib/il/libopenimagelib_il.la \
	# end

libopenmedialib_avformat_la_LDFLAGS = 					\
	$(OLIB_LDFLAGS)							\
	$(BOOST_LIB_PATH)			 			\
	-version-info $(OL_LT_CURRENT):$(OL_LT_REVISION):$(OL_LT_AGE) \
	#end

libopenmedialib_avformat_ladir = $(OPENMEDIALIB_PLUGINPATH)
libopenmedialib_avformat_la_DATA = avformat_plugin.opl

nobase_dist_libopenmedialib_avformat_la_DATA = avformat_plugin.opl

endif
