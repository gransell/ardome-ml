// Micro VITC filter
//
// Copyright (C) 2009 Ardendo
// Released under the terms of the LGPL.
//
// #filter:mvitc_write
//
// Generates a micro VITC signal.
//
// #filter:mvitc_decode
//
// Decodes a micro VITC signal.

#include "precompiled_headers.hpp"
#include "amf_filter_plugin.hpp"
#include "utility.hpp"

namespace aml { namespace openmedialib {

class ML_PLUGIN_DECLSPEC filter_mvitc_write : public ml::filter_simple
{
	public:
		filter_mvitc_write( const std::wstring & )
			: ml::filter_simple( )
			, prop_enable_( pcos::key::from_string( "enable" ) )
		{
			properties( ).append( prop_enable_ = 1 );
		}

		// Indicates if the input will enforce a packet decode
		virtual bool requires_image( ) const { return prop_enable_.value< int >( ) == 1; }

		virtual const std::wstring get_uri( ) const { return L"mvitc_write"; }

	protected:
		void do_fetch( ml::frame_type_ptr &result )
		{
			acquire_values( );

			result = fetch_from_slot( );

			if ( prop_enable_.value< int >( ) && result )
				write( result );
		}

		void write( ml::frame_type_ptr &frame )
		{
			if ( !ml::is_yuv_planar( frame ) )
				frame = ml::frame_convert( frame, _CT("yuv420p") );

			frame->set_image( ml::image::conform( frame->get_image( ), ml::image::writable ) );

            boost::shared_ptr< ml::image::image_type_8 > result = 
                ml::image::coerce< ml::image::image_type_8 >( frame->get_image( ) );

			if ( result )
			{
				int bits = get_position( );
				
				boost::uint8_t *ptr = result->data( 0 );

				for ( int row = 0; row < 2; row ++ )
				{
					for ( int bit = 0; bit < 32; bit ++ )
					{
						int value = bits & 1;
						bits = bits >> 1;
						*ptr ++ = value ? 235 : 16;
						*ptr ++ = value ? 235 : 16;
						*ptr ++ = value ? 235 : 16;
						*ptr ++ = value ? 235 : 16;
						*ptr ++ = value ? 235 : 16;
					}
					bits = ~ get_position( );
				}
			}
		}

		pcos::property prop_enable_;
};

class ML_PLUGIN_DECLSPEC filter_mvitc_decode : public ml::filter_type
{
	public:
		filter_mvitc_decode( const std::wstring & )
			: ml::filter_type( )
			, prop_enable_( pcos::key::from_string( "enable" ) )
			, prop_mask_( pcos::key::from_string( "mask" ) )
			, found_match_( false )
			, tries_( 3 )
		{
			properties( ).append( prop_enable_ = 1 );
			properties( ).append( prop_mask_ = 1 );
		}

		// Indicates if the input will enforce a packet decode
		virtual bool requires_image( ) const { return prop_enable_.value< int >( ) == 1; }

		virtual const std::wstring get_uri( ) const { return L"mvitc_decode"; }

	protected:
		void do_fetch( ml::frame_type_ptr &result )
		{
			acquire_values( );

			result = fetch_from_slot( );

			if ( prop_enable_.value< int >( ) && result )
				decode( result );
		}

		inline void count( boost::uint8_t ptr, int &low, int &high )
		{
			if ( ptr < 128 )
				low ++;
			else
				high ++;
		}

		void decode( ml::frame_type_ptr &frame )
		{
			if ( !ml::is_yuv_planar( frame ) )
				frame = ml::frame_convert( frame, _CT("yuv420p") );

			frame->set_image( ml::image::conform( frame->get_image( ), ml::image::writable ) );

            boost::shared_ptr< ml::image::image_type_8 > result = 
                ml::image::coerce< ml::image::image_type_8 >( frame->get_image( ) );

			if ( result && tries_ > 0 )
			{
				int value[ 2 ];
				value[ 0 ] = 0;
				value[ 1 ] = 0;

				boost::uint8_t *ptr = result->data( 0 );

				for ( int row = 0; row < 2; row ++ )
				{
					for ( int bit = 0; bit < 32; bit ++ )
					{
						int low = 0;
						int high = 0;
						count( *ptr ++, low, high );
						count( *ptr ++, low, high );
						count( *ptr ++, low, high );
						count( *ptr ++, low, high );
						count( *ptr ++, low, high );
						if ( high >= low )
							value[ row ] |= 1 << bit;
					}
				}

				if ( value[ 0 ] == ~ value[ 1 ] )
				{
					frame->set_position( value[ 0 ] );
					found_match_ = true;
				}
				else
				{
					tries_ --;
				}

				if ( found_match_ && prop_mask_.value< int >( ) )
				{
					boost::uint8_t *ptr = result->data( 0 );
					memcpy( ptr, ptr + result->pitch( 0 ), result->pitch( 0 ) );
				}
				else if ( tries_ == 0 )
				{
					prop_enable_ = 0;
				}
			}
		}

		pcos::property prop_enable_;
		pcos::property prop_mask_;
		bool found_match_;
		int tries_;
};

ml::filter_type_ptr ML_PLUGIN_DECLSPEC create_mvitc_decode( const std::wstring &resource )
{
	return ml::filter_type_ptr( new filter_mvitc_decode( resource ) );
}

ml::filter_type_ptr ML_PLUGIN_DECLSPEC create_mvitc_write( const std::wstring &resource )
{
	return ml::filter_type_ptr( new filter_mvitc_write( resource ) );
}

} }

