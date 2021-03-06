
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
#include <openpluginlib/pl/opl_importer.hpp>
#include <opencorelib/cl/core.hpp>
#include <opencorelib/cl/str_util.hpp>

namespace fs = boost::filesystem;
namespace cl = olib::opencorelib;

namespace olib { namespace openpluginlib { namespace detail {

namespace
{
	// Insert all the opl plugins found in lookup_path into the specified db
	bool insert( const std::string& lookup_path, detail::registry::container& db, detail::registry::list &auto_load )
	{
		namespace fs = boost::filesystem;

		typedef detail::registry::container db_container;

		if( lookup_path.empty( ) )
			return false;

		boost::regex opl_ex( ".*\\.opl", boost::regex::extended | boost::regex::icase );

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
			fs::path plugin_dir( cl::str_util::to_t_string( *I ) );
			if( !fs::exists( plugin_dir ) || !fs::is_directory( plugin_dir ) ) continue;

			fs::directory_iterator end_iter;
			for( fs::directory_iterator dir_iter( plugin_dir ); dir_iter != end_iter; ++dir_iter )
			{
				// OLIBs native plugins.
				if( boost::regex_match( dir_iter->path( ).string( ), opl_ex ) )
				{
					opl_importer importer;
					importer( dir_iter->path( ) );
				
					std::copy( importer.plugins.begin( ), importer.plugins.end( ), std::inserter( db, db.begin( ) ) );

					if ( importer.auto_load )
						auto_load.push_back( importer.plugins.begin( )->second );
				}
			}
		}

		return true;
	}

} // namespace

registry*	registry::instance_		 = 0;
bool		registry::destroyed_	 = false;
bool		registry::was_destroyed_ = false;

registry::registry( const std::string& /*lookup_path*/ )
{
}

registry::~registry( )
{
}

bool registry::insert_std( const std::string& lookup_path )
{
	return insert( lookup_path, std_db_, auto_load_ );
}

bool registry::insert_custom( const std::string& lookup_path )
{
	return insert( lookup_path, custom_db_, auto_load_ );
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
