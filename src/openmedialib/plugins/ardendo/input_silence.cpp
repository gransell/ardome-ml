// A silence provider
//
// Copyright (C) 2007 Ardendo
// Released under the LGPL.
//
// #input:silence:
//
// Produces frames with silence at a specified frame rate and audio make up.

#include "precompiled_headers.hpp"
#include "amf_filter_plugin.hpp"
#include "utility.hpp"

namespace aml { namespace openmedialib {

class ML_PLUGIN_DECLSPEC input_silence : public ml::input_type
{
	public:
		input_silence( ) 
			: ml::input_type( ) 
			, prop_af_( pcos::key::from_string( "af" ) )
			, prop_fps_num_( pcos::key::from_string( "fps_num" ) )
			, prop_fps_den_( pcos::key::from_string( "fps_den" ) )
			, prop_frequency_( pcos::key::from_string( "frequency" ) )
			, prop_channels_( pcos::key::from_string( "channels" ) )
			, prop_out_( pcos::key::from_string( "out" ) )
			, prop_profile_( pcos::key::from_string( "profile" ) )
		{
			properties( ).append( prop_af_ = pl::wstring( ml::audio::FORMAT_PCM16 ) );
			properties( ).append( prop_fps_num_ = 25 );
			properties( ).append( prop_fps_den_ = 1 );
			properties( ).append( prop_frequency_ = 48000 );
			properties( ).append( prop_channels_ = 2 );
			properties( ).append( prop_out_ = INT_MAX );
			properties( ).append( prop_profile_ = pl::wstring( L"" ) );
		}

		virtual ~input_silence( ) { }

		// Indicates if the input will enforce a packet decode
		virtual bool requires_image( ) const { return false; }

		// Basic information
		virtual const pl::wstring get_uri( ) const { return L"silence:"; }
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
			// Construct a frame and populate with basic information
			result = ml::frame_type_ptr( new ml::frame_type( ) );

			int fps_num = prop_fps_num_.value< int >( );
			int fps_den = prop_fps_den_.value< int >( );
			int frequency = prop_frequency_.value< int >( );
			int channels = prop_channels_.value< int >( );
			int samples = ml::audio::samples_for_frame( get_position( ), frequency, fps_num, fps_den, prop_profile_.value< pl::wstring >( ) );

			result->set_audio( ml::audio::allocate( prop_af_.value< pl::wstring >( ), frequency, channels, samples, true ) );

			result->set_fps( fps_num, fps_den );
			result->set_pts( get_position( ) * 1.0 / fps( ) );
			result->set_duration( double( samples ) / frequency );
			result->set_position( get_position( ) );
		}

	private:
		pcos::property prop_af_;
		pcos::property prop_fps_num_;
		pcos::property prop_fps_den_;
		pcos::property prop_frequency_;
		pcos::property prop_channels_;
		pcos::property prop_out_;
		pcos::property prop_profile_;
};

ml::input_type_ptr ML_PLUGIN_DECLSPEC create_input_silence( const pl::wstring &resource )
{
	return ml::input_type_ptr( new input_silence( ) );
}

} }
