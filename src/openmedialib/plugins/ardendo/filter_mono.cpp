// Mono filter
//
// Copyright (C) 2007 Ardendo
// Released under the terms of the LGPL.
//
// #filter:mono
//
// Provides a mono video effect (converts all samples over a certain threshold
// to white and all those under to black).

#include "precompiled_headers.hpp"
#include "amf_filter_plugin.hpp"
#include "utility.hpp"

namespace aml { namespace openmedialib {

class ML_PLUGIN_DECLSPEC filter_mono : public ml::filter_simple
{
	public:
		filter_mono( const std::wstring & )
			: ml::filter_simple( )
			, prop_enable_( pcos::key::from_string( "enable" ) )
			, prop_cut_off_( pcos::key::from_string( "cut_off" ) )
			, prop_chroma_( pcos::key::from_string( "chroma" ) )
		{
			properties( ).append( prop_enable_ = 1 );
			properties( ).append( prop_cut_off_ = 128 );
			properties( ).append( prop_chroma_ = 0 );
		}

		// Indicates if the input will enforce a packet decode
		virtual bool requires_image( ) const { return prop_enable_.value< int >( ) == 1; }

		virtual const std::wstring get_uri( ) const { return L"mono"; }

	protected:
		void do_fetch( ml::frame_type_ptr &result )
		{
			result = fetch_from_slot( );

			if ( prop_enable_.value< int >( ) && result )
				mono( result, prop_cut_off_.value< int >( ) );
		}

		void mono( ml::frame_type_ptr &frame, boost::uint8_t cut_off )
		{
			if ( !ml::is_yuv_planar( frame ) )
				frame = ml::frame_convert( frame, _CT("yuv420p") );

			frame->set_image( ml::image::conform( frame->get_image( ), ml::image::writable ) );

            boost::shared_ptr< ml::image::image_type_8 > result = 
                ml::image::coerce< ml::image::image_type_8 >( frame->get_image( ) );

			if ( result )
			{
				if ( prop_chroma_.value< int >( ) == 0 )
				{
					fill_plane( result, 1, 128 );
					fill_plane( result, 2, 128 );
				}
				
				boost::uint8_t *ptr = result->data( 0 );
				int w = result->width( 0 );
				int h = result->height( 0 );
				int r = result->pitch( 0 ) - w;
				int t = w;

				while( h -- )
				{
					t = w;
					while( t -- )
					{
						*ptr = *ptr < cut_off ? 16 : 235;
						ptr ++;
					}
					ptr += r;
				}
			}
		}

		pcos::property prop_enable_;
		pcos::property prop_cut_off_;
		pcos::property prop_chroma_;
};

ml::filter_type_ptr ML_PLUGIN_DECLSPEC create_mono( const std::wstring &resource )
{
	return ml::filter_type_ptr( new filter_mono( resource ) );
}

} }
