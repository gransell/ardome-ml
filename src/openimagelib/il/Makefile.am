
#
#
#

libdir = $(OPENIMAGELIB_LIBPATH)

lib_LTLIBRARIES = libopenimagelib_il.la

libopenimagelib_il_la_SOURCES = \
	basic_image.hpp			\
	compression_traits.hpp	\
	config.hpp				\
	float_traits.hpp		\
	il.hpp					\
	il.cpp					\
	noise_traits.hpp		\
	openimagelib_plugin.hpp	\
	openimagelib_plugin.cpp	\
	rgb_traits.hpp			\
	traits.hpp				\
	utility.cpp				\
	utility.hpp				\
	yuv_traits.hpp

libopenimagelib_il_la_CXXFLAGS = \
	$(OLIB_CXXFLAGS) 			\
	$(BOOST_INCLUDE_PATH)

libopenimagelib_il_la_LDFLAGS = \
	$(OLIB_LDFLAGS)			\
	$(BOOST_LIB_PATH)		\
	$(OIL_EXTRA_LIBS)		\
	-version-info $(OL_LT_CURRENT):$(OL_LT_REVISION):$(OL_LT_AGE)

library_includedir = $(OPENLIBRARIES_INCLUDEPATH)/openimagelib/il

library_include_HEADERS = 	\
	basic_image.hpp			\
	compression_traits.hpp	\
	config.hpp				\
	float_traits.hpp		\
	il.hpp					\
	noise_traits.hpp		\
	openimagelib_plugin.hpp	\
	rgb_traits.hpp			\
	traits.hpp				\
	utility.hpp				\
	yuv_traits.hpp
