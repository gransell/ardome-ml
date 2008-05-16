check_PROGRAMS = store

store_SOURCES = 	\
	store.cpp

store_CXXFLAGS = 										  \
	$(OLIB_CXXFLAGS)									  \
	$(BOOST_INCLUDE_PATH)								  \
	-DOPENIMAGELIB_PLUGINS=\"$(OPENIMAGELIB_PLUGINPATH)\" \
	-DOPENMEDIALIB_PLUGINS=\"$(OPENMEDIALIB_PLUGINPATH)\"

store_LDFLAGS =			\
	$(OLIB_LDFLAGS)							\
	$(BOOST_LIB_RPATH)

store_LDADD = 													\
	$(BOOST_FILESYSTEM_LIBS)									\
	$(top_builddir)/src/openpluginlib/pl/libopenpluginlib_pl.la	\
	$(top_builddir)/src/openimagelib/il/libopenimagelib_il.la	\
	$(top_builddir)/src/openmedialib/ml/libopenmedialib_ml.la	\
	$(DL_LIBS)
