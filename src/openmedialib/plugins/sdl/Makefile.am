
#
#
#

libdir = $(OPENMEDIALIB_PLUGINPATH)

if HAVE_LIBSDL
lib_LTLIBRARIES = libopenmedialib_sdl.la

libopenmedialib_sdl_la_SOURCES = \
	sdl_plugin.cpp				  \
	# end

libopenmedialib_sdl_la_CXXFLAGS = \
	$(SDL_CXXFLAGS) 				   \
	$(OLIB_CXXFLAGS) 				   \
	$(BOOST_INCLUDE_PATH) 			   \
	# end

libopenmedialib_sdl_la_LIBADD = 						  \
	$(SDL_LIBS) 				   \
	$(BOOST_FILESYSTEM_LIBS)								  \
	$(top_builddir)/src/openmedialib/ml/libopenmedialib_ml.la \
	$(top_builddir)/src/openimagelib/il/libopenimagelib_il.la \
	# end

libopenmedialib_sdl_la_LDFLAGS = 							  \
	$(OLIB_LDFLAGS)							\
	$(BOOST_LIB_PATH)			 								  \
	-version-info $(OL_LT_CURRENT):$(OL_LT_REVISION):$(OL_LT_AGE) \
	#end

libopenmedialib_sdl_ladir = $(OPENMEDIALIB_PLUGINPATH)
libopenmedialib_sdl_la_DATA = sdl_plugin.opl

nobase_dist_libopenmedialib_sdl_la_DATA = sdl_plugin.opl

endif
