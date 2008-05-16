
#
#
#

libdir = $(OPENMEDIALIB_PLUGINPATH)

lib_LTLIBRARIES = libopenmedialib_oil.la

libopenmedialib_oil_la_SOURCES = \
	oil_plugin.cpp				  \
	# end

libopenmedialib_oil_la_CXXFLAGS = \
	$(OLIB_CXXFLAGS) 				   \
	$(BOOST_INCLUDE_PATH) 			   \
	# end

libopenmedialib_oil_la_LIBADD = 						  \
	$(BOOST_FILESYSTEM_LIBS)								  \
	$(top_builddir)/src/openmedialib/ml/libopenmedialib_ml.la \
	$(top_builddir)/src/openimagelib/il/libopenimagelib_il.la \
	# end

libopenmedialib_oil_la_LDFLAGS = 							  \
	$(OLIB_LDFLAGS)							\
	$(BOOST_LIB_PATH)			 								  \
	-version-info $(OL_LT_CURRENT):$(OL_LT_REVISION):$(OL_LT_AGE) \
	#end

libopenmedialib_oil_ladir = $(OPENMEDIALIB_PLUGINPATH)
libopenmedialib_oil_la_DATA = oil_plugin.opl

nobase_dist_libopenmedialib_oil_la_DATA = oil_plugin.opl
