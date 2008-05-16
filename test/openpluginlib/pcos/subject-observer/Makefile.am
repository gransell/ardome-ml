
#
#
#

check_PROGRAMS = subjectobserver

subjectobserver_SOURCES = 	\
	subject-observer.cpp

subjectobserver_CXXFLAGS = \
	$(OLIB_CXXFLAGS) \
	$(BOOST_INCLUDE_PATH)

subjectobserver_LDFLAGS = \
	$(OLIB_LDFLAGS)	\
	$(BOOST_LIB_RPATH)

subjectobserver_LDADD =							\
	$(top_builddir)/src/openpluginlib/pl/libopenpluginlib_pl.la	\
	$(DL_LIBS)
