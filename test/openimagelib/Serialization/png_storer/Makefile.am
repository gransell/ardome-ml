
#
#
#

check_PROGRAMS = png_storer

png_storer_SOURCES = 	\
	png_storer.cpp

png_storer_CXXFLAGS = 		\
	$(OLIB_CXXFLAGS) 		\
	$(BOOST_INCLUDE_PATH) 	\
	-DOPENIMAGELIB_PLUGINS=\"$(OPENIMAGELIB_PLUGINPATH)\"

png_storer_LDADD = 												\
	$(BOOST_FILESYSTEM_LIBS)									\
	$(top_builddir)/src/openpluginlib/pl/libopenpluginlib_pl.la	\
	$(DL_LIBS)
