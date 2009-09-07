// avformat - A avformat plugin to ml.

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#include <openmedialib/ml/openmedialib_plugin.hpp>
#include <openpluginlib/pl/pcos/isubject.hpp>
#include <openpluginlib/pl/pcos/observer.hpp>

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

class ML_PLUGIN_DECLSPEC avformat_resampler_filter : public filter_type
{
	public:
		// filter_type overloads
		explicit avformat_resampler_filter(const pl::wstring &)
			: filter_type()
			, prop_output_channels_(pcos::key::from_string("channels"))
			, prop_output_sample_freq_(pcos::key::from_string("frequency"))
			, prop_enable_( pcos::key::from_string( "enable" ) )
			, input_channels_(2)
			, input_sample_freq_(48000)
			, fps_numerator_(25)
			, fps_denominator_(1)
			, context_(NULL)
			, dirty_(true)
			, in_buffer_( )
			, out_buffer_( )
		{
			property_observer_ = boost::shared_ptr<pcos::observer>(new avformat_resampler_filter::property_observer(const_cast<avformat_resampler_filter*>(this)));
		
			properties( ).append( prop_output_channels_ = 2	);
			properties( ).append( prop_output_sample_freq_ = 48000 );
			properties( ).append( prop_enable_ = 1 );
		
			prop_output_channels_.attach( property_observer_ );
			prop_output_sample_freq_.attach( property_observer_ );
		}
	
		virtual ~avformat_resampler_filter()
		{
			if(context_)
				audio_resample_close(context_);
		
			prop_output_channels_.detach(property_observer_);
			prop_output_sample_freq_.detach(property_observer_);
		}
	
		virtual const pl::wstring get_uri( ) const { return L"resampler"; }

		// Indicates if the input will enforce a packet decode
		virtual bool requires_image( ) const { return false; }

		virtual void on_slot_change( ml::input_type_ptr input, int )
		{
			if ( context_ )
			{
				audio_resample_close(context_);
				context_ = NULL;
			}
		}

	protected:
		void do_fetch( frame_type_ptr &current_frame )
		{
			// Cache a local copy of current position
			int current_pos = get_position();
		
			// Ensure the cache is full
			current_frame = fetch_from_slot( 0 );

			if ( prop_enable_.value< int >( ) == 0 || !current_frame )
				return;
		
			ml::audio_type_ptr current_audio = current_frame->get_audio();
			if(!current_audio)
				return;

			// Small hack to allow resampler for channel conversion only
			if(	prop_output_sample_freq_.value<int>( ) == -1 )
			{
				current_audio = audio_channel_convert( current_audio, prop_output_channels_.value<int>( ) );
				if ( current_audio )
					current_frame->set_audio( current_audio );
				return;
			}

			// since ffmpeg resampler will only accept 2 channels or less, force any input with more than 2 channels to conform
			if(current_audio->channels() > 2)
			{
				current_audio = audio_channel_convert(current_audio, 2);
				current_frame->set_audio( current_audio );
			}

			//-----------------------------------------------------------------------------------
			// Check there is a context to work with, if not create one
			//-----------------------------------------------------------------------------------
			if( !context_ )
			{
				current_frame->get_fps(fps_numerator_, fps_denominator_);
				input_channels_		= current_audio->channels();
				input_sample_freq_	= current_audio->frequency();
				dirty_ = true;
			}
			else 
			{
				dirty_ = fps_numerator_ != current_frame->get_fps_num( ) || 
					  fps_denominator_ != current_frame->get_fps_den( ) ||
					  input_channels_ != current_audio->channels( ) ||
					  input_sample_freq_ != current_audio->frequency( );
			}
			
			// If the sample frequencies are the same save some effort
			if(		(input_sample_freq_	== prop_output_sample_freq_.value<int>())
				&&	(input_channels_	== prop_output_channels_.value<int>()) )
				return;
	
			if(dirty_)
			{
				if(context_)
				{
					audio_resample_close(context_);
					context_ = NULL;
				}
				
				context_ = av_audio_resample_init(	prop_output_channels_.value<int>(), 
													input_channels_,
													prop_output_sample_freq_.value<int>(),
													input_sample_freq_,
													SAMPLE_FMT_S16, SAMPLE_FMT_S16,
													16, 10, 0, 0.8 );
				
				if(!context_)
					return;
				
				dirty_ = false;
			}
		
			//----------------------------------------------------------------------------------
			// Prepare a buffer of input data to be passed to ffmpeg's resampler for resampling
			//----------------------------------------------------------------------------------
			int input_samples_per_chan = current_audio->samples();
			
			// Allocate buffer memory
			size_t in_buffer_size = input_channels_ * input_samples_per_chan * sizeof(short);
			if ( in_buffer_.size( ) < in_buffer_size )
				in_buffer_.resize( in_buffer_size );
			
			// Appropriately copy data from cached audio for previous, current & next frames to buffer
			int offset = 0;
			int tmp = 0;
			
			tmp = input_channels_ * current_audio->samples();
			memcpy((void*)(&in_buffer_[ offset ]), (void*)current_audio->data(), tmp * sizeof(short));
			offset += tmp;
			
			// Determine size of output buffer - can be a little bigger than necessary, but can never be too small!
			int estimated_output_samples_per_chan = 0;
			if(input_sample_freq_ < prop_output_sample_freq_.value<int>())
				estimated_output_samples_per_chan = (int)((double(fps_denominator_)/fps_numerator_) * prop_output_sample_freq_.value<int>() * ((current_pos == 0 || current_pos >= get_frames() - 1) ? 2 : 3)) + 1;
			else
				estimated_output_samples_per_chan = (int)((input_samples_per_chan * prop_output_sample_freq_.value<int>() / input_sample_freq_) * (100 + (10 * prop_output_channels_.value<int>()))/100) + 1;
			
			// Allocate the memory
			size_t out_buffer_size = prop_output_channels_.value<int>() * estimated_output_samples_per_chan * sizeof(short);
			if ( out_buffer_.size( ) < out_buffer_size * 2 )
				out_buffer_.resize( out_buffer_size * 2 );
			
			// use ffmpeg to resample data
			int output_samples = audio_resample(context_, &out_buffer_[ 0 ], &in_buffer_[ 0 ], input_samples_per_chan);
			
			// Create a new audio object to hold the determined portion of the output buffer
			audio_type_ptr output_audio = audio_type_ptr(new audio_type(audio<unsigned char, pcm16>(
																			prop_output_sample_freq_.value<int>(), 
																			prop_output_channels_.value<int>(), 
																			output_samples) ) );
			if(!output_audio)
				return;
		
			// Copy data from output buffer to audio object
			memcpy(	(void*)output_audio->data(), 
					(void*)(&out_buffer_[ 0 ]), 
					output_samples * prop_output_channels_.value<int>() * sizeof(short));
	
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
	
		boost::shared_ptr<pcos::observer> property_observer_;
		
		pcos::property	prop_output_channels_;
		pcos::property	prop_output_sample_freq_;
		pcos::property	prop_enable_;
	
		int	input_channels_,
			input_sample_freq_,
			fps_numerator_,
			fps_denominator_;
	
		ReSampleContext* 	context_;
		bool				dirty_;
	
		std::vector< short > in_buffer_;
		std::vector< short > out_buffer_;
};

filter_type_ptr ML_PLUGIN_DECLSPEC create_resampler( const pl::wstring &resource )
{
	return filter_type_ptr( new avformat_resampler_filter( resource ) );
}

} } }
