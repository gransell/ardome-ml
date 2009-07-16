// Pitch filter
//
// Copyright (C) 2007 Ardendo
//
// This is a very useful filter - can be used in a number of situations.
//
// In its most basic form, it provides frame rate interpolation by pitch 
// alteration - this is very useful for converting telecine (24000:1001) to 
// PAL (25:1) since it doesn't involve the generation of additional frames.
//
// Example: amfbatch <movie.avi> filter:pitch 
//
// In a general case, it can be used for speed up/slow down of video by 
// altering the fps_num/fps_den or speed accordingly.
//
// As another use case, this filter can also be used to ensure that the 
// sample count coming from two slots to a mixing filter - composite from olibs
// gensys for example - match precisely. The theory about this one is mostly 
// down to frame rates which give uneven samples per frame - NTSC being a 
// good case in point. 
//
// In the NTSC and similar cases, the sample count per frame deviates over a 
// fixed frame cycle - therefore two inputs connected to the composite may 
// not necessarily be at the same point in the cycle, so the sample count 
// received by each slot will be out of sync and the mix will fail. By placing
// pitch filters on each slot, we can guarantee that the sample count will 
// match:
//
// <video1> pitch <video2> pitch composite
//
// Note: the pitch filters must be immediately upstream of composite (or at 
// least downstream of any clip or frame rate or other potential position 
// modifying filters).
#include "precompiled_headers.hpp"
#include "amf_filter_plugin.hpp"
#include "utility.hpp"

namespace amf { namespace openmedialib {

class ML_PLUGIN_DECLSPEC filter_pitch : public ml::filter_type
{
	public:
		// Filter_type overloads
		explicit filter_pitch( const pl::wstring & )
			: ml::filter_type( )
			, prop_fps_num_( pcos::key::from_string( "fps_num" ) )
			, prop_fps_den_( pcos::key::from_string( "fps_den" ) )
			, prop_speed_( pcos::key::from_string( "speed" ) )
			, prop_samples_( pcos::key::from_string( "samples" ) )
		{
			properties( ).append( prop_fps_num_ = 25 );
			properties( ).append( prop_fps_den_ = 1 );
			properties( ).append( prop_speed_ = -1.0 );
			properties( ).append( prop_samples_ = 0 );
		}

		// Indicates if the input will enforce a packet decode
		virtual bool requires_image( ) const { return false; }

		// This provides the name of the plugin (used in serialisation)
		virtual const pl::wstring get_uri( ) const { return L"pitch"; }

	protected:
		// The main access point to the filter
		void do_fetch( ml::frame_type_ptr &result )
		{
			// Acquire values
			acquire_values( );

			// Frame to return
			result = fetch_from_slot( );

			// Save on typing
			double speed = prop_speed_.value< double >( );
			int num = prop_fps_num_.value< int >( );
			int den = prop_fps_den_.value< int >( );

			if ( num == -1 || den == -1 )
				result->get_fps( num, den );

			if ( result && result->get_audio( ) )
			{
				// Handle the frame with audio case
				ml::audio_type_ptr audio = result->get_audio( );

				int samples = audio->samples( );
				int required = samples;
				int frequency = audio->frequency( );
				int position = get_position( );
				int fixed = prop_samples_.value< int >( );

				if ( fixed > 0 )
				{
					result->set_audio( audio_channel_convert( result->get_audio( ), 2 ) );
					required = fixed;
				}
				else if ( speed > 0.0 )
				{
					required = int( samples / speed );
					result->set_fps( int( result->get_fps_num( ) * speed ), result->get_fps_den( ) );
				}
				else
				{
					required = ml::audio_samples_for_frame(	position, frequency, num, den );
					result->set_fps( num, den );
				}

				if ( samples != required )
					change_pitch( result, required );

			}
			else if ( result && speed > 0.0 )
			{
				result->set_fps( int( result->get_fps_num( ) * speed ), result->get_fps_den( ) );
			}
			else if ( result && ( result->get_fps_num( ) != num || result->get_fps_den( ) != den ) )
			{
				result->set_fps( num, den );
			}
		}

		pcos::property prop_fps_num_;
		pcos::property prop_fps_den_;
		pcos::property prop_speed_;
		pcos::property prop_samples_;
};

ml::filter_type_ptr ML_PLUGIN_DECLSPEC create_pitch( const pl::wstring &resource )
{
	return ml::filter_type_ptr( new filter_pitch( resource ) );
}

} }
