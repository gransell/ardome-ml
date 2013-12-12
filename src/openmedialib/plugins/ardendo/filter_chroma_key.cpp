// Chroma Key filter
//
// Copyright (C) 2007 Ardendo
// Released under the LGPL.
//
// #filter:chroma
//
// Experimental - provides a chroma keying filter (maps colours to alpha).

#include "precompiled_headers.hpp"
#include "amf_filter_plugin.hpp"
#include "utility.hpp"

namespace aml { namespace openmedialib {

class ML_PLUGIN_DECLSPEC filter_chroma_key : public ml::filter_simple
{
	public:
		// Filter_type overloads
		explicit filter_chroma_key( const std::wstring & )
			: ml::filter_simple( )
			, prop_enable_( pcos::key::from_string( "enable" ) )
			, prop_u_( pcos::key::from_string( "u" ) )
			, prop_v_( pcos::key::from_string( "v" ) )
			, prop_var_( pcos::key::from_string( "var" ) )
		{
			properties( ).append( prop_enable_ = 1 );
			properties( ).append( prop_u_ = 145 );
			properties( ).append( prop_v_ = 112 );
			properties( ).append( prop_var_ = 4 );

		}

		// Indicates if the input will enforce a packet decode
		virtual bool requires_image( ) const { return prop_enable_.value< int >( ) == 1; }

		// This provides the name of the plugin (used in serialisation)
		virtual const std::wstring get_uri( ) const { return L"chroma_key"; }

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
				chroma_key( result, prop_u_.value< int >( ), prop_v_.value< int >( ), prop_var_.value< int >( ) );
		}

	protected:

		// Invert the region and planes requested
		void chroma_key( ml::frame_type_ptr &result, boost::uint8_t u, boost::uint8_t v, int var )
		{
			// Convert frame if necessary
			if ( !ml::is_yuv_planar( result ) )
				result = frame_convert( result, _CT("yuv420p") );

			// If frame (after convert) is still valid
			if ( result && result->get_image( ) )
			{
                ml::image_type_ptr alpha = result->get_alpha( );
                boost::shared_ptr< ml::image::image_type_8 > input = ml::image::coerce< ml::image::image_type_8 >( result->get_image( ) );

				// Get the size of the image
				size_t w = input->width( );
				size_t h = input->height( );

				// Get the dimensions of the chroma
				size_t cw = input->width( 1 );
				size_t ch = input->height( 1 );

				// Make sure that we have an alpha mask which is the same size as the chroma
				if ( !alpha )
				{
					alpha = ml::image::allocate( _CT("l8"), cw, ch );
					fill_plane( alpha, 0, 255 );
				}
				else
				{
					alpha = ml::image::rescale( alpha, cw, ch );
                }

                boost::shared_ptr< ml::image::image_type_8 > alpha_type_8 = ml::image::coerce< ml::image::image_type_8 >( alpha );
				// Process chroma
				boost::uint8_t *pu = input->data( 1 );
				boost::uint8_t *pv = input->data( 2 );
				boost::uint8_t *pa = alpha_type_8->data( );

				size_t ru = input->pitch( 1 ) - cw;
				size_t rv = input->pitch( 2 ) - cw;
				size_t ra = alpha_type_8->pitch( ) - cw;

				while( ch -- )
				{
					for ( size_t t = 0; t < cw; t ++ )
					{
						// TODO: Proper maths here...
						if ( RANGE( *pu, u - var, u + var ) && RANGE( *pv, v - var, v + var ) )
							*pa = 0;
						pu ++;
						pv ++;
						pa ++;
					}
					pu += ru;
					pv += rv;
					pa += ra;
				}

				// Update the alpha on the frame accordingly
				alpha = ml::image::rescale( alpha, w, h, ml::image::BICUBIC_SAMPLING );
				result->set_alpha( alpha );
			}
		}

		pcos::property prop_enable_;
		pcos::property prop_u_;
		pcos::property prop_v_;
		pcos::property prop_var_;
};

ml::filter_type_ptr ML_PLUGIN_DECLSPEC create_chroma_key( const std::wstring &resource )
{
	return ml::filter_type_ptr( new filter_chroma_key( resource ) );
}

} }
