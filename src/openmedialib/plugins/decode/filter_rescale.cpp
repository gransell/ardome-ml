// decode - A decode plugin to ml.
//
// Copyright (C) 2010 Vizrt
// Released under the LGPL.

#include "filter_rescale.hpp"

namespace olib { namespace openmedialib { namespace ml { namespace decode {

filter_rescale::filter_rescale( )
	: filter_simple( )
	, prop_enable_( pl::pcos::key::from_string( "enable" ) )
	, prop_progressive_( pl::pcos::key::from_string( "progressive" ) )
	, prop_interp_( pl::pcos::key::from_string( "interp" ) )
	, prop_width_( pl::pcos::key::from_string( "width" ) )
	, prop_height_( pl::pcos::key::from_string( "height" ) )
	, prop_sar_num_( pl::pcos::key::from_string( "sar_num" ) )
	, prop_sar_den_( pl::pcos::key::from_string( "sar_den" ) )
{
	properties( ).append( prop_enable_ = 1 );
	properties( ).append( prop_progressive_ = 1 );
	properties( ).append( prop_interp_ = 0x10 ); //point sampling
	properties( ).append( prop_width_ = 640 );
	properties( ).append( prop_height_ = 360 );
	properties( ).append( prop_sar_num_ = 1 );
	properties( ).append( prop_sar_den_ = 1 );
}

filter_rescale::~filter_rescale( )
{
}

// Indicates if the input will enforce a packet decode
bool filter_rescale::requires_image( ) const { return false; }

const std::wstring filter_rescale::get_uri( ) const { return std::wstring( L"rescale" ); }

const size_t filter_rescale::slot_count( ) const { return 1; }

void filter_rescale::do_fetch( frame_type_ptr &frame )
{
	if ( last_frame_ && last_frame_->get_position( ) == get_position( ) )
	{
		frame = last_frame_->shallow( );
	}
	else
	{
		frame = fetch_from_slot( );
		if ( prop_enable_.value< int >( ) && frame && frame->has_image( ) )
		{
			ml::image_type_ptr image = frame->get_image( );
			if ( prop_progressive_.value< int >( ) == 1 )
				image = ml::image::deinterlace( image );
			else if ( prop_progressive_.value< int >( ) == -1 )
				image->set_field_order( ml::image::progressive );
			image = ml::image::rescale( image, prop_width_.value< int >( ), prop_height_.value< int >( ), ml::image::rescale_filter( prop_interp_.value< int >( ) ) );
			frame->set_image( image );
			frame->set_sar( prop_sar_num_.value< int >( ), prop_sar_den_.value< int >( ) );
		}
		last_frame_ = frame->shallow( );
	}
}


} } } }


