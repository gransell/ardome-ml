
#ifndef AMF_FILTER_PLUGIN_H
#define AMF_FILTER_PLUGIN_H

#ifdef HAVE_CONFIG_H
#include <config.hpp>
#endif

#ifdef WIN32

#	define _WIN32_WINNT 0x0501
#	include <WinSock2.h>
#	include <windows.h>
#   include <tchar.h>
#	define _USE_MATH_DEFINES
#	include <io.h>
#	include <fcntl.h>
#	define atoll		_atoi64

#	define aml_open		_topen
#	define aml_read		_read
#	define aml_write	_write
#	define aml_lseek	_lseeki64
#	define aml_close	_close

#else

#	include <sys/types.h>
#	include <sys/stat.h>
#	include <fcntl.h>

#	define aml_open		::open
#	define aml_read		::read
#	define aml_write	::write
#	define aml_lseek	::lseek
#	define aml_close	::close

#endif 

#include <boost/cstdint.hpp>

#include <openmedialib/ml/openmedialib_plugin.hpp>
#include <openmedialib/ml/filter_simple.hpp>
#include <openpluginlib/pl/pcos/isubject.hpp>
#include <openpluginlib/pl/pcos/observer.hpp>

namespace ml = olib::openmedialib::ml;
namespace pl = olib::openpluginlib;
namespace il = olib::openimagelib::il;
namespace cl = olib::opencorelib;
namespace pcos = olib::openpluginlib::pcos;

namespace aml { namespace openmedialib {

extern bool contains_sub_thread( ml::input_type_ptr input );
extern void activate_sub_thread( ml::input_type_ptr input, int value );

} }

#endif
