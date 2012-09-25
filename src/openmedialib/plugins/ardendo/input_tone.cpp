// Tone generator
//
// Copyright (C) 2009 Ardendo
// Released under the LGPL.
//
// #input:tone:
//
// This input creates a sine-wave based tone which is aligned to the frame 
// rate specified - it generates a number of full sine waves based on the
// periods properties.

#include "precompiled_headers.hpp"
#include "amf_filter_plugin.hpp"
#include "utility.hpp"
#include <cmath>

namespace aml { namespace openmedialib {

class ML_PLUGIN_DECLSPEC input_tone : public ml::input_type
{
	public:
		input_tone( ) 
			: ml::input_type( ) 
			, prop_fps_num_( pcos::key::from_string( "fps_num" ) )
			, prop_fps_den_( pcos::key::from_string( "fps_den" ) )
			, prop_frequency_( pcos::key::from_string( "frequency" ) )
			, prop_channels_( pcos::key::from_string( "channels" ) )
			, prop_out_( pcos::key::from_string( "out" ) )
			, prop_periods_( pcos::key::from_string( "periods" ) )
			, prop_peak_( pcos::key::from_string( "peak" ) )
			, prop_oscillate_( pcos::key::from_string( "oscillate" ) )
			, prop_profile_( pcos::key::from_string( "profile" ) )
		{
			properties( ).append( prop_fps_num_ = 25 );
			properties( ).append( prop_fps_den_ = 1 );
			properties( ).append( prop_frequency_ = 48000 );
			properties( ).append( prop_channels_ = 2 );
			properties( ).append( prop_out_ = 250 );
			properties( ).append( prop_periods_ = 20 );
			properties( ).append( prop_peak_ = 0.5 );
			properties( ).append( prop_oscillate_ = 0 );
			properties( ).append( prop_profile_ = pl::wstring( L"dv" ) );
		}

		virtual ~input_tone( ) { }

		// Indicates if the input will enforce a packet decode
		virtual bool requires_image( ) const { return false; }

		// Basic information
		virtual const pl::wstring get_uri( ) const { return L"tone:"; }
		virtual const pl::wstring get_mime_type( ) const { return L""; }

		// Audio/Visual
		virtual int get_frames( ) const { return prop_out_.value< int >( ); }
		virtual bool is_seekable( ) const { return true; }

		// Visual
		virtual void get_fps( int &num, int &den ) const 
		{
			num = prop_fps_num_.value< int >( );
			den = prop_fps_den_.value< int >( );
		}

		virtual double fps( ) const
		{
			int num, den;
			get_fps( num, den );
			return den != 0 ? double( num ) / double( den ) : 1;
		}

		virtual bool reuse( ) { return false; }

	protected:
		// Fetch method
		void do_fetch( ml::frame_type_ptr &result )
		{
			// Obtain property values
			acquire_values( );

			ml::frame_type *frame = new ml::frame_type( );

			int fps_num = prop_fps_num_.value< int >( );
			int fps_den = prop_fps_den_.value< int >( );
			int frequency = prop_frequency_.value< int >( );
			int channels = prop_channels_.value< int >( );
			int samples = ml::audio::samples_for_frame( get_position( ), frequency, fps_num, fps_den, prop_profile_.value< pl::wstring >( ) );
			int periods = prop_periods_.value< int >( );
			int oscillate = prop_oscillate_.value< int >( );
			double peak = prop_peak_.value< double >( );

			if ( oscillate )
			{
				periods = ( ( get_position( ) % oscillate ) + 1 ) * 2;
			}

			if ( channels )
			{
				ml::audio::floats_ptr aud = ml::audio::floats_ptr( new ml::audio::floats( frequency, channels, samples, false ) );
				float *ptr = aud->data( );
				for ( int i = 0; i < samples; i ++ )
					for ( int j = 0; j < channels; j ++ )
						*ptr ++ = float( std::sin( ( periods * 2 * 3.1415 * i ) / samples ) * peak );

				frame->set_audio( aud );
			}

			// Construct a frame and populate with basic information
			frame->set_fps( fps_num, fps_den );
			frame->set_pts( get_position( ) * 1.0 / fps( ) );
			frame->set_duration( double( samples ) / frequency );
			frame->set_position( get_position( ) );

			// Return a frame
			result = ml::frame_type_ptr( frame );
		}

	private:
		pcos::property prop_fps_num_;
		pcos::property prop_fps_den_;
		pcos::property prop_frequency_;
		pcos::property prop_channels_;
		pcos::property prop_out_;
		pcos::property prop_periods_;
		pcos::property prop_peak_;
		pcos::property prop_oscillate_;
		pcos::property prop_profile_;
};

ml::input_type_ptr ML_PLUGIN_DECLSPEC create_input_tone( const pl::wstring &resource )
{
	return ml::input_type_ptr( new input_tone( ) );
}

} }
