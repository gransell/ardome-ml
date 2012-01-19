
// openpluginlib - A plugin interface to openlibraries.

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#include <algorithm>
#include <functional>
#include <map>
#include <iostream>

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <dlfcn.h>
#endif // WIN32

#ifndef BOOST_FILESYSTEM_DYN_LINK 
    #define BOOST_FILESYSTEM_DYN_LINK 
#endif

#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>

#include <openpluginlib/pl/openplugin.hpp>
#include <openpluginlib/pl/utf8_utils.hpp>
#include <openpluginlib/pl/log.hpp>
#include <sstream>

namespace fs = boost::filesystem;

namespace olib { namespace openpluginlib { 

namespace detail {

namespace
{
#ifdef WIN32
	HMODULE dlopen_( const boost::filesystem::path& path )
#else
	void* dlopen_( const boost::filesystem::path& path )
#endif
	{
#ifdef WIN32
		return LoadLibrary( to_wstring( path.native_file_string( ).c_str( ) ).c_str( ) );
#else
		// CY: It is essential that RTLD_GLOBAL is used here for dynamic_cast to be
		// functional. All g++ apps should also link with -Wl,-E.
		return dlopen( path.native_file_string( ).c_str( ), RTLD_GLOBAL | RTLD_NOW );
#endif
	}
	
#ifdef WIN32
	void* dlsym_( HMODULE shared, const char* entry_point )
#else
	void* dlsym_( void* shared, const char* entry_point )
#endif
	{
#ifdef WIN32
		return GetProcAddress( shared, entry_point );
#else
		return dlsym( shared, entry_point );
#endif
	}

#ifdef WIN32
	bool dlclose_( HMODULE shared )
#else
	bool dlclose_( void* shared )
#endif
	{
#ifdef WIN32
		return FreeLibrary( shared ) == TRUE;
#else
		return dlclose( shared ) == 0;
#endif
	}
	
	bool resolve_plugin_symbols( plugin_resolver& resolver )
	{
		resolver.init			= ( openplugin_init )			dlsym_( resolver.dl_handle, "openplugin_init" );
		resolver.uninit			= ( openplugin_uninit )			dlsym_( resolver.dl_handle, "openplugin_uninit" );
		resolver.create_plugin	= ( openplugin_create_plugin )	dlsym_( resolver.dl_handle, "openplugin_create_plugin" );
		resolver.destroy_plugin = ( openplugin_destroy_plugin ) dlsym_( resolver.dl_handle, "openplugin_destroy_plugin" );

		if( !resolver.init || !resolver.uninit || !resolver.create_plugin || !resolver.destroy_plugin )
			return false;
			
		return true;
	}

	static std::map< fs::path, plugin_resolver > plugin_cache;
}

plugin_resolver::plugin_resolver( )
	: init( 0 )
	, uninit( 0 )
	, create_plugin( 0 )
	, destroy_plugin( 0 )
	, dl_handle( NULL )
	, dlopened( false )
{ }

plugin_resolver::~plugin_resolver( )
{
}

bool acquire_shared_symbols( plugin_resolver& resolver, const std::vector<wstring>& shared_name )
{
	typedef std::vector<wstring>::const_iterator const_iterator;
	fs::path key = fs::path( to_string( *shared_name.begin( ) ).c_str( ), fs::native );
	string error = "";
	
	if ( plugin_cache.find( key ) != plugin_cache.end( ) )
	{
		resolver = plugin_cache[ key ];
		return resolver.dlopened;
	}

	bool lib_file_was_found = false;
	for( const_iterator I = shared_name.begin( ); I != shared_name.end( ); ++I )
	{
		//Check if the file exists before trying to dlopen it
		fs::path lib_path(to_string( *I ).c_str( ), fs::native);
		if( !fs::exists(lib_path) )
		{
			continue;
		}
		else
		{
			lib_file_was_found = true;
		}
        
		resolver.dl_handle = dlopen_( lib_path );
		if( resolver.dl_handle )
		{
			if( !resolve_plugin_symbols( resolver ) ) continue;

			resolver.dlopened = true;
			break;
		}
		else
		{
#ifndef WIN32
			error += std::string( dlerror( ) ) + "\n";
#else
			error += to_string( *I ) + "\n";
#endif
		}
	}

	if ( resolver.dlopened )
		plugin_cache[ key ] = resolver;
	else
	{
		if ( lib_file_was_found )
		{
			PL_LOG( 0, boost::format( "Unable to open shared object: %1%" ) % error );
		}
		else
		{
			//If none of the library paths could be found, there won't be a dlopen/LoadLibrary
			//error for us to display.
			std::stringstream paths_tried;
			for( const_iterator I = shared_name.begin( ); I != shared_name.end( ); ++I )
			{
				paths_tried << to_string( *I ).c_str() << "\n";
			}

			PL_LOG( 0, boost::format( "Could not find the requested plugin library. Tried the following %1% paths:\n%2%" ) % shared_name.size() % paths_tried.str() );
		}
	}

	return resolver.dlopened;
}

void uninit_shared_symbols( )
{
	std::map<fs::path, plugin_resolver>::iterator i = plugin_cache.begin( );
	while( i != plugin_cache.end( ) )
	{
		fs::path key = i->first;
		i ++;
		plugin_cache.erase( key );
	}
}

} } }
