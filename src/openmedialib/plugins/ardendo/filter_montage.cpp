#include "precompiled_headers.hpp"
#include "amf_filter_plugin.hpp"
#include "utility.hpp"

#include <iostream>

namespace aml { namespace openmedialib {

static pl::pcos::key key_r_( pl::pcos::key::from_string( "r" ) );
static pl::pcos::key key_g_( pl::pcos::key::from_string( "g" ) );
static pl::pcos::key key_b_( pl::pcos::key::from_string( "b" ) );
static pl::pcos::key key_frame_rescale_cb_( pcos::key::from_string( "frame_rescale_cb" ) );
static pl::pcos::key key_image_rescale_cb_( pcos::key::from_string( "image_rescale_cb" ) );
static pl::pcos::key key_width_( pcos::key::from_string( "width" ) );
static pl::pcos::key key_height_( pcos::key::from_string( "height" ) );
static pl::pcos::key key_sar_num_( pcos::key::from_string( "sar_num" ) );
static pl::pcos::key key_sar_den_( pcos::key::from_string( "sar_den" ) );
static pl::pcos::key key_x_( pcos::key::from_string( "x" ) );
static pl::pcos::key key_y_( pcos::key::from_string( "y" ) );
static pl::pcos::key key_w_( pcos::key::from_string( "w" ) );
static pl::pcos::key key_h_( pcos::key::from_string( "h" ) );
static pl::pcos::key key_rx_( pcos::key::from_string( "rx" ) );
static pl::pcos::key key_ry_( pcos::key::from_string( "ry" ) );
static pl::pcos::key key_rw_( pcos::key::from_string( "rw" ) );
static pl::pcos::key key_rh_( pcos::key::from_string( "rh" ) );
static pl::pcos::key key_mode_( pcos::key::from_string( "mode" ) );
static pl::pcos::key key_background_( pcos::key::from_string( "background" ) );
static pl::pcos::key key_slots_( pcos::key::from_string( "slots" ) );

// These are defined in filter_compositor.cpp - they are defined using mc booster when available
extern il::image_type_ptr image_rescale( const il::image_type_ptr &img, int w, int h, int d, il::rescale_filter filter );
extern ml::frame_type_ptr frame_rescale( ml::frame_type_ptr frame, int w, int h, il::rescale_filter filter );

static ml::frame_type_ptr decorate( ml::frame_type_ptr frame, int cx, int cy, int cw, int lines )
{
	if ( frame->get_image( ) && frame->get_image( )->pf( ) == L"yuv420p" )
	{
		unsigned char R = ( unsigned char )frame->properties( ).get_property_with_key( key_r_ ).value< int >( );
		unsigned char G = ( unsigned char )frame->properties( ).get_property_with_key( key_g_ ).value< int >( );
		unsigned char B = ( unsigned char )frame->properties( ).get_property_with_key( key_b_ ).value< int >( );

		int Y, U, V;
		il::rgb24_to_yuv444( Y, U, V, R, G, B );

		int pitch_y = frame->get_image( )->pitch( 0 );
		int pitch_u = frame->get_image( )->pitch( 1 );
		int pitch_v = frame->get_image( )->pitch( 2 );

		boost::uint8_t *py = frame->get_image( )->data( 0 ) + pitch_y * cy + cx;
		boost::uint8_t *pu = frame->get_image( )->data( 1 ) + pitch_u * ( cy / 2 ) + ( cx / 2 );
		boost::uint8_t *pv = frame->get_image( )->data( 2 ) + pitch_v * ( cy / 2 ) + ( cx / 2 );

		while( lines -- )
		{
			memset( py, Y, cw );
			py += pitch_y;
			if ( lines % 2 == 0 )
			{
				memset( pu, U, ( cw + 1 ) / 2 );
				pu += pitch_u;
				memset( pv, V, ( cw + 1 ) / 2 );
				pv += pitch_v;
			}
		}
	}

	return frame;
}

class ML_PLUGIN_DECLSPEC filter_montage : public ml::filter_type
{
	public:
		filter_montage( const pl::wstring & )
			: filter_type( )
			, prop_width_( pcos::key::from_string( "width" ) )
			, prop_height_( pcos::key::from_string( "height" ) )
			, prop_orient_( pcos::key::from_string( "orient" ) )
			, prop_lines_( pcos::key::from_string( "lines" ) )
			, prop_mode_( pcos::key::from_string( "mode" ) )
			, prop_deferred_( pcos::key::from_string( "deferred" ) )
			, prop_r_( pcos::key::from_string( "r" ) )
			, prop_g_( pcos::key::from_string( "g" ) )
			, prop_b_( pcos::key::from_string( "b" ) )
			, result_frame_( )
		{
			properties( ).append( prop_width_ = 180 );
			properties( ).append( prop_height_ = 100 );
			properties( ).append( prop_orient_ = 0 );
			properties( ).append( prop_lines_ = 4 );
			properties( ).append( prop_mode_ = pl::wstring( L"smart" ) );
			properties( ).append( prop_deferred_ = 0 );
			properties( ).append( prop_r_ = 248 );
			properties( ).append( prop_g_ = 118 );
			properties( ).append( prop_b_ = 19 );
		}

		// Indicates if the input will enforce a packet decode
		virtual bool requires_image( ) const { return true; }

		virtual int get_frames( ) const
		{ return 1; }

		virtual const opl::wstring get_uri( ) const { return L"montage"; }

	protected:
		void do_fetch( ml::frame_type_ptr &result )
		{
			acquire_values( );

			result = result_frame_;
			ml::input_type_ptr input = fetch_slot( );

			if ( !result && input && input->get_frames( ) > 0 )
			{
				int length = input->get_frames( );

				int height = prop_height_.value< int >( ) * length;
				int width = prop_width_.value< int >( );

				int rows = 1;
				int cols = 1;

				switch( prop_orient_.value< int >( ) )
				{
					case 1:
						width = prop_width_.value< int >( ) * length;
						height = prop_height_.value< int >( );
						break;

					case 2:
        				while( length > rows * cols )
						{
            				cols *= 2;
            				rows *= 2;
						}

        				while( length <= ( rows - 1 ) * cols )
            				if ( length <= ( cols - 1 ) *  -- rows )
                				cols --;

						width = cols * prop_width_.value< int >( );
						height = rows * prop_height_.value< int >( );
						break;

					default:
						break;
				}

            	ml::input_type_ptr pusher_bg = ml::create_input( L"pusher:" );
            	ml::input_type_ptr pusher_fg = ml::create_input( L"pusher:" );
            	ml::input_type_ptr composite = ml::create_filter( L"composite" );

				pusher_bg->init( );
				pusher_fg->init( );
				composite->init( );

				composite->connect( pusher_bg, 0 );
				composite->connect( pusher_fg, 1 );

				composite->properties( ).get_property_with_key( key_frame_rescale_cb_ ) = boost::uint64_t( frame_rescale );
				composite->properties( ).get_property_with_key( key_image_rescale_cb_ ) = boost::uint64_t( image_rescale );

				ml::input_type_ptr bg = ml::create_input( L"colour:" );
				bg->properties( ).get_property_with_key( key_width_ ) = width;
				bg->properties( ).get_property_with_key( key_height_ ) = height;
				bg->properties( ).get_property_with_key( key_sar_num_ ) = 1;
				bg->properties( ).get_property_with_key( key_sar_den_ ) = 1;

				bg->init( );

				result = bg->fetch( );

				pl::pcos::property x = composite->properties( ).get_property_with_key( key_rx_ );
				pl::pcos::property y = composite->properties( ).get_property_with_key( key_ry_ );
				pl::pcos::property w = composite->properties( ).get_property_with_key( key_rw_ );
				pl::pcos::property h = composite->properties( ).get_property_with_key( key_rh_ );
				pl::pcos::property m = composite->properties( ).get_property_with_key( key_mode_ );

				m = prop_mode_.value< pl::wstring >( );

				for ( int position = 0; position < length; position ++ )
				{
					input->seek( position );
					ml::frame_type_ptr fg = input->fetch( );

					switch( prop_orient_.value< int >( ) )
					{
						case 1:
							x = double( position * prop_width_.value< int >( ) ) / width;
							y = 0.0;
							w = double( prop_width_.value< int >( ) ) / width;
							h = 1.0;
							break;

						case 2:
							x = double( ( position % cols ) * prop_width_.value< int >( ) ) / width;
							y = double( ( position / cols ) * prop_height_.value< int >( ) ) / height;
							w = double( prop_width_.value< int >( ) ) / width;
							h = double( prop_height_.value< int >( ) ) / height;
							break;

						default:
							x = 0.0;
							y = double( position * prop_height_.value< int >( ) ) / height;
							w = 1.0;
							h = double( prop_height_.value< int >( ) ) / height;
							break;
					}

					if ( prop_deferred_.value< int >( ) == 0 )
					{
						pusher_bg->push( result );
						pusher_fg->push( fg );
						result = composite->fetch( );

						if ( prop_lines_.value< int >( ) > 0 && length > 1 )
						{
							int cx = int( x.value< double >( ) * width + 0.5 );
							int cy = int( y.value< double >( ) * height + 0.5 ) + prop_height_.value< int >( ) - prop_lines_.value< int >( );
							int cw = int( ( double( position ) / ( length - 1 ) ) * prop_width_.value< int >( ) + 0.5 );

							// Properties attached to result
							pl::pcos::property prop_r( key_r_ );
							pl::pcos::property prop_g( key_g_ );
							pl::pcos::property prop_b( key_b_ );

							result->properties( ).append( prop_r = prop_r_.value< int >( ) );
							result->properties( ).append( prop_g = prop_g_.value< int >( ) );
							result->properties( ).append( prop_b = prop_b_.value< int >( ) );

							decorate( result, cx, cy, cw, prop_lines_.value< int >( ) );
						}
					}
					else
					{
						assign( fg, key_x_, x.value< double >( ) );
						assign( fg, key_y_, y.value< double >( ) );
						assign( fg, key_w_, w.value< double >( ) );
						assign( fg, key_h_, h.value< double >( ) );
						assign( fg, key_mode_, prop_mode_.value< pl::wstring >( ) );
						result->push( fg );
					}
				}

				if ( prop_deferred_.value< int >( ) )
				{
					pl::pcos::property background( key_background_ );
					result->properties().append( background = 1 );
					pl::pcos::property slots( key_slots_ );
					result->properties().append( slots = length + 1 );
				}

				result_frame_ = result;
			}
		}

		void assign( ml::frame_type_ptr frame, pl::pcos::key name, double value )
		{
			if ( frame->properties( ).get_property_with_key( name ).valid( ) )
			{
				frame->properties( ).get_property_with_key( name ) = value;
			}
			else
			{
				pl::pcos::property prop( name );
				frame->properties( ).append( prop = value );
			}
		}

		void assign( ml::frame_type_ptr frame, pl::pcos::key name, pl::wstring value )
		{
			if ( frame->properties( ).get_property_with_key( name ).valid( ) )
			{
				frame->properties( ).get_property_with_key( name ) = value;
			}
			else
			{
				pl::pcos::property prop( name );
				frame->properties( ).append( prop = value );
			}
		}

	private:
		pl::pcos::property prop_width_;
		pl::pcos::property prop_height_;
		pl::pcos::property prop_orient_;
		pl::pcos::property prop_lines_;
		pl::pcos::property prop_mode_;
		pl::pcos::property prop_deferred_;
		pl::pcos::property prop_r_;
		pl::pcos::property prop_g_;
		pl::pcos::property prop_b_;
		ml::frame_type_ptr result_frame_;
};

ml::filter_type_ptr ML_PLUGIN_DECLSPEC create_montage( const pl::wstring &resource )
{
	return ml::filter_type_ptr( new filter_montage( resource ) );
}

} }
