libdir = $(OPENMEDIALIB_LIBPATH)

lib_LTLIBRARIES = libopenmedialib_ml.la

libopenmedialib_ml_la_SOURCES = \
    audio.hpp	              	\
    config.hpp              	\
	filter.hpp					\
	filter.cpp					\
	frame.hpp					\
	frame.cpp					\
	input.hpp					\
    ml.cpp                  	\
    ml.hpp                  	\
	openmedialib_plugin.cpp		\
	openmedialib_plugin.hpp		\
	store.hpp					\
	utilities.cpp				\
	utilities.hpp				\
	# end

libopenmedialib_ml_la_CXXFLAGS = \
    $(OLIB_CXXFLAGS) 			 \
    $(BOOST_INCLUDE_PATH) 		 \
    # end

libopenmedialib_ml_la_LDFLAGS = \
    $(OLIB_LDFLAGS)		\
    $(BOOST_LIB_PATH)   \
	$(OML_EXTRA_LIBS)	\
    -version-info $(OL_LT_CURRENT):$(OL_LT_REVISION):$(OL_LT_AGE)

library_includedir = $(OPENLIBRARIES_INCLUDEPATH)/openmedialib/ml

library_include_HEADERS =   \
    audio.hpp	           	\
    config.hpp              \
	filter.hpp				\
	frame.hpp				\
	input.hpp				\
    ml.hpp                  \
	openmedialib_plugin.hpp	\
	store.hpp				\
	utilities.hpp			\
	# end

