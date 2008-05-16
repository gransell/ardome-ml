#
#
#

libdir = $(OPENMEDIALIB_LIBPATH)

if HAVE_PYTHON

lib_LTLIBRARIES = libopenmedialib_py.la

libopenmedialib_py_la_SOURCES = \
	ml.cpp	\
	py.hpp	\
	py.cpp

libopenmedialib_py_la_CXXFLAGS = \
	$(OLIB_CXXFLAGS) 		\
	$(BOOST_INCLUDE_PATH)	\
	$(PYTHON_INCLUDE_PATH)

libopenmedialib_py_la_LIBADD = 	\
	$(BOOST_PYTHON_LIBS)		\
	$(top_builddir)/src/openmedialib/ml/libopenmedialib_ml.la \
	$(top_builddir)/src/openpluginlib/pl/libopenpluginlib_pl.la

libopenmedialib_py_la_LDFLAGS = \
	$(OLIB_LDFLAGS)							\
	$(BOOST_LIB_PATH)			\
	-version-info $(OL_LT_CURRENT):$(OL_LT_REVISION):$(OL_LT_AGE) \
	-module

endif

install-exec-hook:
if HAVE_PYTHON
	mkdir -p $(DESTDIR)$(prefix)/$(OL_LIBNAME)/python$(PYTHON_VERSION)/site-packages
	ln -sf $(libdir)/libopenmedialib_py.so $(DESTDIR)$(prefix)/$(OL_LIBNAME)/python$(PYTHON_VERSION)/site-packages/openmedialib.so
endif

uninstall-hook:
if HAVE_PYTHON
	rm -rf $(DESTDIR)$(prefix)/$(OL_LIBNAME)/python$(PYTHON_VERSION)/site-packages/openmedialib.so

endif
