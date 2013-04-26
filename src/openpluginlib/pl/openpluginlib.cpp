
// openpluginlib - A plugin interface to openlibraries.

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#ifdef HAVE_CONFIG_H
#include <openlibraries_global_config.hpp>
#endif

#ifdef WIN32
#define STRICT
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shlobj.h>
#endif

#include <algorithm>
#include <cassert>
#include <functional>

#include <boost/bind.hpp>
#ifndef BOOST_FILESYSTEM_DYN_LINK 
    #define BOOST_FILESYSTEM_DYN_LINK 
#endif

#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <boost/tokenizer.hpp>

#include <opencorelib/cl/core.hpp>
#include <opencorelib/cl/str_util.hpp>
#include <opencorelib/cl/profile.hpp>
#include <opencorelib/cl/enforce_defines.hpp>

#include <opencorelib/cl/logger.hpp>
#include <opencorelib/cl/loghandler.hpp>
#include <opencorelib/cl/logtarget.hpp>
#include <opencorelib/cl/logprinter.hpp>

#include <openpluginlib/pl/openpluginlib.hpp>
#include <openpluginlib/pl/registry.hpp>

typedef std::list< std::string > string_list;
typedef std::list< std::wstring > wstring_list;

namespace fs = boost::filesystem;
namespace cl = olib::opencorelib;

namespace olib { namespace openpluginlib {

namespace
{
#ifdef _WIN32
	std::string g_kernels_path("");
	std::string g_shaders_path("");
#endif

	class pl_logtarget : public cl::logtarget
	{
		public:
			pl_logtarget( )
			{
				 cl::log_utilities::get_formatted_stream( formatted_stream_, _CT(""), _CT("%H:%M:%S%F") );
			}

			virtual ~pl_logtarget( ) { }
	
			virtual void log( cl::invoke_assert& a, const TCHAR* log_source)
			{
				olib::t_stringstream ss;
				a.pretty_print_one_line( ss, cl::print::output_default );
				std::cerr << "invoke assert: " << cl::str_util::to_string( ss.str( ) ) << std::endl;
			}
			
			virtual void log( cl::base_exception& e, const TCHAR* log_source)
			{
				olib::t_stringstream ss;
				e.pretty_print_one_line( ss, cl::print::output_default );
				std::cerr << "base exception: " << cl::str_util::to_string( ss.str( ) ) << std::endl;
			}
	
			virtual void log( cl::logger& log_msg, const TCHAR* log_source)
			{
				olib::t_stringstream ss;
				log_msg.pretty_print_one_line( ss, cl::print::output_default );
				std::cerr << cl::str_util::to_string( cl::log_utilities::get_log_prefix_string( log_msg.level( ), formatted_stream_, cl::logoutput::output_default ) ) << " " << cl::str_util::to_string( ss.str( ) ) << std::endl;
			}
		private:
			t_stringstream formatted_stream_;
	};

	class add_to_filter_string : public std::unary_function<detail::registry::container::value_type, void>
	{
	public:
		explicit add_to_filter_string( std::wstring& str, bool in_filter )
			: str_( str )
			, in_filter_( in_filter )
		{ }
		
		void operator( )( const detail::registry::container::value_type& str )
		{
			std::wstring filter;
			if( in_filter_ )
				filter = str.second.in_filter;
			else
				filter = str.second.out_filter;
				
			if( str_.find( filter ) == std::wstring::npos )
				str_ += filter + std::wstring( L" " );
		}
		
		std::wstring filter( ) const
		{ return str_; }
		
	private:
		add_to_filter_string& operator=( const add_to_filter_string& );
	
	private:
		std::wstring& str_;
		bool in_filter_;
	};

	// OFX search paths.
	string_list get_ofx_plugin_path( )
	{
		string_list paths;
		
		// Follow the discovery algorithm as in the OFX spec:
		// search for paths in the OFX_PLUGIN_PATH
		// environment variable, look in Common Files (both
		// localised and non-localised locations).
		if( cl::str_util::env_var_exists( _CT("OFX_PLUGIN_PATH") ) )
		{
			t_string ofx_path_env = cl::str_util::get_env_var( _CT("OFX_PLUGIN_PATH") );
			typedef boost::tokenizer<boost::char_separator<TCHAR>, t_string::const_iterator, t_string> tokenizer;
#	ifdef WIN32
			boost::char_separator<TCHAR> sep( _CT(";") );
#	else
			boost::char_separator<TCHAR> sep( _CT(":") );
#	endif
			tokenizer tok( ofx_path_env.begin( ), ofx_path_env.end( ), sep );
			for( tokenizer::const_iterator I = tok.begin( ); I != tok.end( ); ++I )
			{
				const fs::path plugin_dir( *I );
				if( !fs::exists( plugin_dir ) || !fs::is_directory( plugin_dir ) ) continue;
				paths.push_back( cl::str_util::to_string( *I ) );
			}
		}
		
#	ifdef WIN32
		TCHAR buffer[ MAX_PATH ];
		SHGetFolderPath( NULL, CSIDL_PROGRAM_FILES_COMMON, NULL, SHGFP_TYPE_CURRENT, buffer );
		
		paths.push_back( cl::str_util::to_string( std::wstring( buffer ) ) + "\\OFX\\Plugins" );
		
		// Check non-localised path for backward compatibility reasons.
		if( wcscmp( buffer, L"C:\\Program Files\\Common Files" ) )
			paths.push_back( "C:\\Program Files\\Common Files\\OFX\\Plugins" );
#	elif defined __APPLE__
		paths.push_back( "/Library/OFX/Plugins" );
#	else
		paths.push_back( "/usr/OFX/Plugins" );
#	endif

		return paths;
	}

	void reflib( int init, const std::string& lookup_path = "" )
	{
		static long refs = 0;
		std::string profile_base = lookup_path;

		assert( refs >= 0 && L"openpluginlib::refinit: refs is negative." );

		detail::registry& el_reg = detail::registry::instance( );

		if( init > 0 && ++refs )
		{
			if( refs == 1 )
			{
				olib::t_string var( _CT("AML_PATH") );

				if ( cl::str_util::env_var_exists( var ) )
				{
					profile_base = cl::str_util::to_string( cl::str_util::get_env_var( var ) );
					el_reg.insert_std( cl::str_util::to_string( profile_base ) );
				}
#ifndef WIN32
				else
				{
//					el_reg.insert_std( OPENIMAGELIB_PLUGINS );
					el_reg.insert_std( OPENMEDIALIB_PLUGINS );
				}
#endif

				string_list ofx_paths = get_ofx_plugin_path( );
				std::for_each( ofx_paths.begin( ), ofx_paths.end( ), boost::bind( &detail::registry::insert_std, boost::ref( el_reg ), _1 ) );
			}

			if( !lookup_path.empty( ) )
				el_reg.insert_custom( lookup_path );

			// Ugh - must find a better way to handle path info (do we really need platform specific here at all?)
#ifdef WIN32
			profile_base += "/../profiles/";
#elif __APPLE__
			profile_base += "/../Resources/profiles/";
#else
			profile_base += "/../../../../share/aml/profiles/";
#endif

			// Register the derived directory for the profiles
			cl::profile_init( profile_base );
		}
		else if( init < 0 && --refs == 0 )
		{
			if( !lookup_path.empty( ) )
				el_reg.remove( );
			else
				el_reg.clear( );

			detail::unload_shared_library( );
		}
	}
	
	static boost::recursive_mutex mutex;
}

namespace detail
{
	namespace
	{
		class is_match : public std::unary_function<boost::wregex, bool>
		{
		public:
			explicit is_match( const std::wstring& to_match )
				: to_match_( to_match )
			{ }

			bool operator( )( const boost::wregex& expr )
			{
				if( boost::regex_match( to_match_.c_str( ), expr ) )
					return true;

				return false;
			}

		private:
			is_match& operator=( const is_match& );

		private:
			const std::wstring& to_match_;
		};

		bool if_matches_expression( const std::wstring& to_match, const std::vector<boost::wregex>& expression )
		{
			typedef std::vector<boost::wregex>::const_iterator const_iterator;

			const_iterator I = std::find_if( expression.begin( ), expression.end( ), is_match( to_match ) );

			return I != expression.end( );
		}

		void null_delete( void* )
		{ }
	}

	opl_ptr discover_query_impl::plugin_proxy::create_plugin( const std::string& options ) const
	{
		if( !item_.resolver.dlopened )
			load_shared_library( item_.resolver, item_.filenames );

		if( item_.resolver.dlopened )
		{
			if( item_.resolver.init( ) )
			{
				openplugin* plugin = 0;
				item_.resolver.create_plugin( options.c_str( ), &plugin );
	
				if( plugin ) return opl_ptr( plugin, item_.resolver.destroy_plugin );
			}
		}

		return opl_ptr( static_cast<openplugin*>( 0 ), null_delete );
	}

	discover_query_impl::container find_plugins( const std::wstring& libname, 
												 const std::wstring& type, 
												 const std::wstring& to_match, 
												 const detail::registry::container& el_reg_db )
	{
		typedef detail::registry::container						db_container;
		typedef db_container::const_iterator					db_const_iterator;
		typedef db_container::iterator							db_iterator;
		typedef std::pair<db_const_iterator, db_const_iterator>	query_iterator_pair;

		discover_query_impl::container plugins;

		query_iterator_pair pair;
		if( libname.empty( ) )
		{
			pair.first  = el_reg_db.begin( );
			pair.second = el_reg_db.end( );
		}
		else
		{
			pair = el_reg_db.equal_range( libname );
		}

		while( pair.first != pair.second )
		{
			const std::wstring& item_type							= ( *pair.first ).second.type;
			const std::vector<boost::wregex>& item_extension	= ( *pair.first ).second.extension;

			if( ( type.empty( ) || type == item_type ) && ( to_match.empty( ) || if_matches_expression( to_match, item_extension ) ) ) 
				plugins.push_back( ( *pair.first ).second );

			++pair.first;
		}

		return plugins;
	}
	
	bool discover_query_impl::operator( )( const std::wstring& libname, const std::wstring& type, const std::wstring& to_match )
	{
		// Custom db first
		container plugins = find_plugins( libname, type, to_match, detail::registry::instance( ).get_custom_db( ) );

		if ( plugins.empty() ) 
		{
			// try the std db
			plugins = find_plugins( libname, type, to_match, detail::registry::instance( ).get_std_db( ) );
		}

		if ( !plugins.empty() )
			std::copy( plugins.begin(), plugins.end(), std::inserter( plugins_, plugins_.end() ) );
		
		sort<highest_merit_sort>( );

		return true;
	}
}

void auto_load( )
{
	const detail::registry::list &l = detail::registry::instance( ).auto_load( );
	for ( detail::registry::list::const_iterator i = l.begin( ); i != l.end( ); ++i )
	{
		struct detail::plugin_resolver resolver;
		ARENFORCE_MSG( load_shared_library( resolver, i->filenames ), 
			"Failed to auto-load plugin in \"%1%\" (%2%)\nThe full path is \"%3%\"." )
			( fs::path(i->opl_path).filename().c_str() )( i->name )( i->opl_path );
		resolver.init( );
	}
	detail::registry::instance( ).auto_clear( );
}

bool init( const string_list& lookup_paths )
{
	boost::recursive_mutex::scoped_lock lock( mutex );

    for ( string_list::const_iterator I = lookup_paths.begin();
          I != lookup_paths.end(); ++I ) 
    {
        reflib( 1, *I );
    }

	auto_load( );

	return true;
}

bool init( const std::string& lookup_path )
{
	boost::recursive_mutex::scoped_lock lock( mutex );

	reflib( 1, lookup_path );

	auto_load( );

	return true;
}

bool uninit( )
{
	boost::recursive_mutex::scoped_lock lock( mutex );

	reflib( -1 );

	return true;
}

OPENPLUGINLIB_DECLSPEC void init_log( )
{
	cl::the_log_handler::instance( ).add_target( cl::logtarget_ptr( new pl_logtarget( ) ) );
}

#ifdef WIN32
// 2 alternative initialization functions required on windows so that when bundling the openlibraries with an app
// the kernel & shader paths passed in here are used rather than the default openlibraries registry settings
bool init( const string_list& lookup_paths, const std::string& kernels_path, const std::string& /*shaders_path*/ )
{
	boost::recursive_mutex::scoped_lock lock( mutex );

    for ( string_list::const_iterator I = lookup_paths.begin();
          I != lookup_paths.end(); ++I ) 
    {
        reflib( 1, *I );
    }
	g_kernels_path = kernels_path;
	return true;
}

bool init( const std::string& lookup_path, const std::string& /*kernels_path*/, const std::string& shaders_path )
{
	boost::recursive_mutex::scoped_lock lock( mutex );

	reflib( 1, lookup_path );
	g_shaders_path = shaders_path;
	return true;
}
#endif

std::wstring registered_filters( bool in_filter )
{
	typedef detail::registry::container					db;
	typedef detail::registry::container::const_iterator	const_iterator;
	
	std::wstring filter;
	add_to_filter_string f( filter, in_filter );

	const db& custom_db = detail::registry::instance( ).get_custom_db( );	
	std::for_each( custom_db.begin( ), custom_db.end( ), f );

	const db& std_db = detail::registry::instance( ).get_std_db( );	
	std::for_each( std_db.begin( ), std_db.end( ), f );

	return f.filter( );
}

#ifdef WIN32
namespace
{
	std::string get_registry_key( const std::wstring& key_str )
	{
		const int allocsiz = 512;

		TCHAR buf[ allocsiz ];
		DWORD bufsiz = allocsiz;

		std::string value;
		
		HKEY key;
		if( RegOpenKeyEx( HKEY_LOCAL_MACHINE, L"Software\\OpenLibraries", 0, KEY_READ, &key ) == ERROR_SUCCESS )
		{
			if( RegQueryValueEx( key, key_str.c_str( ), 0, NULL, ( LPBYTE ) buf, &bufsiz ) == ERROR_SUCCESS )
				value = cl::str_util::to_string( buf );

			RegCloseKey( key );
		}

		return value;
	}
}

std::string plugins_path( )
{
#ifndef NDEBUG
	return get_registry_key( L"PluginsDirDebug" );
#else
	return get_registry_key( L"PluginsDirRelease" );
#endif	
}

std::string kernels_path( )
{
	if(g_kernels_path != "")
		return g_kernels_path;
	else
		return get_registry_key( L"KernelsDir" );
}

std::string shaders_path( )
{
	if(g_shaders_path != "")
		return g_shaders_path;
	else
		return get_registry_key( L"ShadersDir" );
}

#endif
} }
