
#include "precompiled_headers.hpp"
#include "profile.hpp"
#include "profile_properties.hpp"

#include <opencorelib/cl/core.hpp>
#include <opencorelib/cl/enforce_defines.hpp>

#include <iostream>
#include <fstream>
#include <sstream>

#include <boost/filesystem.hpp>
#include <boost/foreach.hpp>

#include "opencorelib/cl/enforce_defines.hpp"

namespace fs = boost::filesystem;

namespace olib { namespace opencorelib {

class profile_impl : public profile
{
	public:
		profile_impl( )
		{
			ops_[ "=" ] = op_equals;
			ops_[ "|=" ] = op_or;
			ops_[ "&=" ] = op_and;
			ops_[ "~=" ] = op_off;
			ops_[ "^=" ] = op_xor;
		}

		virtual ~profile_impl( ) { }

		std::vector< std::string > split( const std::string &str, size_t max )
		{
			const std::string delimiter = " ";
			std::vector< std::string > result;
			std::string::size_type last = str.find_first_not_of( delimiter, 0 );
			std::string::size_type pos = str.find_first_of( delimiter, last );

			while ( result.size( ) < max - 1 && ( pos != std::string::npos || last != std::string::npos ) )
			{
				result.push_back( str.substr( last, pos -  last ) );
				last = str.find_first_not_of( delimiter, pos );
				pos = str.find_first_of( delimiter, last );
			}

			if ( pos != std::string::npos || last != std::string::npos )
			{
				result.push_back( str.substr( last, str.size( ) -  last ) );
			}

			return result;
		}

		bool parse( std::istream &file )
		{
			std::string line;

			while ( std::getline( file, line ) ) 
			{
				if ( line == "" )
					continue;
				
				std::vector< std::string > tokens = split( line, 3 );

				if ( tokens[ 0 ][ 0 ] == '#' )
					continue;
				else if ( tokens[ 0 ] == "include" )
					parse_include( tokens, line );
				else if ( tokens[ 0 ] != "" )
					parse_entry( tokens, line );
			}

			return true;
		}

		virtual std::vector< profile_entry >::iterator begin( )
		{
			return entries_.begin( );
		}

		virtual std::vector< profile_entry >::iterator end( )
		{
			return entries_.end( );
		}

		virtual std::vector< profile_entry >::const_iterator begin( ) const
		{
			return entries_.begin( );
		}

		virtual std::vector< profile_entry >::const_iterator end( ) const
		{
			return entries_.end( );
		}

		virtual list::iterator find( const std::string& key )
		{
			for( list::iterator it = entries_.begin( ); it != entries_.end( ); ++it )
			{
				if( it->name == key ) return it;
			}

			return entries_.end( );
		}

		virtual list::const_iterator find( const std::string& key ) const
		{
			for( list::const_iterator it = entries_.begin( ); it != entries_.end( ); ++it )
			{
				if( it->name == key ) return it;
			}
			
			return entries_.end( );
		}

	protected:
		void parse_entry( const std::vector< std::string > &tokens, const std::string &line )
		{
			if ( tokens.size( ) == 3 && ops_.find( tokens[ 1 ] ) != ops_.end( ) )
			{
				struct profile_entry entry;
				entry.name = tokens[ 0 ];
				entry.op = ops_[ tokens[ 1 ] ];
				entry.value = tokens[ 2 ];
				entries_.push_back( entry );
			}
			else
			{
				throw std::runtime_error( "Syntax error " + line );
			}
		}

		void parse_include( const std::vector< std::string > &tokens, const std::string &line )
		{
			if ( tokens.size( ) == 2 )
			{
				profile_ptr profile = profile_load( tokens[ 1 ] );
				for( profile::list::const_iterator i = profile->begin( ); i != profile->end( ); i ++ )
					entries_.push_back( *i );
			}
			else
			{
				throw std::runtime_error( "Syntax error " + line );
			}
		}

		std::map< std::string, profile_op > ops_;
		std::vector< profile_entry > entries_;
};

static std::string base_directory;

void profile_init( const std::string &directory )
{
	base_directory = directory;
}

profile_ptr profile_load( const std::string &profile )
{
	typedef boost::shared_ptr< profile_impl > profile_impl_ptr;
	profile_impl_ptr result( new profile_impl( ) );
	if ( fs::exists( profile ) )
	{
		std::ifstream stream( profile.c_str( ), std::ifstream::in );
		result->parse( stream );
	}
	else if ( fs::exists( base_directory + profile ) )
	{
		std::ifstream stream( ( base_directory + profile ).c_str( ), std::ifstream::in );
		result->parse( stream );
	}
	else
	{
		ARENFORCE_MSG( false, "Could not find the profile %1% (also tried %2%)" )( profile )( base_directory + profile );
	}
	return result;
}

// Wrapper implementation
profile_wrapper::profile_wrapper( bool managed )
	: managed_( managed )
{ }

profile_wrapper::~profile_wrapper( ) 
{
	for ( map::iterator it = begin( ); it != end( ); it ++ )
		if ( ( *it ).second->managed_property( ) )
			delete ( *it ).second;
	map_.clear( );
}

void profile_wrapper::enroll( const std::string &name, int &ref )
{ 
	map_[ name ] = new profile_int( ref ); 
}

void profile_wrapper::enroll( const std::string &name, unsigned int &ref )
{ 
	map_[ name ] = new profile_uint( ref ); 
}

void profile_wrapper::enroll( const std::string &name, boost::int64_t &ref )
{ 
	map_[ name ] = new profile_int64( ref ); 
}

void profile_wrapper::enroll( const std::string &name, float &ref )
{ 
	map_[ name ] = new profile_float( ref ); 
}

void profile_wrapper::enroll( const std::string &name, double &ref )
{ 
	map_[ name ] = new profile_double( ref ); 
}

void profile_wrapper::enroll( const std::string &name, profile_property *property )
{
	map_[ name ] = property;
}

profile_wrapper::map::iterator profile_wrapper::begin( ) 
{ 
	return map_.begin( ); 
}

profile_wrapper::map::iterator profile_wrapper::end( )
{ 
	return map_.end( ); 
}

profile_wrapper::map::const_iterator profile_wrapper::begin( ) const
{ 
	return map_.begin( ); 
}

profile_wrapper::map::const_iterator profile_wrapper::end( ) const
{ 
	return map_.end( ); 
}

profile_wrapper::map::iterator profile_wrapper::find( const std::string &name )
{ 
	return map_.find( name ); 
}

// Distributor implementation
profile_manager::profile_manager( )
{
}

profile_manager::~profile_manager( )
{
	for ( targets::iterator it = targets_.begin( ); it != targets_.end( ); it ++ )
		if ( ( *it )->managed_wrapper( ) )
			delete ( *it );
	targets_.clear( );
}

void profile_manager::enroll( profile_wrapper *wrapper )
{ 
	targets_.push_back( wrapper ); 
}

void profile_manager::load( const std::string &id )
{
	profile_ptr profile = profile_load( id );
	for( profile::list::const_iterator i = profile->begin( ); i != profile->end( ); i ++ )
	{
		bool found = false;
		for( size_t j = 0; j != targets_.size( ); j ++ )
		{
			if ( targets_[ j ]->find( ( *i ).name ) != targets_[ j ]->end( ) )
			{
				targets_[ j ]->find( ( *i ).name )->second->assign( *i );
				found = true;
			}
		}
		ARENFORCE_MSG( found, "Failed to find a place to store profile entry. Make sure you have enrolled this value" )( ( *i ).name );
	}
}

} }


