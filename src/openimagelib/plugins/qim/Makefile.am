
#
#
#

libdir = $(OPENIMAGELIB_PLUGINPATH)

if HAVE_QT

if USE_GPL_COMPONENTS

lib_LTLIBRARIES = libopenimagelib_qim.la

libopenimagelib_qim_la_SOURCES = \
	config.hpp			\
	qim.cpp				\
	qim_plugin.hpp		\
	qim_plugin.cpp

libopenimagelib_qim_la_CXXFLAGS = \
	$(OLIB_CXXFLAGS) \
	$(BOOST_INCLUDE_PATH) \
	$(QT_INCLUDE_PATH)

libopenimagelib_qim_la_LIBADD = \
	$(BOOST_FILESYSTEM_LIBS) 	\
	$(QT_LIBS)

libopenimagelib_qim_la_LDFLAGS = \
	$(OLIB_LDFLAGS)							\
	$(BOOST_LIB_PATH)			 \
	$(QT_LIB_PATH)				 \
	-version-info $(OL_LT_CURRENT):$(OL_LT_REVISION):$(OL_LT_AGE)

libopenimagelib_qim_ladir = $(OPENIMAGELIB_PLUGINPATH)
libopenimagelib_qim_la_DATA = qim_plugin.opl

nobase_dist_libopenimagelib_qim_la_DATA = qim_plugin.opl

endif

endif
