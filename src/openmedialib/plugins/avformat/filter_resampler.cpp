// avformat - A avformat plugin to ml.
//
// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.
//
// #filter:resampler
//
// Provides an audio resampler based on the libavformat provided resampler.

#include <openmedialib/ml/openmedialib_plugin.hpp>
#include <openmedialib/ml/filter_simple.hpp>
#include <openpluginlib/pl/pcos/isubject.hpp>
#include <openpluginlib/pl/pcos/observer.hpp>

#include "avaudio_base.hpp"

#include <vector>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
}

namespace ml = olib::openmedialib;
namespace pl = olib::openpluginlib;
namespace pcos = olib::openpluginlib::pcos;

namespace olib { namespace openmedialib { namespace ml { 

static pl::pcos::key key_audio_reversed_ =  pl::pcos::key::from_string( "audio_reversed" );

class ML_PLUGIN_DECLSPEC avformat_resampler_filter : public filter_simple
{
	public:
		// filter_type overloads
		explicit avformat_resampler_filter(const std::wstring &)
			: filter_simple()
			, prop_output_channels_(pcos::key::from_string("channels"))
			, prop_output_sample_freq_(pcos::key::from_string("frequency"))
			, prop_enable_( pcos::key::from_string( "enable" ) )
			, input_channels_(2)
			, input_sample_freq_(48000)
			, fps_numerator_(25)
			, fps_denominator_(1)
			, dirty_(true)
			, direction_( 1 )
			, expected_( -1 )
		{
			property_observer_ = boost::shared_ptr<pcos::observer>(new avformat_resampler_filter::property_observer(const_cast<avformat_resampler_filter*>(this)));
		
			properties( ).append( prop_output_channels_ = 2	);
			properties( ).append( prop_output_sample_freq_ = 48000 );
			properties( ).append( prop_enable_ = 1 );
		
			prop_output_channels_.attach( property_observer_ );
			prop_output_sample_freq_.attach( property_observer_ );
		}
	
		virtual const std::wstring get_uri( ) const { return L"resampler"; }

		// Indicates if the input will enforce a packet decode
		virtual bool requires_image( ) const { return false; }

		virtual void on_slot_change( ml::input_type_ptr input, int )
		{
			dirty_ = true;
		}

	protected:
		void do_fetch( frame_type_ptr &current_frame )
		{
			// Cache a local copy of current position
			int current_pos = get_position();

			// Make sure we know the direction of the samples so we can correct as required
			if ( current_pos != expected_ )
				direction_ = expected_ - current_pos == 2 ? -1 : 1;

			// Next expected frame
			expected_ = current_pos + direction_;

			// Ensure the cache is full
			current_frame = fetch_from_slot( 0 );

			if ( prop_enable_.value< int >( ) == 0 || !current_frame )
				return;

			// Ensure the audio samples are in the right direction
			current_frame = current_frame->shallow( );
			reverse( current_frame );

			ml::audio_type_ptr current_audio = current_frame->get_audio( );
			if( !current_audio )
				return;

			int out_frequency = prop_output_sample_freq_.value< int >( );
			int out_channels = prop_output_channels_.value< int >( );
			out_channels = out_channels <= 0 ? current_audio->channels( ) : out_channels;

			// Small hack to allow resampler for channel conversion only
			if(	out_frequency == -1 )
			{
				current_audio = audio::channel_convert( current_audio, out_channels, last_channels_ );
				last_channels_ = current_audio;
				if ( current_audio )
					current_frame->set_audio( current_audio );
				return;
			}

			// since ffmpeg resampler will only accept 8 channels or less, force any input with more than 8 channels to conform
			if( current_audio->channels() > 8 )
			{
				current_audio = audio::channel_convert(current_audio, 8, last_channels_);
				last_channels_ = current_audio;
				current_frame->set_audio( current_audio );
			}

			// Check if changes have occurred in the input
			if ( !dirty_ )
			{
				dirty_ = fps_numerator_ != current_frame->get_fps_num( ) || 
					     fps_denominator_ != current_frame->get_fps_den( ) ||
					     input_channels_ != current_audio->channels( ) ||
					     input_sample_freq_ != current_audio->frequency( );
			}

			if ( dirty_ )
			{
				current_frame->get_fps( fps_numerator_, fps_denominator_ );
				input_channels_	 = current_audio->channels( );
				input_sample_freq_ = current_audio->frequency( );
				filter_.set_passthrough( current_audio );
				filter_.set_channels( out_channels );
				filter_.set_frequency( out_frequency );
				dirty_ = false;
			}

			ml::audio_type_ptr output_audio = filter_.convert( current_audio );
			output_audio->set_position( current_audio->position( ) );

			// Set current frame to have this new audio object, which now holds the resampled audio data
			current_frame->set_audio(output_audio);
		}
	
	private:
		class property_observer : public pcos::observer
		{
		public:
			property_observer(avformat_resampler_filter* filter)
				: filter_(filter)
			{
			}
	
			void updated(pcos::isubject*)
			{
				filter_->dirty_ = true;
			}
	
			private:
				avformat_resampler_filter* filter_;
		};

		// Handles the audio direction
		void reverse( ml::frame_type_ptr &frame )
		{
			ml::audio_type_ptr audio = frame->get_audio( );

			if ( audio )
			{
				// Make sure we have a propery on the frame
				if ( !frame->property_with_key( key_audio_reversed_ ).valid( ) )
					frame->properties( ).append( pl::pcos::property( key_audio_reversed_ ) = 0 );

				// Obtain the reverse audio property
				pl::pcos::property property = frame->property_with_key( key_audio_reversed_ );

				if ( ( direction_ < 0 && property.value< int >( ) == 0 ) || ( direction_ > 0 && property.value< int >( ) == 1 ) )
				{
					audio = ml::audio::reverse( audio );
					property = ( int )( property.value< int >( ) ? 0 : 1 );
				}
			}

			frame->set_audio( audio );
		}

		boost::shared_ptr<pcos::observer> property_observer_;
		
		pcos::property prop_output_channels_;
		pcos::property prop_output_sample_freq_;
		pcos::property prop_enable_;
	
		int	input_channels_;
		int input_sample_freq_;
		int fps_numerator_;
		int fps_denominator_;
	
		avaudio_filter filter_;
		bool dirty_;
		audio_type_ptr last_channels_;
		int direction_;
		int expected_;
};

filter_type_ptr ML_PLUGIN_DECLSPEC create_resampler( const std::wstring &resource )
{
	return filter_type_ptr( new avformat_resampler_filter( resource ) );
}

} } }

