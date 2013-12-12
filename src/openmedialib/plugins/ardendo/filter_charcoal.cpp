// Charcoal filter
//
// Copyright (C) 2007 Ardendo
// Released under the LGPL.
//
// #filter:charcoal
//
// Provides a charcoal video effect.
//
// Example:
//
// file.mpg
// filter:charcoal mix=0.5
//
// Will provide a colourised charcoal rendering of file.mpg - the mix level
// dictates how the charcoal rendering is mixed with the source.

#include "precompiled_headers.hpp"
#include "amf_filter_plugin.hpp"
#include "utility.hpp"

namespace ml = olib::openmedialib::ml;

namespace aml { namespace openmedialib {

class ML_PLUGIN_DECLSPEC filter_charcoal : public ml::filter_simple
{
	public:
		// Filter_type overloads
		explicit filter_charcoal( const std::wstring & )
			: ml::filter_simple( )
			, prop_enable_( pcos::key::from_string( "enable" ) )
			, prop_scale_( pcos::key::from_string( "scale" ) )
			, prop_mix_( pcos::key::from_string( "mix" ) )
		{
			properties( ).append( prop_enable_ = 1 );
			properties( ).append( prop_scale_ = 1.5 );
			properties( ).append( prop_mix_ = 0.0 );
		}

		// Indicates if the input will enforce a packet decode
		virtual bool requires_image( ) const { return prop_enable_.value< int >( ) == 1; }

		// This provides the name of the plugin (used in serialisation)
		virtual const std::wstring get_uri( ) const { return L"charcoal"; }

	protected:
		// The main access point to the filter
		void do_fetch( ml::frame_type_ptr &result )
		{
			// Frame to return
			result = fetch_from_slot( );

			// Handle the frame with image case
			if ( result && prop_enable_.value< int >( ) && result->get_image( ) )
				charcoal( result );
		}

	protected:

		// Invert the region and planes requested
		void charcoal( ml::frame_type_ptr &result )
		{
			if ( result && result->get_image( ) )
			{
				if ( !ml::is_yuv_planar( result ) || result->get_image( )->bitdepth( ) != 8 )
					result = frame_convert( result, _CT("yuv420p") );
				ml::image_type_ptr input = result->get_image( );
				ARENFORCE_MSG( input, "Unable to acquire a valid image" );
				ml::image_type_ptr output = ml::image::allocate( input );
				ARENFORCE_MSG( output, "Unable to allocate an output image" );
				charcoal_plane( output, input, 0 );
				copy_plane( output, input, 1 );
				copy_plane( output, input, 2 );
				result->set_image( output );
			}
		}

		void charcoal_plane( ml::image_type_ptr o, ml::image_type_ptr i, int plane )
		{
            boost::shared_ptr< ml::image::image_type_8 > output = ml::image::coerce< ml::image::image_type_8 >( o );
            boost::shared_ptr< ml::image::image_type_8 > input = ml::image::coerce< ml::image::image_type_8 >( i );
            boost::uint8_t *dst = output->data( plane );
			int w = output->width( plane );
			int h = output->height( plane ) - 2;
			int p = input->pitch( plane );
            boost::uint8_t *src = input->data( plane ) + p + 1;
			int dst_rem = output->pitch( plane ) - w;
			int src_rem = input->pitch( plane ) - w + 2;
			int t;
			int row[ 2 ];
			int sum;
			int scale = int( 256 * prop_scale_.value< double >( ) );
			int mix = int( 256 * prop_mix_.value< double >( ) );

			memset( dst, 16, w );
			dst += output->pitch( plane );

			if ( mix == 0 )
			{
				while( h -- )
				{
					*dst ++ = 16;
					t = w - 2;
					while( t -- )
					{
						row[ 0 ] = ( *(src+p-1) - *(src-p-1) ) + ( ( *(src+p) - *(src-p) ) << 1 ) + ( *(src+p+1) - *(src+p-1) );
						row[ 1 ] = ( *(src-p+1) - *(src-p-1) ) + ( ( *(src+1) - *(src-1) ) << 1 ) + ( *(src+p+1) - *(src+p-1) );
						sum = ( ( 3 * sqrti( row[ 0 ] * row[ 0 ] + row[ 1 ] * row[ 1 ] ) / 2 ) * scale ) >> 8;
						*dst ++ = 251 - ( sum < 16 ? 16 : sum > 235 ? 235 : boost::uint8_t( sum ) );
						src ++;
					}
					*dst ++ = 16;
					dst += dst_rem;
					src += src_rem;
				}
				memset( dst, 16, w );
			}
			else
			{
				while( h -- )
				{
					*dst ++ = 16;
					t = w - 2;
					while( t -- )
					{
						row[ 0 ] = ( *(src+p-1) - *(src-p-1) ) + ( ( *(src+p) - *(src-p) ) << 1 ) + ( *(src+p+1) - *(src+p-1) );
						row[ 1 ] = ( *(src-p+1) - *(src-p-1) ) + ( ( *(src+1) - *(src-1) ) << 1 ) + ( *(src+p+1) - *(src+p-1) );
						sum = ( ( 3 * sqrti( row[ 0 ] * row[ 0 ] + row[ 1 ] * row[ 1 ] ) / 2 ) * scale ) >> 8;
						sum = 251 - ( sum < 16 ? 16 : sum > 235 ? 235 : sum );
						sum = ( ( 256 - mix ) * sum + ( mix * *src ) ) >> 8;
						*dst ++ = boost::uint8_t( sum );
						src ++;
					}
					*dst ++ = 16;
					dst += dst_rem;
					src += src_rem;
				}
				memset( dst, 16, w );
			}
		}

		pcos::property prop_enable_;
		pcos::property prop_scale_;
		pcos::property prop_mix_;
};

ml::filter_type_ptr ML_PLUGIN_DECLSPEC create_charcoal( const std::wstring &resource )
{
	return ml::filter_type_ptr( new filter_charcoal( resource ) );
}

} }
