
#
#
#

libdir = $(OPENMEDIALIB_PLUGINPATH)

if HAVE_LIBCACA
lib_LTLIBRARIES = libopenmedialib_caca.la

libopenmedialib_caca_la_SOURCES = \
	caca_plugin.cpp				  \
	# end

libopenmedialib_caca_la_CXXFLAGS = \
	$(CACA_CXXFLAGS) 				   \
	$(OLIB_CXXFLAGS) 				   \
	$(BOOST_INCLUDE_PATH) 			   \
	# end

libopenmedialib_caca_la_LIBADD = 						  \
	$(CACA_LIBS) 				   \
	$(BOOST_FILESYSTEM_LIBS)								  \
	$(top_builddir)/src/openmedialib/ml/libopenmedialib_ml.la \
	$(top_builddir)/src/openimagelib/il/libopenimagelib_il.la \
	# end

libopenmedialib_caca_la_LDFLAGS = 							  \
	$(OLIB_LDFLAGS)							\
	$(BOOST_LIB_PATH)			 								  \
	-version-info $(OL_LT_CURRENT):$(OL_LT_REVISION):$(OL_LT_AGE) \
	#end

libopenmedialib_caca_ladir = $(OPENMEDIALIB_PLUGINPATH)
libopenmedialib_caca_la_DATA = caca_plugin.opl

nobase_dist_libopenmedialib_caca_la_DATA = caca_plugin.opl

endif
