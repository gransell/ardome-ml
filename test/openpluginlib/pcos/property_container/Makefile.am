
#
#
#

check_PROGRAMS = property_container

property_container_SOURCES = 	\
	property_container.cpp

property_container_CXXFLAGS = \
	$(OLIB_CXXFLAGS) \
	$(BOOST_INCLUDE_PATH)

property_container_LDFLAGS = \
	$(OLIB_LDFLAGS)		\
	$(BOOST_LIB_RPATH)

property_container_LDADD =						\
	$(top_builddir)/src/openpluginlib/pl/libopenpluginlib_pl.la	\
	$(DL_LIBS)
