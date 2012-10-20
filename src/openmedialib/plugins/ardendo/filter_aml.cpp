// AML filter
//
// Copyright (C) 2008 Ardendo
// Released under the LGPL.
//
// #filter:aml
//
// This filter generates aml files on request. AML files are a serialisation of
// the graph they're connected to - they can be reloaded as valid video clips for
// reuse in other projects via input:aml_stack.
//
// Example:
//
// <graph>
// filter:aml filename=-
//
// Writes a serialisation of the graph to stdout.

#include "precompiled_headers.hpp"
#include "amf_filter_plugin.hpp"
#include "utility.hpp"

#include <iostream>
#include <sstream>
#include <fstream>
#include <map>

#include <boost/cstdint.hpp>

namespace aml { namespace openmedialib {

static bool key_sort( const pcos::key &k1, const pcos::key &k2 )
{
	return strcmp( k1.as_string( ), k2.as_string( ) ) < 0;
}

class ML_PLUGIN_DECLSPEC filter_aml : public ml::filter_type
{
	public:
		// Filter_type overloads
		explicit filter_aml( const std::wstring & )
			: ml::filter_type( )
			, prop_filename_( pl::pcos::key::from_string( "filename" ) )
			, prop_write_( pl::pcos::key::from_string( "write" ) )
			, prop_stdout_( pl::pcos::key::from_string( "stdout" ) )
			, prop_limit_( pl::pcos::key::from_string( "limit" ) )
		{
			properties( ).append( prop_filename_ = std::wstring( L"graph.aml" ) );
			properties( ).append( prop_write_ = 1 );
			properties( ).append( prop_stdout_ = std::string( "" ) );
			properties( ).append( prop_limit_ = 0 );
		}

		// Indicates if the input will enforce a packet decode
		virtual bool requires_image( ) const { return false; }

		// This provides the name of the plugin (used in serialisation)
		virtual const std::wstring get_uri( ) const { return L"aml"; }

		virtual void on_slot_change( ml::input_type_ptr input, int )
		{
			if ( input )
			{
				if ( prop_write_.value< int >( ) )
				{
					create_aml( input, prop_filename_.value< std::wstring >( ), prop_limit_.value< int >( ) );
					prop_write_ = 0;
				}
			}
		}

	protected:
		// The main access point to the filter
		void do_fetch( ml::frame_type_ptr &result )
		{
			if ( prop_write_.value< int >( ) )
			{
				create_aml( fetch_slot( ), prop_filename_.value< std::wstring >( ), prop_limit_.value< int >( ) );
				prop_write_ = 0;
			}
			result = fetch_from_slot( );
		}

	private:
		void create_aml( ml::input_type_ptr input, const std::wstring filename, int limit )
		{
			if ( filename == L"@" )
			{
				std::ostringstream stream;
				create_aml( stream, input, limit );
				prop_stdout_ = stream.str( );
			}
			else if ( filename != L"-" )
			{
				std::fstream stream( olib::opencorelib::str_util::to_string( filename ).c_str( ), std::ios::out );
				create_aml( stream, input, limit );
			}
			else
			{
				create_aml( std::cout, input, limit );
			}
		}

		void to_stream( std::ostream &stream, const std::string &name, std::vector< double > l )
		{
			stream << name << "=";
			if ( l.begin( ) != l.end( ) )
			{
				for ( std::vector< double >::iterator iter = l.begin( ); iter != l.end( ); ++iter )
					stream << *iter << " ";
			}
			else
			{
				stream << " ";
			}
		}

		void to_stream( std::ostream &stream, const std::string &name, std::vector< int > l )
		{
			stream << name << "=";
			if ( l.begin( ) != l.end( ) )
			{
				for ( std::vector< int >::iterator iter = l.begin( ); iter != l.end( ); ++iter )
					stream << *iter << " ";
			}
			else
			{
				stream << " ";
			}
		}

		bool needs_quotes( const std::wstring &value )
		{
			bool result = true;
			if ( value.size( ) > 1 )
			{
				if ( ( value[ 0 ] == L'\"' || value[ 0 ] == L'\'' ) && value[ value.size( ) - 1 ] == value[ 0 ] )
					result = false;
			}
			return result;
		}

		void create_aml( std::ostream &stream, ml::input_type_ptr input, int limit )
		{
			if ( input )
			{
				const pl::pcos::property_container &props = input->properties( );
				pl::pcos::key_vector keys = props.get_keys( );

				if ( !props.get_property_with_string( "serialise_terminator" ).valid( ) )
				{
					if ( -- limit != 0 )
						for( size_t i = 0; i < input->slot_count( ); i ++ )
							create_aml( stream, input->fetch_slot( i ), limit );
				}
				else
				{
					if ( props.get_property_with_string( "serialise" ).valid( ) )
					{
                    	props.get_property_with_string( "serialise" ) = boost::int64_t( &stream );
                    	props.get_property_with_string( "serialise" ) = boost::int64_t( 0 );
						return;
					}
				}

				std::string uri = olib::opencorelib::str_util::to_string( input->get_uri( ) );

				if ( input->slot_count( ) == 0 && input->get_mime_type( ) == L"text/value" )
					stream << uri;
				else if ( input->slot_count( ) == 0 && uri.find( " " ) != std::string::npos )
					stream << '\"' << uri << "\"";
				else if ( input->slot_count( ) == 0 || uri == "aml_stack:" )
					stream << uri;
				else
					stream << "filter:" << uri;

				if ( props.get_property_with_string( "serialise" ).valid( ) )
				{
					stream << std::endl;
                    props.get_property_with_string( "serialise" ) = boost::int64_t( &stream );
                    props.get_property_with_string( "serialise" ) = boost::int64_t( 0 );
				}
				else
				{
					std::sort( keys.begin( ), keys.end( ), key_sort );

					if ( keys.begin( ) != keys.end( ) )
						stream << " ";

					for( pl::pcos::key_vector::iterator it = keys.begin( ); it != keys.end( ); ++it )
					{
						std::string name( ( *it ).as_string( ) );
						pl::pcos::property p = props.get_property_with_string( name.c_str( ) );
						if ( name.find( ".aml_" ) == 0 )
							continue;
						else if ( p.is_a< double >( ) )
							stream << name << "=" << p.value< double >( ) << " ";
						else if ( p.is_a< int >( ) )
							stream << name << "=" << p.value< int >( ) << " ";
						else if ( p.is_a< boost::int64_t >( ) )
							stream << name << "=" << p.value< boost::int64_t >( ) << " ";
						else if ( p.is_a< boost::uint64_t >( ) )
							stream << name << "=" << p.value< boost::uint64_t >( ) << " ";
						else if ( p.is_a< std::wstring >( ) && needs_quotes( p.value< std::wstring >( ) ) )
							stream << name << "=\"" << olib::opencorelib::str_util::to_string( p.value< std::wstring >( ) ) << "\" ";
						else if ( p.is_a< std::wstring >( ) )
							stream << name << "=" << olib::opencorelib::str_util::to_string( p.value< std::wstring >( ) ) << " ";
						else if ( p.is_a< std::vector< double > >( ) )
							to_stream( stream, name, p.value< std::vector< double > >( ) );
						else if ( p.is_a< std::vector< int > >( ) )
							to_stream( stream, name, p.value< std::vector< int > >( ) );
						else
							stream << name << "=<unknown> ";
					}

					stream << std::endl;
				}
			}
			else
			{
				stream << "null:" << std::endl;
			}
		}

		pl::pcos::property prop_filename_;
		pl::pcos::property prop_write_;
		pl::pcos::property prop_stdout_;
		pl::pcos::property prop_limit_;
};

ml::filter_type_ptr ML_PLUGIN_DECLSPEC create_aml( const std::wstring &resource )
{
	return ml::filter_type_ptr( new filter_aml( resource ) );
}

} }
