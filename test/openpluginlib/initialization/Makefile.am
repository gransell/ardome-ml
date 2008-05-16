
#
#
#

check_PROGRAMS = initialization

initialization_SOURCES = 	\
	initialization.cpp

initialization_CXXFLAGS = \
	$(OLIB_CXXFLAGS) \
	$(BOOST_INCLUDE_PATH) \
	# end

initialization_LDFLAGS = \
	$(OLIB_LDFLAGS)	\
	$(BOOST_LIB_RPATH)

initialization_LDADD = 											\
	$(BOOST_FILESYSTEM_LIBS)									\
	$(top_builddir)/src/openpluginlib/pl/libopenpluginlib_pl.la \
	$(DL_LIBS)
