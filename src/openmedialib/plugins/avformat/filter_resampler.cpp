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

#include "avaudio_convert.hpp"
#include "utils.hpp"

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
			, prop_af_(pcos::key::from_string("af"))
			, prop_enable_( pcos::key::from_string( "enable" ) )
			, current_frequency_(-1)
			, current_channels_(-1)
			, filter_( 0 )
			, dirty_(true)
			, direction_( 1 )
			, expected_( -1 )
		{
			property_observer_ = boost::shared_ptr<pcos::observer>(new avformat_resampler_filter::property_observer(const_cast<avformat_resampler_filter*>(this)));
		
			properties( ).append( prop_output_channels_ = -1 );
			properties( ).append( prop_output_sample_freq_ = 48000 );
			properties( ).append( prop_af_ = std::wstring( L"" ) );
			properties( ).append( prop_enable_ = 1 );
		
			prop_output_channels_.attach( property_observer_ );
			prop_output_sample_freq_.attach( property_observer_ );
			prop_af_.attach( property_observer_ );
		}
		
		virtual ~avformat_resampler_filter()
		{
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

			// Obtain the requested values
			int out_frequency = prop_output_sample_freq_.value< int >( );
			int out_channels = prop_output_channels_.value< int >( );
			std::wstring out_af = prop_af_.value< std::wstring >( );

			// Fill in defaults from current frame if necessary
			if ( out_frequency <= 0 ) out_frequency = current_frequency_ <= 0 ? current_audio->frequency( ) : current_frequency_;
			if ( out_channels <= 0 ) out_channels = current_channels_ <= 0 ? current_audio->channels( ) : current_channels_;
			if ( out_af == L"" ) out_af = current_af_ == L"" ? opencorelib::str_util::to_wstring( current_audio->af( ) ) : current_af_;

			// Store the used values for subsequent reuse
			current_frequency_ = out_frequency;
			current_channels_ = out_channels;
			current_af_ = out_af;

			// FIXME: Hack to provide a uniform output if frequency is unchanged (should be handled by swresample)
			if ( out_channels != current_audio->channels( ) )
			{
				current_audio = audio::channel_convert(current_audio, out_channels, last_channels_);
				current_frame->set_audio( current_audio );
			}

			// Drop out now if we have what we want
			if ( out_frequency == current_audio->frequency( ) && 
				 out_channels == current_audio->channels( ) && 
				 out_af == opencorelib::str_util::to_wstring( current_audio->af( ) ) )
			{
				last_channels_ = current_audio;
				return;
			}

			// since ffmpeg resampler will only accept 8 channels or less, force any input with more than 8 channels to conform
			if( current_audio->channels() > 8 && out_channels > 8 )
			{
				current_audio = audio::channel_convert(current_audio, 8, last_channels_);
				current_channels_ = 8;
				out_channels = out_channels <= 8 ? out_channels : 8;
			}

			// Check if changes have occurred in the input
			if ( !dirty_ )
			{
				dirty_ =	( filter_->get_in_frequency( ) != current_audio->frequency( ) ) ||
							( filter_->get_in_channels( ) != current_audio->channels( ) ) ||
							( filter_->get_in_format( ) != aml_id_to_AVSampleFormat( current_audio->id( ) ) );
			}

			if ( dirty_ )
			{
				filter_.reset(new avaudio_convert_to_aml( current_audio->frequency( ), out_frequency, current_audio->channels( ), out_channels, aml_id_to_AVSampleFormat( current_audio->id( ) ), ml::audio::af_to_id( opencorelib::str_util::to_t_string( out_af ) ) ) );

				dirty_ = false;
			}

			ml::audio_type_ptr output_audio = filter_->convert( current_audio );
			output_audio->set_position( current_audio->position( ) );

			// Set current frame to have this new audio object, which now holds the resampled audio data
			current_frame->set_audio(output_audio);
			last_channels_ = output_audio;
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
		pcos::property prop_af_;
		pcos::property prop_enable_;
	
		std::wstring af_;

		int current_frequency_;
		int current_channels_;
		std::wstring current_af_;
	
		boost::scoped_ptr< avaudio_convert_to_aml > filter_;
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

