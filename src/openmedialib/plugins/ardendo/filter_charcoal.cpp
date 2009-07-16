// Charcoal filter
//
// Copyright (C) 2007 Ardendo
#include "precompiled_headers.hpp"
#include "amf_filter_plugin.hpp"
#include "utility.hpp"

namespace amf { namespace openmedialib {

class ML_PLUGIN_DECLSPEC filter_charcoal : public ml::filter_type
{
	public:
		// Filter_type overloads
		explicit filter_charcoal( const pl::wstring & )
			: ml::filter_type( )
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
		virtual const pl::wstring get_uri( ) const { return L"charcoal"; }

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
				charcoal( result );
		}

	protected:

		// Invert the region and planes requested
		void charcoal( ml::frame_type_ptr &result )
		{
			if ( !ml::is_yuv_planar( result ) )
				result = frame_convert( result, L"yuv420p" );

			if ( result && result->get_image( ) )
			{
				il::image_type_ptr input = result->get_image( );
				il::image_type_ptr output = il::allocate( input );
				charcoal_plane( output, input, 0 );
				copy_plane( output, input, 1 );
				copy_plane( output, input, 2 );
				result->set_image( output );
			}
		}

		void charcoal_plane( il::image_type_ptr output, il::image_type_ptr input, int plane )
		{
			unsigned char *dst = output->data( plane );
			int w = output->width( plane );
			int h = output->height( plane ) - 2;
			int p = input->pitch( plane );
			unsigned char *src = input->data( plane ) + p + 1;
			int dst_rem = output->pitch( plane ) - w;
			int src_rem = input->pitch( plane ) - w + 2;
			int t;
			int row[ 2 ];
			int sum;
			int scale = int( 256 * prop_scale_.value< double >( ) );
			int mix = int( 256 * prop_mix_.value< double >( ) );

			memset( dst, 16, w );
			dst += dst_rem;

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

ml::filter_type_ptr ML_PLUGIN_DECLSPEC create_charcoal( const pl::wstring &resource )
{
	return ml::filter_type_ptr( new filter_charcoal( resource ) );
}

} }
