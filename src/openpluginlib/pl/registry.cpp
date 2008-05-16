
// openpluginlib - A plugin interface to openlibraries.

// Copyright (C) 2005-2007 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#ifdef HAVE_CONFIG_H
#include <openlibraries_global_config.hpp>
#endif

#ifdef __APPLE__
#include <CoreFoundation/CoreFoundation.h>
#endif
#ifndef BOOST_FILESYSTEM_DYN_LINK 
    #define BOOST_FILESYSTEM_DYN_LINK 
#endif
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#ifndef BOOST_REGEX_DYN_LINK 
    #define BOOST_REGEX_DYN_LINK 
#endif
#include <boost/regex.hpp>
#include <boost/tokenizer.hpp>

#ifdef HAVE_OFX
#include <OfxCore.h>
#endif

#include <openpluginlib/pl/registry.hpp>
#include <openpluginlib/pl/utf8_utils.hpp>
#include <openpluginlib/pl/opl_importer.hpp>

namespace fs = boost::filesystem;

namespace olib { namespace openpluginlib { namespace detail {

namespace
{
	// Code replication due to OFX. There are some
	// differences in how OFX plugins are specified.
	// Refactoring is left as an exercise to the reader.
#ifdef WIN32
	typedef HMODULE module_t;
	HMODULE dlopen_( const char* path )
#elif defined __APPLE__
	typedef CFBundleRef module_t;
	CFBundleRef dlopen_( const char* path )
#else
	typedef void* module_t;
	void* dlopen_( const char* path )
#endif
	{
#ifdef WIN32
		return LoadLibrary( to_wstring( path ).c_str( ) );
#elif defined __APPLE__
		CFStringRef bundle_str = CFStringCreateWithCString( kCFAllocatorDefault, path, kCFStringEncodingASCII );
		CFURLRef url_ref = CFURLCreateWithFileSystemPath( kCFAllocatorDefault, bundle_str, kCFURLPOSIXPathStyle, true );
		
		CFBundleRef bundle = CFBundleCreate( kCFAllocatorDefault, url_ref );
		
		CFRelease( url_ref );
		CFRelease( bundle_str );
		
		return bundle;
#else
		return dlopen( path, RTLD_GLOBAL | RTLD_NOW );
#endif
	}
	
	void* dlsym_( module_t shared, const char* entry_point )
	{
#ifdef WIN32
		return GetProcAddress( shared, entry_point );
#elif defined __APPLE__
		CFStringRef entry_str = CFStringCreateWithCString( kCFAllocatorDefault, entry_point, kCFStringEncodingASCII );

		void* entry = CFBundleGetFunctionPointerForName( shared, entry_str );
		CFRelease( entry_str );

		return entry;
#else
		return dlsym( shared, entry_point );
#endif
	}

#ifdef WIN32
	// Replicate some of OFX types on Win32 to avoid an include dependency.
	// In the unlikely event that they will change then this will have to be
	// updated.
	typedef struct	OfxPropertySetStruct* OfxPropertySetHandle;
	typedef int		OfxStatus;
	typedef			OfxStatus ( OfxPluginEntryPoint )( const char* action, const void* handle, OfxPropertySetHandle inArgs, OfxPropertySetHandle outArgs );

	typedef struct OfxHost
	{
		OfxPropertySetHandle host;
		void *( *fetchSuite )( OfxPropertySetHandle host, const char* suiteName, int suiteVersion );
	} OfxHost;

	typedef struct OfxPlugin
	{
		const char*		pluginApi;
		int				apiVersion;
		const char*		pluginIdentifier; 
		unsigned int 	pluginVersionMajor;
		unsigned int	pluginVersionMinor;
		void ( *setHost )( OfxHost* host );
		OfxPluginEntryPoint *mainEntry;
	} OfxPlugin;
#endif

#if defined( HAVE_OFX ) || defined( WIN32 )
	typedef OfxPlugin*	( *OfxGetPluginFunc )( int );
	typedef int			( *OfxGetNumPluginsFunc )( );
#endif

	// Insert all the opl/ofx plugins found in lookup_path into the specified db
	bool insert( const string& lookup_path, detail::registry::container& db )
	{
		namespace fs = boost::filesystem;

		typedef detail::registry::container db_container;

		if( lookup_path.empty( ) )
			return false;

		boost::regex opl_ex( ".*\\.opl", boost::regex::extended | boost::regex::icase );
		boost::regex ofx_ex( ".*\\.ofx\\.bundle", boost::regex::extended | boost::regex::icase );

		typedef boost::tokenizer<boost::char_separator<char> > tokenizer;

#ifdef WIN32
		boost::char_separator<char> sep( ";" );
#else
		boost::char_separator<char> sep( ":" );
#endif

		std::string lp( lookup_path.c_str( ) );

		tokenizer tok( lp.begin( ), lp.end( ), sep );
		for( tokenizer::const_iterator I = tok.begin( ); I != tok.end( ); ++I )
		{
			if( !fs::exists( fs::path( *I, fs::native ) ) || !fs::is_directory( fs::path( *I, fs::native ) ) ) continue;

			fs::directory_iterator end_iter;
			for( fs::directory_iterator dir_iter( fs::path( *I, fs::native ) ); dir_iter != end_iter; ++dir_iter )
			{
				// OLIBs native plugins.
				if( boost::regex_match( ( *dir_iter ).string( ), opl_ex ) )
				{
					opl_importer importer;
					importer( fs::path( ( *dir_iter ).string( ), fs::native ) );
				
					std::copy( importer.plugins.begin( ), importer.plugins.end( ), std::inserter( db, db.begin( ) ) );
				}

#		if defined( HAVE_OFX ) || defined( WIN32 )
				// OPL has support for a 3rd party API (OFX). In order to maintain
				// consistency and uniformity within the framework when dealing with
				// plugins, the discovery mechanism has been extended to incorporate
				// discovery of OFX plugins.
				// Subdirectories are not being checked - a must for OFX.
				if( ( *dir_iter ).string( )[ 0 ] != '@' && boost::regex_match( ( *dir_iter ).string( ), ofx_ex ) )
				{
					std::string plugin_shared = ( *dir_iter ).string( );
#		ifdef WIN32
					plugin_shared += "/Contents/Win32/" + 
						std::string( plugin_shared.begin( ) + plugin_shared.find_last_of( '/' ), 
									 plugin_shared.begin( ) + plugin_shared.rfind( ".bundle" ) );
#		elif defined __APPLE__
#		else
					plugin_shared += "/Contents/Linux-x86/" + 
						std::string( plugin_shared.begin( ) + plugin_shared.find_last_of( '/' ), 
									 plugin_shared.begin( ) + plugin_shared.rfind( ".bundle" ) );
#		endif
					module_t module = dlopen_( plugin_shared.c_str( ) );
					if( module )
					{
						OfxGetNumPluginsFunc num_plugins = ( OfxGetNumPluginsFunc ) dlsym_( module, "OfxGetNumberOfPlugins" );
						OfxGetPluginFunc get_plugin = ( OfxGetPluginFunc ) dlsym_( module, "OfxGetPlugin" );

						// OFX plugin instantiation is a little bit more complicated than shown here.
						// What's below is a simplification. Please check the spec for full details.
						if( num_plugins && get_plugin )
						{
							int n = num_plugins( );
							for( int i = 0; i < n; ++i )
							{
								OfxPlugin* plugin = get_plugin( i );
							
								// For each plugin build the opl item struct.
								plugin_item item;
							
								item.name = L"ofx_" + to_wstring( plugin->pluginIdentifier );
								item.extension.push_back( item.name );
								item.type = L"ofx";
								item.category = to_wstring( plugin->pluginApi );
								item.context = plugin;
							
								db.insert( db_container::value_type( L"openmedialib", item ) );
							}
						}
					}
				}
#		endif
			}
		}

		return true;
	}

} // namespace

registry*	registry::instance_		 = 0;
bool		registry::destroyed_	 = false;
bool		registry::was_destroyed_ = false;

registry::registry( const string& /*lookup_path*/ )
{
}

registry::~registry( )
{
}

bool registry::insert_std( const string& lookup_path )
{
	return insert( lookup_path, std_db_ );
}

bool registry::insert_custom( const string& lookup_path )
{
	return insert( lookup_path, custom_db_ );
}

bool registry::remove( )
{
	return false;
}

void registry::clear( )
{
	std_db_.clear( );	
	custom_db_.clear( );
}

} } }
