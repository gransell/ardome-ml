
// openpluginlib - A plugin interface to openlibraries.

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#include <openpluginlib/py/python.hpp>
#include <openpluginlib/py/py.hpp>

#include <openpluginlib/pl/openpluginlib.hpp>

namespace opl	= olib::openpluginlib;
namespace py	= boost::python;

typedef std::list< std::string > string_list;

namespace olib { namespace openpluginlib { namespace detail {

struct all_query_traits
{
	all_query_traits( const std::wstring& libname = L"", const std::wstring& type = L"", const std::wstring& to_match = L"", int merit = 0 )
		: libname_( libname )
		, type_( type )
		, to_match_( to_match )
		, merit_( merit )
	{ }

	std::wstring libname( ) const
	{ return libname_; }
	std::wstring type( ) const
	{ return type_; }
	std::wstring to_match( ) const
	{ return to_match_; }
	int merit( ) const
	{ return merit_; }
	
	std::wstring libname_, type_, to_match_;
	int merit_;
};

// Wrap init to allow zero args (boost::python does not preserve default args)
bool init0() { return init( "" ); }
bool init1( const std::string& path ) { return init( path ); }
bool initlist( const string_list& paths ) { return init( paths ); };
#ifdef WIN32
bool init2( const std::string& path, const std::string& kernels_path, const std::string& shaders_path ) { return init( path, kernels_path, shaders_path ); }
bool initlist1( const string_list& paths, const std::string& kernels_path, const std::string& shaders_path ) { return init( paths, kernels_path, shaders_path ); }
#endif

void py_openpluginlib( )
{
	py::def( "init", &init0 );
	py::def( "init", &init1 );
	py::def( "init", &initlist );
	py::def( "init_log", &opl::init_log );
	py::def( "uninit", &uninit );
	py::def( "registered_filters", &registered_filters );
#ifdef WIN32
	py::def( "init", &init2 );
	py::def( "init", &initlist1 );
#endif
	py::def( "set_log_level", &opl::set_log_level );
	
	py::class_<detail::all_query_traits>( "all_query_traits", py::init<std::wstring, std::wstring, std::wstring, int>( ) )
		.def( "libname", &detail::all_query_traits::libname )
		.def( "type", &detail::all_query_traits::type )
		.def( "to_match", &detail::all_query_traits::to_match )
		.def( "merit", &detail::all_query_traits::merit )
		;
	
	typedef detail::discover_query_impl::plugin_proxy plugin_proxy;
	opl_ptr ( plugin_proxy::*plugin_creator )( const std::string& ) const = &plugin_proxy::create_plugin;
	
	py::class_<openplugin, boost::noncopyable, opl_ptr>( "openplugin", py::init<>( ) )
		;
	
	std::vector< std::wstring >::iterator ( std::vector< std::wstring >::*begin )( ) = &std::vector< std::wstring >::begin;
	std::vector< std::wstring >::iterator ( std::vector< std::wstring >::*end )( ) = &std::vector< std::wstring >::end;

	py::class_< std::vector< std::wstring > >( "string_array", py::init< std::vector< std::wstring > >() )
		.def( "__iter__", py::range( begin, end ) );

	py::class_<plugin_proxy>( "plugin_proxy", py::no_init )
		.def( "create_plugin", plugin_creator )
		.def( "name", &plugin_proxy::name )
		.def( "type", &plugin_proxy::type )
		.def( "mime", &plugin_proxy::mime )
		.def( "category", &plugin_proxy::category )
		.def( "libname", &plugin_proxy::libname )
		.def( "in_filter", &plugin_proxy::in_filter )
		.def( "out_filter", &plugin_proxy::out_filter )
		.def( "extension", &plugin_proxy::extension )
		.def( "merit", &plugin_proxy::merit )
		.def( "filenames", &plugin_proxy::filenames )
		;

	typedef opl::discovery<detail::all_query_traits> all_discovery;
	py::class_<all_discovery>( "discovery", py::init<detail::all_query_traits>( ) )
		.def( "__iter__", py::range( &all_discovery::begin, &all_discovery::end ) )
		.def( "empty", &all_discovery::empty )
		.def( "size", &all_discovery::size )
		;
}

} } }
