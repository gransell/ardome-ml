// Invert filter
//
// Copyright (C) 2007 Ardendo
//
// This filter creates an inverted yuv planar image - alpha is currently left 
// alone, but it can localise the region of the invert via the relative
// crop geometry offered.
//
// TODO: Sanitise geometry prior to cropping
#include "precompiled_headers.hpp"
#include "amf_filter_plugin.hpp"

namespace aml { namespace openmedialib {

class ML_PLUGIN_DECLSPEC filter_invert : public ml::filter_type
{
	public:
		// Filter_type overloads
		explicit filter_invert( const pl::wstring & )
			: ml::filter_type( )
			, prop_enable_( pcos::key::from_string( "enable" ) )
			, prop_planes_( pcos::key::from_string( "planes" ) )
			, prop_rx_( pcos::key::from_string( "rx" ) )
			, prop_ry_( pcos::key::from_string( "ry" ) )
			, prop_rw_( pcos::key::from_string( "rw" ) )
			, prop_rh_( pcos::key::from_string( "rh" ) )
		{
			properties( ).append( prop_enable_ = 1 );
			properties( ).append( prop_planes_ = 7 );
			properties( ).append( prop_rx_ = 0.0 );
			properties( ).append( prop_ry_ = 0.0 );
			properties( ).append( prop_rw_ = 1.0 );
			properties( ).append( prop_rh_ = 1.0 );
		}

		// Indicates if the input will enforce a packet decode
		virtual bool requires_image( ) const { return prop_enable_.value< int >( ) == 1; }

		// This provides the name of the plugin (used in serialisation)
		virtual const pl::wstring get_uri( ) const { return L"invert"; }

	protected:
		// The main access point to the filter
		void do_fetch( ml::frame_type_ptr &result )
		{
			// Acquire values
			acquire_values( );

			// Frame to return
			result = fetch_from_slot( );

			// Handle the frame with image case
			if ( result && prop_enable_.value< int >( ) && result->get_image( ) )
				invert( result );
		}

		// Invert the region and planes requested
		void invert( ml::frame_type_ptr &result )
		{
			if ( !ml::is_yuv_planar( result ) )
				result = frame_convert( result, L"yuv420p" );

			result->set_image( il::conform( result->get_image( ), il::writable ) );

			if ( result && result->get_image( ) )
			{
				il::image_type_ptr image = result->get_image( );
				int planes = prop_planes_.value< int >( );

				// Assumes sane cropping values
				image->crop( int( prop_rx_.value< double >( ) * image->width( ) ), 
							 int( prop_ry_.value< double >( ) * image->height( ) ),
							 int( prop_rw_.value< double >( ) * image->width( ) ), 
							 int( prop_rh_.value< double >( ) * image->height( ) ) );

				// Invert each plane
				if ( planes & 1 )
					invert_plane( image, 0, 256 );
				if ( planes & 2 )
					invert_plane( image, 1, 251 );
				if ( planes & 4 )
					invert_plane( image, 2, 251 );

				// Restore to full image
				image->crop_clear( );
			}
		}

		// Invert the plane requested
		void invert_plane( il::image_type_ptr image, int plane, int negate )
		{
			unsigned char *ptr = image->data( plane );
			int w = image->width( plane );
			int h = image->height( plane );
			int remainder = image->pitch( plane ) - w;
			int t;

			while( h -- )
			{
				t = w;
				while( t -- )
				{
					*ptr = negate - *ptr;
					ptr ++;
				}
				ptr += remainder;
			}
		}

		pcos::property prop_enable_;
		pcos::property prop_planes_;
		pcos::property prop_rx_;
		pcos::property prop_ry_;
		pcos::property prop_rw_;
		pcos::property prop_rh_;
};

ml::filter_type_ptr ML_PLUGIN_DECLSPEC create_invert( const pl::wstring &resource )
{
	return ml::filter_type_ptr( new filter_invert( resource ) );
}

} }
