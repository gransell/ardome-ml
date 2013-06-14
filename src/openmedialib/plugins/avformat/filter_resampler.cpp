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
			, resampler_( 0 )
			, dirty_(true)
			, direction_( 1 )
			, expected_( -1 )
			, latest_processed_position_( -1 )
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
			if ( prop_enable_.value< int >( ) == 0 )
			{
				this->flush_resampler( );
				current_frame = this->fetch_from_slot( 0 );
				return;
			}

			int wanted_position = this->get_position( );

			this->internal_do_fetch( current_frame );

			this->seek( wanted_position );
			this->cleanup_frame_cache( );
		}

	private:

		class cached_audio;


		void internal_do_fetch( frame_type_ptr &current_frame )
		{
			// Cache a local copy of current position
			int current_pos = get_position();

			// Make sure we know the direction of the samples so we can correct as required
			if ( current_pos != expected_ )
			{
				direction_ = expected_ - current_pos == 2 ? -1 : 1;
				this->flush_resampler( );
				dirty_ = true;
			}

			// Next expected frame
			expected_ = current_pos + direction_;

			if ( false == this->fetch_current_frame( current_frame ) )
			{
				this->flush_resampler( );
				return;
			}


			// we don't want any downstream filters corrupting the frame in our cache: shallow copy it
			current_frame = current_frame->shallow( );

			ml::audio_type_ptr current_audio = current_frame->get_audio( );

			// Obtain the requested values
			int out_frequency = prop_output_sample_freq_.value< int >( );
			int out_channels = prop_output_channels_.value< int >( );
			olib::t_string out_af = opencorelib::str_util::to_t_string( prop_af_.value< std::wstring >( ) );

			// Fill in defaults from current frame if necessary
			if ( out_frequency <= 0 ) out_frequency = current_frequency_ <= 0 ? current_audio->frequency( ) : current_frequency_;
			if ( out_channels <= 0 ) out_channels = current_channels_ <= 0 ? current_audio->channels( ) : current_channels_;
			if ( out_af == _CT( "" ) ) out_af = current_af_ == _CT( "" ) ? current_audio->af( ) : current_af_;

			// Store the used values for subsequent reuse. 
			current_frequency_ = out_frequency;
			current_channels_ = out_channels;
			current_af_ = out_af;

			// Drop out now if we have what we want
			if ( out_frequency == current_audio->frequency( ) && 
				 out_channels == current_audio->channels( ) && 
				 out_af == current_audio->af( ) )
			{
				return;
			}

			avformat_resampler_filter::cached_audio audio_out;
			int out_samples = ml::audio::samples_for_frame( current_pos, out_frequency, current_frame->get_fps_num( ), current_frame->get_fps_den( ) );

			audio_out.set_audio( ml::audio::allocate( ml::audio::af_to_id( out_af ), 
													  out_frequency, 
													  out_channels, 
													  out_samples ) );

			audio_out.set_current_position( 0 );

			while ( false == audio_out.filled( ) )
			{
				ml::audio_type_ptr current_audio = this->fetch_next_audio( );

				if ( 0 == current_audio )
					break;

				this->convert_audio( current_audio, audio_out, out_frequency, out_channels, out_af );
				
			}



			if ( false == audio_out.filled( ) )
			{
				this->flush_resampler( &audio_out );
				this->pad_with_zeroes( audio_out );
			}

			ml::audio_type_ptr audio_out_ptr = audio_out.get_audio( );
			audio_out_ptr->set_position( current_frame->get_audio( )->position( ) );
			audio_out_ptr = ml::audio::coerce( ml::audio::af_to_id( out_af ), audio_out_ptr );
			current_frame->set_audio( audio_out_ptr );



		}

		bool fetch_current_frame( frame_type_ptr& a_frame )
		{
			for(frame_list::iterator i = in_frame_cache_.begin(), 
				e = in_frame_cache_.end(); i != e; ++i)
			{
				 if ( (*i)->get_position( ) == this->get_position( ) )
				 {
					 a_frame = *i;
					 return true;
				 }
			}

			a_frame = fetch_from_slot( 0 );

			if ( 0 == a_frame || 0 == a_frame->get_audio( ) )
				return false;

			this->reverse( a_frame );

			in_frame_cache_.push_back( a_frame );

			return true;

		}


		ml::audio_type_ptr fetch_next_audio( )
		{
			int next_position = latest_processed_position_ < 0 ? this->get_position( ) : latest_processed_position_ + direction_;

			if ( 0 > next_position || this->get_frames( ) <= next_position )
			{
				return ml::audio_type_ptr();
			}

			this->seek( next_position );

			frame_type_ptr tmpframe;

			if ( false == this->fetch_current_frame( tmpframe ) )
				return ml::audio_type_ptr();


			return tmpframe->get_audio( );

		}

		void convert_audio( ml::audio_type_ptr& audio_in, avformat_resampler_filter::cached_audio& audio_out, 
							const int& out_freq, const int& out_chan, const olib::t_string& out_af )
		{
			if ( !dirty_ )
			{
				dirty_ =	( resampler_->get_frequency_in( ) != audio_in->frequency( ) ) ||
							( resampler_->get_channels_in( ) != audio_in->channels( ) ) ||
							( resampler_->get_format_in( ) != aml_id_to_AVSampleFormat( audio_in->id( ) ) );
			}

			if ( dirty_ )
			{
				// reset the resampler since input has changed
				// first flush the resampler
				this->flush_resampler( &audio_out );

				resampler_.reset(new avaudio_resampler( audio_in->frequency( ), 
														out_freq, 
														audio_in->channels( ), 
														out_chan, 
														avaudio_resampler::channels_to_layout( audio_in->channels( ) ),
														avaudio_resampler::channels_to_layout( out_chan ),
														aml_id_to_AVSampleFormat( audio_in->id( ) ), 
														aml_id_to_AVSampleFormat( ml::audio::af_to_id( out_af ) ) ) );

				dirty_ = false;
			}



			int out_offset = audio_out.get_byte_offset( );

			
			int resampled_count = resampler_->resample( (boost::uint8_t*) audio_out.get_audio( )->pointer( ) + out_offset, 
														audio_out.get_audio( )->samples( ) - audio_out.get_current_position( ), 
														(boost::uint8_t*) audio_in->pointer( ),
														audio_in->samples( ) );


			latest_processed_position_ = this->get_position();

			audio_out.set_current_position( audio_out.get_current_position( ) + resampled_count );

		}

		void flush_resampler( avformat_resampler_filter::cached_audio* audio = 0 )
		{
			latest_processed_position_ = -1;

			if ( 0 == resampler_ )
			{
				return;
			}

			if ( audio )
			{
				int out_offset = audio->get_byte_offset( );

				int resampled_count = 
					resampler_->drain( (boost::uint8_t*) audio->get_audio( )->pointer( ) + out_offset, 
									   audio->get_audio( )->samples( ) - audio->get_current_position( ) );

				audio->set_current_position( audio->get_current_position( ) + resampled_count );
			}

			// make sure resampler is empty. Flush 10000 bytes at a time
			// FIXME: isn't there a way to know how many samples are still in the resampler?
			while (resampler_->drain( 10000 ) ) {}

		}

		void pad_with_zeroes( avformat_resampler_filter::cached_audio& audio )
		{
			int out_offset = audio.get_byte_offset( );
			memset( static_cast<boost::uint8_t*>( audio.get_audio( )->pointer( ) ) + out_offset, 0, audio.get_audio( )->size( ) - out_offset );
		}

		void cleanup_frame_cache( )
		{
			for(frame_list::iterator i = in_frame_cache_.begin(), 
				e = in_frame_cache_.end(); i != e; )
			{
				if ( (*i)->get_position( ) != expected_ ||
					 (*i)->get_position( ) != this->get_position( ) )
				{
					i = in_frame_cache_.erase( i );
					continue;
				}

				++i;
			}
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

		class cached_audio
		{
			public:

				cached_audio()
					: current_sample_position_(-1)
				{}

				bool init_with_frame( frame_type_ptr& frame )
				{
					if ( 0 == frame->get_audio( ) )
					{
						return false;
					}

					current_audio_ = frame->get_audio( );
					current_sample_position_ = 0;

					return true;
				}

				void set_audio( ml::audio_type_ptr audio_ptr )
				{
					current_audio_ = audio_ptr;
				}

				ml::audio_type_ptr& get_audio( )
				{
					return current_audio_;
				}

				bool filled( ) const
				{
					return ( 0 != current_audio_ ) && ( current_sample_position_ == current_audio_->samples() );
				}

				int get_byte_offset( ) const
				{
					if ( 0 == current_audio_ )
					{
						return 0;
					}

					return current_sample_position_ * current_audio_->channels( ) * current_audio_->sample_storage_size( );
				}

				int get_current_position( ) const
				{
					return current_sample_position_;
				}

				void set_current_position( int new_position )
				{
					current_sample_position_ = new_position;
				}

			private:

				int						current_sample_position_;
				ml::audio_type_ptr		current_audio_;

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
					frame->set_audio( audio );
				}
			}

		}

		boost::shared_ptr<pcos::observer> property_observer_;
		
		pcos::property prop_output_channels_;
		pcos::property prop_output_sample_freq_;
		pcos::property prop_af_;
		pcos::property prop_enable_;
	
		std::wstring af_;

		int current_frequency_;
		int current_channels_;
		olib::t_string current_af_;
	
		boost::scoped_ptr< avaudio_resampler > resampler_;
		bool dirty_;
		int direction_;
		int expected_;
		int latest_processed_position_;

		typedef std::list<frame_type_ptr> frame_list;
		frame_list in_frame_cache_;
};

filter_type_ptr ML_PLUGIN_DECLSPEC create_resampler( const std::wstring &resource )
{
	return filter_type_ptr( new avformat_resampler_filter( resource ) );
}

} } }

