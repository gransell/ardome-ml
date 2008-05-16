
#
#
#

libdir = $(OPENMEDIALIB_PLUGINPATH)

if HAVE_LIBMLT
lib_LTLIBRARIES = libopenmedialib_mlt.la

libopenmedialib_mlt_la_SOURCES = \
	mlt_plugin.cpp				  \
	# end

libopenmedialib_mlt_la_CXXFLAGS = \
	$(OLIB_CXXFLAGS) 				   \
	$(BOOST_INCLUDE_PATH) 			   \
	$(MLT_CXXFLAGS)			   \
	# end

libopenmedialib_mlt_la_LIBADD = 						  \
	$(BOOST_FILESYSTEM_LIBS)								  \
	$(MLT_LIBS)										  \
	$(top_builddir)/src/openmedialib/ml/libopenmedialib_ml.la \
	$(top_builddir)/src/openimagelib/il/libopenimagelib_il.la \
	# end

libopenmedialib_mlt_la_LDFLAGS = 							  \
	$(OLIB_LDFLAGS)							\
	$(BOOST_LIB_PATH)			 								  \
	-version-info $(OL_LT_CURRENT):$(OL_LT_REVISION):$(OL_LT_AGE) \
	#end

libopenmedialib_mlt_ladir = $(OPENMEDIALIB_PLUGINPATH)
libopenmedialib_mlt_la_DATA = mlt_plugin.opl

nobase_dist_libopenmedialib_mlt_la_DATA = mlt_plugin.opl

endif
