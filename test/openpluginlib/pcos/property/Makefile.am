
#
#
#

check_PROGRAMS = property

property_SOURCES = 	\
	property.cpp

property_CXXFLAGS = \
	$(OLIB_CXXFLAGS) \
	$(BOOST_INCLUDE_PATH)

property_LDFLAGS = \
	$(OLIB_LDFLAGS)		\
	$(BOOST_LIB_RPATH)

property_LDADD =							\
	$(top_builddir)/src/openpluginlib/pl/libopenpluginlib_pl.la	\
	$(DL_LIBS)
