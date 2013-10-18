// Copyright (C) 2010 Vizrt
// Released under the LGPL.

#include "filter_analyse.hpp"

namespace pl = olib::openpluginlib;
namespace ml = olib::openmedialib::ml;
namespace il = olib::openmedialib::ml;
namespace pcos = olib::openpluginlib::pcos;
namespace cl = olib::opencorelib;

namespace olib { namespace openmedialib { namespace ml { namespace decode {

filter_analyse::filter_analyse( )
	: ml::filter_simple( )
{
}
    
filter_analyse::~filter_analyse( )
{ }
    
bool filter_analyse::requires_image( ) const 
{ 
	return false; 
}
    
const std::wstring filter_analyse::get_uri( ) const
{ 
	return L"analyse";
}
    
void filter_analyse::do_fetch( ml::frame_type_ptr &result )
{
	if ( gop_.find( get_position( ) ) == gop_.end( ) )
	{
		// Remove previously cached gop
		gop_.erase( gop_.begin( ), gop_.end( ) );

		// Fetch the current frame
		ml::frame_type_ptr temp;

		if ( inner_fetch( temp, get_position( ), false ) && temp->get_stream( ) )
		{
			std::map< int, ml::stream_type_ptr > streams;
			int start = temp->get_stream( )->key( );
			int index = start;

			// Read the current gop
			while( index < get_frames( ) )
			{
				inner_fetch( temp, index ++ );
				if ( !temp || !temp->get_stream( ) || start != temp->get_stream( )->key( ) ) break;
				int sorted = start + temp->get_stream( )->properties( ).get_property_with_key( ml::analyse_type::key_temporal_reference_ ).value< int >( );
				pl::pcos::property temporal_offset( ml::analyse_type::key_temporal_offset_ );
				temp->get_stream( )->properties( ).append( temporal_offset = boost::int64_t( sorted ) );
				gop_[ temp->get_position( ) ] = temp;
				streams[ sorted ] = temp->get_stream( );
			}

			// Ensure that the recipient has access to the temporally sorted frames too
			for ( std::map< int, ml::frame_type_ptr >::iterator iter = gop_.begin( ); iter != gop_.end( ); ++iter )
			{
				pl::pcos::property temporal_stream( ml::analyse_type::key_temporal_stream_ );
				if ( streams.find( ( *iter ).first ) != streams.end( ) )
					( *iter ).second->properties( ).append( temporal_stream = streams[ ( *iter ).first ] );
			}

			result = gop_[ get_position( ) ];
		}
		else
		{
			result = temp;
		}
	}
	else
	{
		result = gop_[ get_position( ) ];
	}
}
    
bool filter_analyse::inner_fetch( ml::frame_type_ptr &result, int position, bool parse )
{
	fetch_slot( 0 )->seek( position );
	result = fetch_slot( 0 )->fetch( );
	if( !result ) return false;

	if ( !parse ) return true;
	
	ml::stream_type_ptr stream = result->get_stream( );
	
	// If the frame has no stream then there is nothing to analyse.
	if( !stream ) return false;

	if ( !analyse_ )
		analyse_ = ml::analyse_factory( stream );
	
	return analyse_ ? analyse_->analyse( stream ) : false;
}


} } } }

