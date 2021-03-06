// decode - A decode plugin to ml.
//
// Copyright (C) 2010 Vizrt
// Released under the LGPL.

#include "filter_field_order.hpp"

namespace pl = olib::openpluginlib;
namespace ml = olib::openmedialib::ml;
namespace pcos = olib::openpluginlib::pcos;
namespace cl = olib::opencorelib;

namespace
{
	template< typename T >
	ml::image_type_ptr merge( ml::image_type_ptr image1, int scan1, ml::image_type_ptr image2, int scan2 )
	{
		// Create a new image which has the same dimensions
		// NOTE: we assume that image dimensions are consistent (should we?)
		ml::image_type_ptr result = ml::image::allocate( image1 );

		boost::shared_ptr< T > result_type = ml::image::coerce< T >( result );
		boost::shared_ptr< T > image1_type = ml::image::coerce< T >( image1 );
		boost::shared_ptr< T > image2_type = ml::image::coerce< T >( image2 );
		for ( int i = 0; i < result->plane_count( ); i ++ )
		{
			typename T::data_type *dst_row = result_type->data( i );
			typename T::data_type *src1_row = image1_type->data( i ) + image1->pitch( i ) * scan1;
			typename T::data_type *src2_row = image2_type->data( i ) + image2->pitch( i ) * scan2;
			int dst_linesize = result->linesize( i );
			int dst_stride = result->pitch( i );
			int src_stride = image1->pitch( i ) * 2;
			int height = result->height( i ) / 2;

			while( height -- )
			{
				memcpy( dst_row, src1_row, dst_linesize );
				dst_row += dst_stride / sizeof( typename T::data_type );
				src1_row += src_stride / sizeof( typename T::data_type );

				memcpy( dst_row, src2_row, dst_linesize );
				dst_row += dst_stride / sizeof( typename T::data_type );
				src2_row += src_stride / sizeof( typename T::data_type );
			}
		}

		return result;
	}

	ml::image_type_ptr merge( ml::image_type_ptr image1, int scan1, ml::image_type_ptr image2, int scan2 )
	{  
		ml::image_type_ptr result;
		if ( ml::image::coerce< ml::image::image_type_8 >( image1 ) )
			result = merge< ml::image::image_type_8 >( image1, scan1, image2, scan2 );
		else if ( ml::image::coerce< ml::image::image_type_16 >( image1 ) )
			result = merge< ml::image::image_type_16 >( image1, scan1, image2, scan2 );
		return result;
	}
}

namespace olib { namespace openmedialib { namespace ml { namespace decode {

filter_field_order::filter_field_order( )
	: filter_simple( )
	, prop_order_( pl::pcos::key::from_string( "order" ) )
{
	properties( ).append( prop_order_ = 2 );
}

 filter_field_order::~filter_field_order( )
{
}

// Indicates if the input will enforce a packet decode
bool filter_field_order::requires_image( ) const { return true; }

const std::wstring filter_field_order::get_uri( ) const { return std::wstring( L"field_order" ); }

const size_t filter_field_order::slot_count( ) const { return 1; }

void filter_field_order::do_fetch( frame_type_ptr &frame )
{
	frame = fetch_from_slot( );

	if ( prop_order_.value< int >( ) && frame && frame->has_image( ) )
	{
		// Fetch image
		ml::image_type_ptr image = frame->get_image( );

		if ( image->field_order( ) != ml::image::progressive && image->field_order( ) != ml::image::field_order_flags( prop_order_.value< int >( ) ) )
		{
			// Do we have a previous frame and is it the one preceding this request?
			if ( !previous_ || previous_->position( ) != get_position( ) - 1 )
			{
				if ( get_position( ) > 0 )
					previous_ = fetch_slot( 0 )->fetch( get_position( ) - 1 )->get_image( );
				else
					previous_ = ml::image_type_ptr( );
			}

			// If we have a previous frame, merge the fields accordingly, otherwise duplicate the first field
			if ( previous_ )
			{
				// Image is definitely in the wrong order - think this is correct for both?
				ml::image_type_ptr merged;
				if ( prop_order_.value< int >( ) == 2 )
					merged = merge( image, 0, previous_, 1 );
				else
					merged = merge( previous_, 0, image, 1 );
				merged->set_field_order( ml::image::field_order_flags( prop_order_.value< int >( ) ) );
				merged->set_position( get_position( ) );
				frame->set_image( merged );
			}
			else
			{
				ml::image_type_ptr merged = merge( image, 0, image, 0 );
				merged->set_field_order( ml::image::field_order_flags( prop_order_.value< int >( ) ) );
				merged->set_position( get_position( ) );
				frame->set_image( merged );
			}

			// We need to remember this frame for reuse on the next request
			previous_ = image;
			previous_->set_position( get_position( ) );
		}
	}
}


} } } }


