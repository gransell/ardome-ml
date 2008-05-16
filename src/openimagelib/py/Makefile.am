
#
#
#

libdir = $(OPENIMAGELIB_LIBPATH)

if HAVE_PYTHON

lib_LTLIBRARIES = libopenimagelib_py.la

libopenimagelib_py_la_SOURCES = \
	il.cpp	\
	py.hpp	\
	py.cpp

libopenimagelib_py_la_CXXFLAGS = \
	$(OLIB_CXXFLAGS) 		\
	$(BOOST_INCLUDE_PATH)	\
	$(PYTHON_INCLUDE_PATH)

libopenimagelib_py_la_LIBADD = 	\
	$(BOOST_PYTHON_LIBS)		\
	$(top_builddir)/src/openimagelib/il/libopenimagelib_il.la

libopenimagelib_py_la_LDFLAGS = \
	$(OLIB_LDFLAGS)							\
	$(BOOST_LIB_PATH)			\
	-version-info $(OL_LT_CURRENT):$(OL_LT_REVISION):$(OL_LT_AGE) \
	-module

endif

install-exec-hook:
if HAVE_PYTHON
	mkdir -p $(DESTDIR)$(prefix)/$(OL_LIBNAME)/python$(PYTHON_VERSION)/site-packages
	ln -sf $(libdir)/libopenimagelib_py.so $(DESTDIR)$(prefix)/$(OL_LIBNAME)/python$(PYTHON_VERSION)/site-packages/openimagelib.so
endif

uninstall-hook:
if HAVE_PYTHON
	rm -rf $(DESTDIR)$(prefix)/$(OL_LIBNAME)/python$(PYTHON_VERSION)/site-packages/openimagelib.so

endif
