
#
#
#

libdir = $(OPENMEDIALIB_PLUGINPATH)

lib_LTLIBRARIES = libopenmedialib_template.la

libopenmedialib_template_la_SOURCES = \
	template_plugin.cpp				  \
	# end

libopenmedialib_template_la_CXXFLAGS = \
	$(OLIB_CXXFLAGS) 				   \
	$(BOOST_INCLUDE_PATH) 			   \
	# end

libopenmedialib_template_la_LIBADD = 						  \
	$(BOOST_FILESYSTEM_LIBS)								  \
	$(top_builddir)/src/openmedialib/ml/libopenmedialib_ml.la \
	$(top_builddir)/src/openimagelib/il/libopenimagelib_il.la \
	# end

libopenmedialib_template_la_LDFLAGS = 							  \
	$(OLIB_LDFLAGS)							\
	$(BOOST_LIB_PATH)			 								  \
	-version-info $(OL_LT_CURRENT):$(OL_LT_REVISION):$(OL_LT_AGE) \
	#end

libopenmedialib_template_ladir = $(OPENMEDIALIB_PLUGINPATH)
libopenmedialib_template_la_DATA = template_plugin.opl

nobase_dist_libopenmedialib_template_la_DATA = template_plugin.opl
