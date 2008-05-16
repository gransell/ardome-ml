
#
#
#

check_PROGRAMS = key

key_SOURCES = 	\
	key.cpp

key_CXXFLAGS = \
	$(OLIB_CXXFLAGS) \
	$(BOOST_INCLUDE_PATH)

key_LDFLAGS = \
	$(OLIB_LDFLAGS)		\
	$(BOOST_LIB_RPATH)

key_LDADD =								\
	$(top_builddir)/src/openpluginlib/pl/libopenpluginlib_pl.la	\
	$(DL_LIBS)
