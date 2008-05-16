
#
#
#

check_PROGRAMS = discovery

discovery_SOURCES = 	\
	discovery.cpp

discovery_CXXFLAGS = \
	$(OLIB_CXXFLAGS) \
	$(BOOST_INCLUDE_PATH)

discovery_LDFLAGS = \
	$(OLIB_LDFLAGS)		\
	$(BOOST_LIB_RPATH)

discovery_LDADD = 												\
	$(BOOST_FILESYSTEM_LIBS)									\
	$(top_builddir)/src/openpluginlib/pl/libopenpluginlib_pl.la \
	$(DL_LIBS)
