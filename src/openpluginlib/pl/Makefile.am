
#
#
#

libdir = $(OPENPLUGINLIB_LIBPATH)

lib_LTLIBRARIES = libopenpluginlib_pl.la

libopenpluginlib_pl_la_SOURCES =		\
	bind_info.hpp				\
	bind_info.cpp				\
	config.hpp				\
	content_handler_libxml.hpp		\
	content_handler_libxml.cpp		\
	fast_math.hpp				\
	fast_math.cpp				\
	discovery_traits.hpp			\
	geometry.hpp				\
	log.hpp						\
	log.cpp						\
	openplugin.hpp				\
	openplugin.cpp				\
	openpluginlib.hpp			\
	openpluginlib.cpp			\
	opl_importer.hpp			\
	opl_importer.cpp			\
	registry.hpp				\
	registry.cpp				\
	utf8_utils.hpp				\
	utf8_utils.cpp				\
	opl_parser_action.hpp			\
	opl_parser_action.cpp			\
	pool.cpp				\
	pool.hpp				\
	string.hpp				\
	timer.hpp				\
	timer.cpp				\
	allocatorstringstorage.h		\
	cowstringopt.h				\
	flex_string_details.h			\
	flex_string.h				\
	flex_string_shell.h			\
	simplestringstorage.h			\
	smallstringopt.h			\
	vectorstringstorage.h			\
	string.hpp				\
	pcos/key.hpp				\
	pcos/key.cpp				\
	pcos/observer.hpp			\
	pcos/property.hpp			\
	pcos/property.cpp			\
	pcos/property_container.hpp		\
	pcos/property_container.cpp		\
	pcos/subject.hpp			\
	pcos/subject.cpp			\
	pcos/any.hpp				\
	pcos/visitor.hpp			\
	pcos/isubject.hpp			\
	pcos/iclonable.hpp

libopenpluginlib_pl_la_CXXFLAGS = 								\
	$(OLIB_CXXFLAGS) 											\
	$(BOOST_INCLUDE_PATH)										\
	$(GLEW_INCLUDE_PATH) 										\
	$(XML2_CXXFLAGS)											\
	$(OFX_INCLUDE_PATH)	\
	-DOPENIMAGELIB_PLUGINS=\"$(OPENIMAGELIB_PLUGINPATH)\" \
	-DOPENMEDIALIB_PLUGINS=\"$(OPENMEDIALIB_PLUGINPATH)\"

libopenpluginlib_pl_la_LIBADD =			\
	$(OPL_EXTRA_LIBS)				\
	$(BOOST_FILESYSTEM_LIBS)		\
	$(BOOST_REGEX_LIBS)			\
	$(BOOST_THREAD_LIBS)			\
	$(XML2_LIBS)

libopenpluginlib_pl_la_LDFLAGS = 		\
	$(OLIB_LDFLAGS)				\
	$(BOOST_LIB_PATH)			\
	-version-info $(OL_LT_CURRENT):$(OL_LT_REVISION):$(OL_LT_AGE)

library_includedir = $(OPENLIBRARIES_INCLUDEPATH)/openpluginlib/pl

nobase_library_include_HEADERS = 			\
	bind_info.hpp					\
	discovery_traits.hpp				\
	openplugin.hpp					\
	openpluginlib.hpp				\
	config.hpp					\
	log.hpp					\
	pool.hpp					\
	utf8_utils.hpp					\
	string.hpp					\
	timer.hpp					\
	allocatorstringstorage.h			\
	cowstringopt.h					\
	flex_string_details.h				\
	flex_string.h					\
	flex_string_shell.h				\
	simplestringstorage.h				\
	smallstringopt.h				\
	vectorstringstorage.h				\
	geometry.hpp					\
	string.hpp					\
	pcos/key.hpp					\
	pcos/observer.hpp				\
	pcos/iproperty_container.hpp 			\
	pcos/property.hpp				\
	pcos/property_container.hpp			\
	pcos/subject.hpp				\
	pcos/any.hpp					\
	pcos/visitor.hpp				\
	pcos/isubject.hpp				\
	pcos/iclonable.hpp
