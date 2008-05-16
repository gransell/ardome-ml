
#
#
#

check_PROGRAMS = enumerate

enumerate_SOURCES = 	\
	enumerate.cpp

enumerate_CXXFLAGS = \
	$(OLIB_CXXFLAGS) \
	$(BOOST_INCLUDE_PATH)

enumerate_LDFLAGS = \
	$(OLIB_LDFLAGS)	\
	$(BOOST_LIB_RPATH)

enumerate_LDADD = 												\
	$(BOOST_FILESYSTEM_LIBS)									\
	$(top_builddir)/src/openpluginlib/pl/libopenpluginlib_pl.la	\
	$(DL_LIBS)
