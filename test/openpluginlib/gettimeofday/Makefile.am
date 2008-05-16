
#
#
#

check_PROGRAMS = gettimeofday

gettimeofday_SOURCES = 	\
	gettimeofday.cpp

gettimeofday_CXXFLAGS =		\
	$(OLIB_CXXFLAGS) 		\
	$(BOOST_INCLUDE_PATH)	\
	$(GLEW_INCLUDE_PATH)

gettimeofday_LDFLAGS = 	\
	$(OLIB_LDFLAGS)		\
	$(GLEW_LIB_PATH)	\
	$(BOOST_LIB_RPATH)	\
	$(GLEW_LIB_RPATH)

gettimeofday_LDADD =	 										\
	$(BOOST_FILESYSTEM_LIBS)									\
	$(top_builddir)/src/openpluginlib/pl/libopenpluginlib_pl.la	\
	$(DL_LIBS)
