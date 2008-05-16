
#
#
#

libdir = $(OPENPLUGINLIB_LIBPATH)

if HAVE_PYTHON

lib_LTLIBRARIES = libopenpluginlib_py.la

libopenpluginlib_py_la_SOURCES =	\
	geometry.cpp			\
	py.hpp					\
	py.cpp					\
	python.hpp				\
	openpluginlib.cpp		\
	string.cpp			\
	pcos/key.cpp			\
	pcos/observer.cpp		\
	pcos/subject.cpp		\
	pcos/property.cpp		\
	pcos/any.cpp			\
	pcos/property_container.cpp	\
	pcos/visitor.cpp

libopenpluginlib_py_la_CXXFLAGS = 	\
	$(OLIB_CXXFLAGS) 		\
	$(BOOST_INCLUDE_PATH)	\
	$(XML2_CXXFLAGS)		\
	$(PYTHON_INCLUDE_PATH)

libopenpluginlib_py_la_LIBADD =	\
	$(BOOST_FILESYSTEM_LIBS)	\
	$(BOOST_REGEX_LIBS)			\
	$(BOOST_THREAD_LIBS)		\
	$(BOOST_PYTHON_LIBS)		\
	$(CG_LIBS)					\
	$(XML2_LIBS)				\
	$(top_builddir)/src/openpluginlib/pl/libopenpluginlib_pl.la

libopenpluginlib_py_la_LDFLAGS = 	\
	$(OLIB_LDFLAGS)					\
	$(BOOST_LIB_PATH)				\
	-version-info $(OL_LT_CURRENT):$(OL_LT_REVISION):$(OL_LT_AGE) \
	-module

endif

install-exec-hook:
if HAVE_PYTHON
	mkdir -p $(DESTDIR)$(prefix)/$(OL_LIBNAME)/python$(PYTHON_VERSION)/site-packages
	ln -sf $(libdir)/libopenpluginlib_py.so $(DESTDIR)$(prefix)/$(OL_LIBNAME)/python$(PYTHON_VERSION)/site-packages/openpluginlib.so
endif

uninstall-hook:
if HAVE_PYTHON
	rm -rf $(DESTDIR)$(prefix)/$(OL_LIBNAME)/python$(PYTHON_VERSION)/site-packages/openpluginlib.so
endif

