// Montage filter
//
// Copyright (C) 2007 Ardendo
// Released under the terms of the LGPL.
//
// #filter:montage
//
// Produces a montage of video frames from the input as a vertical, horizontal
// or grid pattern.
//
// Example:
//
// file.mpg
// filter:extract frames=16
// filter:montage orient=2
//
// Provides a single frame containing 16 frames from file.mpg, with each image scaled to
// a thumbnail and arranged in a 4x4 grid.

#include "precompiled_headers.hpp"
#include "amf_filter_plugin.hpp"
#include "utility.hpp"

#include <iostream>

namespace aml { namespace openmedialib {

static pl::pcos::key key_r_( pl::pcos::key::from_string( "r" ) );
static pl::pcos::key key_g_( pl::pcos::key::from_string( "g" ) );
static pl::pcos::key key_b_( pl::pcos::key::from_string( "b" ) );
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
static pl::pcos::key key_mix_( pcos::key::from_string( "mix" ) );
static pl::pcos::key key_mode_( pcos::key::from_string( "mode" ) );
static pl::pcos::key key_background_( pcos::key::from_string( "background" ) );
static pl::pcos::key key_slots_( pcos::key::from_string( "slots" ) );

static ml::frame_type_ptr decorate( ml::frame_type_ptr frame, int cx, int cy, int cw, int lines )
{
	if ( frame->get_image( ) && frame->get_image( )->pf( ) == _CT("yuv420p") )
	{
		unsigned char R = ( unsigned char )frame->properties( ).get_property_with_key( key_r_ ).value< int >( );
		unsigned char G = ( unsigned char )frame->properties( ).get_property_with_key( key_g_ ).value< int >( );
		unsigned char B = ( unsigned char )frame->properties( ).get_property_with_key( key_b_ ).value< int >( );

		int Y, U, V;
		ml::image::rgb24_to_yuv444( Y, U, V, R, G, B );

		int pitch_y = frame->get_image( )->pitch( 0 );
		int pitch_u = frame->get_image( )->pitch( 1 );
		int pitch_v = frame->get_image( )->pitch( 2 );

        boost::shared_ptr< ml::image::image_type_8 > image = ml::image::coerce< ml::image::image_type_8 >( frame->get_image( ) );

		boost::uint8_t *py = image->data( 0 ) + pitch_y * cy + cx;
		boost::uint8_t *pu = image->data( 1 ) + pitch_u * ( cy / 2 ) + ( cx / 2 );
		boost::uint8_t *pv = image->data( 2 ) + pitch_v * ( cy / 2 ) + ( cx / 2 );

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
		filter_montage( const std::wstring & )
			: filter_type( )
			, prop_width_( pcos::key::from_string( "width" ) )
			, prop_height_( pcos::key::from_string( "height" ) )
			, prop_orient_( pcos::key::from_string( "orient" ) )
			, prop_lines_( pcos::key::from_string( "lines" ) )
			, prop_mode_( pcos::key::from_string( "mode" ) )
			, prop_deferred_( pcos::key::from_string( "deferred" ) )
			, prop_frames_( pcos::key::from_string( "frames" ) )
			, prop_step_( pcos::key::from_string( "step" ) )
			, prop_r_( pcos::key::from_string( "r" ) )
			, prop_g_( pcos::key::from_string( "g" ) )
			, prop_b_( pcos::key::from_string( "b" ) )
			, result_frame_( )
		{
			properties( ).append( prop_width_ = 180 );
			properties( ).append( prop_height_ = 100 );
			properties( ).append( prop_orient_ = 0 );
			properties( ).append( prop_lines_ = 4 );
			properties( ).append( prop_mode_ = std::wstring( L"smart" ) );
			properties( ).append( prop_deferred_ = 0 );
			properties( ).append( prop_frames_ = 0 );
			properties( ).append( prop_step_ = 1 );
			properties( ).append( prop_r_ = 248 );
			properties( ).append( prop_g_ = 118 );
			properties( ).append( prop_b_ = 19 );
		}

		// Indicates if the input will enforce a packet decode
		virtual bool requires_image( ) const { return true; }

		virtual int get_frames( ) const
		{
			if ( prop_frames_.value< int >( ) > 0 )
			{
				ml::input_type_ptr input = fetch_slot( 0 );
				if ( input )
					return input->get_frames( );
			}
			return 1;
		}

		virtual const std::wstring get_uri( ) const { return L"montage"; }

	protected:
		void do_fetch( ml::frame_type_ptr &result )
		{
			acquire_values( );

			result = result_frame_;

			ml::input_type_ptr input = fetch_slot( );

			if ( !result && input && input->get_frames( ) > 0 )
			{
				int position = ( get_position( ) / prop_step_.value< int >( ) ) * prop_step_.value< int >( );
				int length = input->get_frames( ) / prop_step_.value< int >( );
				int in = 0;
				int out = length;

				if ( prop_frames_.value< int >( ) > 0 )
				{
					length = prop_frames_.value< int >( );
					in = position - ( length / 2 ) * prop_step_.value< int >( );
					out = position + ( length / 2 + 1 ) * prop_step_.value< int >( );
				}

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
				pl::pcos::property mix = composite->properties( ).get_property_with_key( key_mix_ );

				m = prop_mode_.value< std::wstring >( );

				for ( int index = in, offset = 0; index < out; index += prop_step_.value< int >( ), offset ++ )
				{
					ml::frame_type_ptr fg;

					if ( index < 0 || index >= input->get_frames( ) )
						continue;

					if ( frames_.find( index ) == frames_.end( ) )
					{
						input->seek( index );
						frames_[ index ] = input->fetch( );
					}

					fg = frames_[ index ]->shallow( );
					fg->set_position( 0 );

					switch( prop_orient_.value< int >( ) )
					{
						case 1:
							x = double( offset * prop_width_.value< int >( ) ) / width;
							y = 0.0;
							w = double( prop_width_.value< int >( ) ) / width;
							h = 1.0;
							break;

						case 2:
							x = double( ( offset % cols ) * prop_width_.value< int >( ) ) / width;
							y = double( ( offset / cols ) * prop_height_.value< int >( ) ) / height;
							w = double( prop_width_.value< int >( ) ) / width;
							h = double( prop_height_.value< int >( ) ) / height;
							break;

						default:
							x = 0.0;
							y = double( offset * prop_height_.value< int >( ) ) / height;
							w = 1.0;
							h = double( prop_height_.value< int >( ) ) / height;
							break;
					}

					if ( prop_frames_.value< int >( ) && index != position )
						mix = 0.5;
					else
						mix = 1.0;

					if ( prop_deferred_.value< int >( ) == 0 )
					{
						pusher_bg->push( result );
						pusher_fg->push( fg );
						result = composite->fetch( );

						if ( prop_lines_.value< int >( ) > 0 && length > 1 )
						{
							int cx = int( x.value< double >( ) * width + 0.5 );
							int cy = int( y.value< double >( ) * height + 0.5 ) + prop_height_.value< int >( ) - prop_lines_.value< int >( );
							int cw = int( ( double( offset ) / ( length - 1 ) ) * prop_width_.value< int >( ) + 0.5 );

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
						assign( fg, key_mode_, prop_mode_.value< std::wstring >( ) );
						assign( fg, key_mix_, mix.value< double >( ) );
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

				if ( prop_frames_.value< int >( ) <= 0 )
					result_frame_ = result;
				else
					result->set_position( get_position( ) );
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

		void assign( ml::frame_type_ptr frame, pl::pcos::key name, std::wstring value )
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
		pl::pcos::property prop_frames_;
		pl::pcos::property prop_step_;
		pl::pcos::property prop_r_;
		pl::pcos::property prop_g_;
		pl::pcos::property prop_b_;
		ml::frame_type_ptr result_frame_;
		std::map< int, ml::frame_type_ptr > frames_;
};

ml::filter_type_ptr ML_PLUGIN_DECLSPEC create_montage( const std::wstring &resource )
{
	return ml::filter_type_ptr( new filter_montage( resource ) );
}

} }
