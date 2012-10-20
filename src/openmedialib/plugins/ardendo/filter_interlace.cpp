#include "precompiled_headers.hpp"
#include "amf_filter_plugin.hpp"
#include "utility.hpp"

namespace aml { namespace openmedialib {

class ML_PLUGIN_DECLSPEC filter_interlace : public ml::filter_type
{
	public:
		// Filter_type overloads
		filter_interlace( const std::wstring & )
			: ml::filter_type( )
			, prop_enable_( pcos::key::from_string( "enable" ) )
			, prop_bff_( pcos::key::from_string( "bff" ) )
		{
			properties( ).append( prop_enable_ = 1 );
			properties( ).append( prop_bff_ = 1 );
		}

		// Indicates if the input will enforce a packet decode
		virtual bool requires_image( ) const { return prop_enable_.value< int >( ) == 1; }

		// This provides the name of the plugin (used in serialisation)
		virtual const std::wstring get_uri( ) const { return L"interlace"; }

		// Has twice the number of frames as the source provides
		virtual int get_frames( ) const
		{
			int result = 0;
			ml::input_type_ptr input = fetch_slot( 0 );
			if ( input )
				result = prop_enable_.value< int >( ) ? int( input->get_frames( ) / 2 ) : input->get_frames( );
			return result;
		}

	protected:
		// The main access point to the filter
		void do_fetch( ml::frame_type_ptr &result )
		{
			if ( prop_enable_.value< int >( ) )
				result = merge( fetch_slot( ) );
			else
				result = fetch_from_slot( );
		}

		ml::frame_type_ptr merge( ml::input_type_ptr source )
		{
			ml::frame_type_ptr frame0 = source->fetch( get_position( ) * 2 );
			ml::frame_type_ptr frame1 = source->fetch( get_position( ) * 2 + 1 );

			// Merge fields
			if ( prop_bff_.value< int >( ) )
			{
				frame0->set_image( merge_fields( frame1->get_image( ), frame0->get_image( ) ) );
				frame0->set_alpha( merge_fields( frame1->get_alpha( ), frame0->get_alpha( ) ) );
				frame0->get_image( )->set_field_order( il::bottom_field_first );
			}
			else
			{
				frame0->set_image( merge_fields( frame0->get_image( ), frame1->get_image( ) ) );
				frame0->set_alpha( merge_fields( frame0->get_alpha( ), frame1->get_alpha( ) ) );
				frame0->get_image( )->set_field_order( il::top_field_first );
			}

			frame0->get_image( )->set_position( get_position( ) );
			frame0->set_duration( 2 * frame0->get_duration( ) );
			frame0->set_position( get_position( ) );

			if ( frame0->get_audio( ) && frame1->get_audio( ) )
			{
				ml::audio_type_ptr a = frame0->get_audio( );
				ml::audio_type_ptr b = frame1->get_audio( );
				ml::audio_type_ptr audio = ml::audio::allocate( a, -1, -1, a->samples( ) + b->samples( ), false );
				memcpy( ( boost::uint8_t * )audio->pointer( ), a->pointer( ), a->size( ) );
				memcpy( ( boost::uint8_t * )audio->pointer( ) + a->size( ), b->pointer( ), b->size( ) );
				frame0->set_audio( audio );
			}

			int num, den;
			frame0->get_fps( num, den );
			frame0->set_fps( num / 2, den );

			return frame0;
		}

		il::image_type_ptr merge_fields( il::image_type_ptr frame0, il::image_type_ptr frame1 )
		{
			il::image_type_ptr image;

			if ( !frame0 || !frame1 ) return image;

			image = il::allocate( frame0->pf( ), frame0->width( ), frame0->height( ) );

			for ( int p = 0; p < image->plane_count( ); p ++ )
			{
				boost::uint8_t *dest_ptr = image->data( p );
				boost::uint8_t *frame0_ptr = frame0->data( p );
				boost::uint8_t *frame1_ptr = frame1->data( p ) + frame1->pitch( p );
				int frame0_pitch = frame0->pitch( p ) * 2;
				int frame1_pitch = frame1->pitch( p ) * 2;
				int linesize = image->linesize( p );
				int pitch = image->pitch( p );
				int height = frame0->height( p ) / 2;

				while( height -- )
				{
					memcpy( dest_ptr, frame0_ptr, linesize );
					dest_ptr += pitch;
					frame0_ptr += frame0_pitch;
					memcpy( dest_ptr, frame1_ptr, linesize );
					dest_ptr += pitch;
					frame1_ptr += frame1_pitch;
				}
			}

			return image;
		}

		pcos::property prop_enable_;
		pcos::property prop_bff_;
};

ml::filter_type_ptr ML_PLUGIN_DECLSPEC create_interlace( const std::wstring &resource )
{
	return ml::filter_type_ptr( new filter_interlace( resource ) );
}

} } 
