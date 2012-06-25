// rubberband - A rubberband plugin to ml.
//
// Copyright (C) 2009 Charles Yates
// Released under the LGPL.

#include <stddef.h>
#include <rubberband/RubberBandStretcher.h>

#include <openmedialib/ml/openmedialib_plugin.hpp>
#include <openmedialib/ml/scope_handler.hpp>
#include <openpluginlib/pl/pcos/observer.hpp>

#include <opencorelib/cl/log_defines.hpp>
#include <opencorelib/cl/enforce_defines.hpp>

#include <boost/algorithm/string.hpp>

#include <vector>

namespace pl = olib::openpluginlib;
namespace ml = olib::openmedialib::ml;
namespace il = olib::openimagelib::il;
namespace pcos = olib::openpluginlib::pcos;

using namespace RubberBand;

namespace olib { namespace openmedialib { namespace ml { namespace rubberband {

static pl::pcos::key key_audio_reversed_ =  pl::pcos::key::from_string( "audio_reversed" );

class rubber
{
	public:
		typedef float * float_ptr;

		rubber( )
			: rubber_( 0 )
			, source_fps_num_( -1 )
			, source_fps_den_( -1 )
			, fps_num_( -1 )
			, fps_den_( -1 )
			, frequency_( -1 )
			, channels_( -1 )
			, expected_( -1 )
			, source_( -1 )
			, increment_( 1 )
			, cache_( ml::the_scope_handler::Instance().lru_cache( L"rubberband" ) )
			, old_speed_( 1.0 )
			, last_position_( -1 )
		{
		}

		~rubber( )
		{
			delete rubber_;
		}

		const bool started( ) const 
		{
			return rubber_;
		}

		bool start( ml::input_type_ptr input, ml::frame_type_ptr frame, double speed, double pitch )
		{
			bool result = input && frame;

			if ( result )
			{
				input_ = input;

				// Check that we have audio
				result = frame->has_audio( );

				if ( result )
				{
					delete rubber_;
					rubber_ = 0;

					// Need an audio sample
					ml::audio_type_ptr audio = frame->get_audio( );
					size_t frequency = size_t( audio->frequency( ) );
					size_t channels = size_t( audio->channels( ) );

					// Options for realtime
					int options = RubberBandStretcher::OptionProcessRealTime | 
								  RubberBandStretcher::OptionPitchHighConsistency | 
								  RubberBandStretcher::OptionFormantPreserved | 
								  RubberBandStretcher::OptionWindowLong | 
								  RubberBandStretcher::OptionPhaseLaminar | 
								  RubberBandStretcher::OptionDetectorSoft | 
								  RubberBandStretcher::OptionTransientsMixed | 
								  RubberBandStretcher::OptionStretchPrecise;

					// Create the rubberband
					rubber_ = new RubberBandStretcher( frequency, channels, options, 1.0, 1.0 );

					// Needed in order to determine how many samples are expected for each frame
					frequency_ = audio->frequency( );
					channels_ = audio->channels( );
				}

				// Force an update of the position
				expected_ = -1;

				// Store the source frame rate
				source_fps_num_ = frame->get_fps_num( );
				source_fps_den_ = frame->get_fps_den( );

				sync( speed, pitch, true );

			}

			if ( !pusher_ || !lowpass_filter_ )
			{
				pusher_         = ml::create_input(  L"pusher:" );
				lowpass_filter_ = ml::create_filter( L"lowpass" );

				pusher_->init( );
				lowpass_filter_->init( );
				lowpass_filter_->connect( pusher_, 0 );
			}

			return result;
		}

		// Sync with changes to the speed and pitch
		void sync( double speed, double pitch, bool force = false )
		{
			if ( rubber_ && ( force || 1.0 / speed != rubber_->getTimeRatio( ) || pitch != rubber_->getPitchScale( ) ) )
			{
				try
				{
					// Sync rubberband state
					rubber_->setTimeRatio( 1.0 / speed );
					rubber_->setPitchScale( pitch );
					rubber_->reset( );
					ARLOG_DEBUG7( "Changed rubber to speed = %1%, pitch = %2%, force = %3%" )( speed )( pitch )( force );
				}
				catch( const std::exception& e )
				{
					ARLOG_ERR( "Failed to sync rubberband. what = %4%, speed = %1%, pitch = %2%, force = %3%" )
						( speed )( pitch )( force )( e.what( ) );
					throw;
				}

				// Sync source to expected frame
				source_ = expected_;
			}

			// Determine the resultant frame rate
			boost::int64_t n = boost::int64_t( boost::int64_t( 10000LL * source_fps_num_ ) * speed );
			boost::int64_t d = boost::int64_t( 10000LL * source_fps_den_ );
			ml::remove_gcd( n, d );
			fps_num_ = n;
			fps_den_ = d;
		}

		lru_cache_type::key_type lru_key_for_position( boost::int32_t pos )
		{
			lru_cache_type::key_type my_key( pos, L"rubberband" );
			//if( !scope_uri_key_.empty( ) ) my_key.second = scope_uri_key_;
			return my_key;
		}

		ml::frame_type_ptr fetch( ml::input_type_ptr input, int position, int total_frames, double speed, double pitch, int lowpass_ )
		{
			ml::frame_type_ptr result;

			if ( !input ) return result;

			if( speed <= 0.0 )
			{
				input->seek( position );
				return input->fetch( );
			}

			// Make sure we have the correct state
			if ( input != input_ || !started( ) )
			{
				input->seek( position );
				result = input->fetch( );
				start( input, result, speed, pitch );
			}
			else
			{
				sync( speed, pitch );
			}

			// Reset the state if necessary
			if ( position != expected_ || speed != old_speed_ )
			{
				increment_ = expected_ - position == 2 ? -1 : 1;
				if ( rubber_ ) rubber_->reset( );
				expected_ = position;
				source_ = position;
			}

			int difference = abs( position - last_position_ );

			if ( ( result && !result->get_audio( ) ) || speed > 2.0 || ( last_position_ != -1 && difference > 1 ) )
			{
				if ( !result )
				{
					input_->seek( position );
					result = input_->fetch( );
				}
				ARENFORCE_MSG( result, "Frame required for rubberband %d" )( position );
				result = result->shallow( );
				if ( result->get_audio( ) )
				{
					int samples = result->get_audio( )->samples( );
					int required = int( samples / speed );
					ml::audio_type_ptr audio = reverse_audio( result, increment_ );
					audio->set_position( result->get_position( ) );
					result->set_audio( ml::audio::pitch( audio, required ) );
				}
				result->set_fps( int( fps_num_ ), int( fps_den_ ) );
				result->set_pts( position * double( fps_den_ ) / fps_num_ );
				result->set_duration( double( fps_den_ ) / fps_num_ );
				result->set_position( position );
			}
			else if ( speed != 1.0 )
			{
				// Determine how many samples we need for this frame
				int samples = ml::audio::samples_for_frame( position, frequency_, fps_num_, fps_den_ );

				// Loop until we have the requisite samples
				while( rubber_->available( ) < samples && source_ < total_frames )
				{
					ml::frame_type_ptr frame = result ? result : input_->fetch( source_ );
					result = ml::frame_type_ptr( );
					ARENFORCE_MSG( frame, "Frame required for rubberband %d" )( source_ );
					if ( !frame->get_audio( ) )
						frame->set_audio( ml::audio::allocate( ml::audio::FORMAT_FLOAT, frequency_, channels_, ml::audio::samples_for_frame( source_, frequency_, frame->get_fps_num( ), frame->get_fps_den( ) ) ) );
					ml::audio_type_ptr audio = frame->get_audio( );
					ARLOG_DEBUG7( "Caching frame %d of %d" )( source_ )( total_frames );
					cache_->insert_frame_for_position( lru_key_for_position( source_ ), frame );
					frame = frame->shallow( );
					source_ += increment_;
					audio = reverse_audio( frame, increment_ );
					ml::audio_type_ptr floats = ml::audio::coerce( ml::audio::FORMAT_FLOAT, audio );
					ARENFORCE_MSG( floats->pointer( ), "Audio conversion failed for rubberband" );
					float_ptr *channels = convert( floats );
					try
					{
						rubber_->process( channels, size_t( floats->samples( ) ), source_ >= total_frames );
					}
					catch( const std::exception& e )
					{
						ARLOG_ERR( "Failed to process audio through rubberband. what = %3%, position = %1%, samples = %2%" )
							( frame->get_position( ) )( floats->samples( ) )( e.what( ) );
						throw;
					}
					clean( channels );
				}

				// Create the result frame
				result = cache_->frame_for_position( lru_key_for_position( position ) );
				ARENFORCE_MSG( result, "Frame inaccessible from cache %d" )( position );
				result = result->shallow( );
				result->set_fps( fps_num_, fps_den_ );
				result->set_pts( position * double( fps_den_ ) / fps_num_ );
				result->set_duration( double( fps_den_ ) / fps_num_ );
				result->set_position( position );

				// Obtain the samples required
				result->set_audio( retrieve( position, samples ) );
				result->get_audio( )->set_position( result->get_position( ) );
				if (lowpass_ && lowpass_filter_)
				{
				    pusher_->push( result );
				    result = lowpass_filter_->fetch( );
				}
				if ( !result->property_with_key( key_audio_reversed_ ).valid( ) )
					result->properties( ).append( pl::pcos::property( key_audio_reversed_ ) = 0 );
				pl::pcos::property reverse = result->property_with_key( key_audio_reversed_ );
				reverse = increment_ < 0 ? 1 : 0;
			}
			else if ( !result )
			{
				input_->seek( position );
				result = input_->fetch( );
			}

			result = result->shallow( );
			result->set_audio( reverse_audio( result, increment_ ) );

			// We expect the next frame to follow...
			last_position_ = position;
			expected_ += increment_;
			old_speed_ = speed;

			return result;
		}

		ml::audio_type_ptr reverse_audio( ml::frame_type_ptr &frame, int increment_ )
		{
			ml::audio_type_ptr audio = frame->get_audio( );

			if ( !audio ) return audio;

			// Make sure we have a propery on the frame
			if ( !frame->property_with_key( key_audio_reversed_ ).valid( ) )
				frame->properties( ).append( pl::pcos::property( key_audio_reversed_ ) = 0 );

			// Obtain the reverse audio property
			pl::pcos::property reverse = frame->property_with_key( key_audio_reversed_ );

			if ( ( increment_ < 0 && reverse.value< int >( ) == 0 ) || ( increment_ > 0 && reverse.value< int >( ) == 1 ) )
			{
				audio = ml::audio::reverse( audio );
				frame->set_audio( audio );
				reverse = (int)( reverse.value< int >( ) ? 0 : 1 );
			}

			return audio;
		}

		float_ptr *allocate( int samples )
		{
			float_ptr *array = new float_ptr[ channels_ ];
			for ( int c = 0; c < channels_; c ++ )
				array[ c ] = new float[ samples ];
			return array;
		}

		float_ptr *convert( ml::audio_type_ptr audio )
		{
			float_ptr source = ( float_ptr )audio->pointer( );
			int samples = audio->samples( );
			float_ptr *array = allocate( samples );
			for ( int s = 0; s < samples; s ++ )
				for ( int c = 0; c < channels_; c ++ )
					array[ c ][ s ] = *source ++;
			return array;
		}

		void clean( float_ptr *channels )
		{
			for ( int c = 0; c < channels_; c ++ )
				delete [ ] channels[ c ];
			delete [ ] channels;
		}

		ml::audio_type_ptr retrieve( int position, int samples )
		{
			float_ptr *array = allocate( samples );
			ml::audio_type_ptr output = ml::audio::allocate( L"float", frequency_, channels_, samples );
			try
			{
				rubber_->retrieve( array, samples );	
			}
			catch( const std::exception& e )
			{
				ARLOG_ERR( "Failed to retrieve samples from rubberband. what = %4% Wanted samples = %1%, channels = %2%, frequence = %3%" )
					( samples )( channels_ )( frequency_ )( e.what( ) );
				throw;
			}
			float_ptr dst = ( float_ptr )output->pointer( );
			for ( int s = 0; s < samples; s ++ )
				for ( int c = 0; c < channels_; c ++ )
					*dst ++ = array[ c ][ s ];
			output->set_position( position );
			clean( array );
			return output;
		}

	private:                
                ml::input_type_ptr  pusher_;
                ml::filter_type_ptr lowpass_filter_;

		ml::input_type_ptr input_;
		RubberBandStretcher *rubber_;
		int source_fps_num_;
		int source_fps_den_;
		int fps_num_;
		int fps_den_;
		int frequency_;
		int channels_;
		int expected_;
		int source_;
		int increment_;
		lru_cache_type_ptr cache_;
		double old_speed_;
		int last_position_;
};

class ML_PLUGIN_DECLSPEC filter_rubberband : public filter_simple
{
	public:
		filter_rubberband( )
			: filter_simple( )
			, prop_speed_( pl::pcos::key::from_string( "speed" ) )
			, prop_pitch_( pl::pcos::key::from_string( "pitch" ) )
		        , prop_lowpass_( pl::pcos::key::from_string( "lowpass" ) )
		  {			
			properties( ).append( prop_speed_ = 1.0 );
			properties( ).append( prop_pitch_ = 1.0 );
			properties( ).append( prop_lowpass_ = 0   );
		  }
	
		// Indicates if the input will enforce a packet decode
		bool requires_image( ) const { return false; }

		// Return the filter id
		const pl::wstring get_uri( ) const { return L"rubberband"; }
	
		// Return the number of slots that can be connected to this filter
		const size_t slot_count( ) const { return 1; }

	protected:
		void do_fetch( frame_type_ptr &result )
		{
			// Obtain the current frame
		  result = rubber_.fetch( fetch_slot( ), get_position( ), get_frames( ), prop_speed_.value< double >( ), prop_pitch_.value< double >( ), prop_lowpass_.value< int >( ) );
		}

	private:
		pl::pcos::property prop_speed_;
		pl::pcos::property prop_pitch_;
		pl::pcos::property prop_lowpass_;
		rubber rubber_;
};

class ML_PLUGIN_DECLSPEC filter_pitch : public ml::filter_type
{
	public:
		// Filter_type overloads
		filter_pitch( )
			: ml::filter_type( )
			, filter_( ml::create_filter( L"rubberband" ) )
			, prop_fps_num_( pcos::key::from_string( "fps_num" ) )
			, prop_fps_den_( pcos::key::from_string( "fps_den" ) )
			, prop_lowpass_( pl::pcos::key::from_string( "lowpass" ) )
			, prop_speed_( filter_->property( "speed" ) )
			, prop_samples_( pcos::key::from_string( "samples" ) )
			, total_frames_( 0 )
		{
			properties( ).append( prop_fps_num_ = 25 );
			properties( ).append( prop_fps_den_ = 1 );
			properties( ).append( prop_speed_ = 1.0 );
			properties( ).append( prop_samples_ = 0 );
			properties( ).append( prop_lowpass_ = 1  );
		}

		// Indicates if the input will enforce a packet decode
		virtual bool requires_image( ) const { return false; }

		// This provides the name of the plugin (used in serialisation)
		virtual const pl::wstring get_uri( ) const { return L"pitch"; }

		// Number of frames provided by this filter
		virtual int get_frames( ) const
		{
			return total_frames_;
		}

		virtual void sync( )
		{
			if ( filter_->fetch_slot( ) != fetch_slot( ) )
				filter_->connect( fetch_slot( ) );
			filter_->sync( );
			total_frames_ = filter_->get_frames( );
		}

	protected:
		// The main access point to the filter
		void do_fetch( ml::frame_type_ptr &result )
		{
			// Make sure we're connect the internal filter to our current input even if we won't use it
			if ( filter_->fetch_slot( ) != fetch_slot( ) )
				filter_->connect( fetch_slot( ) );

			filter_->seek( get_position( ) );
			result = filter_->fetch( );
		}

		ml::filter_type_ptr filter_;
		pcos::property prop_fps_num_;
		pcos::property prop_fps_den_;
		pcos::property prop_lowpass_;
		pcos::property prop_speed_;
		pcos::property prop_samples_;
		int total_frames_;
};

//
// Plugin object
//

class ML_PLUGIN_DECLSPEC plugin : public openmedialib_plugin
{
public:
	virtual input_type_ptr input(  const pl::wstring &spec )
	{
		return input_type_ptr( );
	}

	virtual store_type_ptr store( const pl::wstring &name, const frame_type_ptr &frame )
	{
		return store_type_ptr( );
	}

	virtual filter_type_ptr filter( const pl::wstring &spec )
	{
		if ( spec == L"pitch" )
			return filter_type_ptr( new filter_pitch( ) );
		else
			return filter_type_ptr( new filter_rubberband( ) );
	}
};

} } } }

//
// Access methods for openpluginlib
//

extern "C"
{
	ML_PLUGIN_DECLSPEC bool openplugin_init( void )
	{
		return true;
	}

	ML_PLUGIN_DECLSPEC bool openplugin_uninit( void )
	{
		return true;
	}
	
	ML_PLUGIN_DECLSPEC bool openplugin_create_plugin( const char*, pl::openplugin** plug )
	{
		*plug = new ml::rubberband::plugin;
		return true;
	}
	
	ML_PLUGIN_DECLSPEC void openplugin_destroy_plugin( pl::openplugin* plug )
	{ 
		delete static_cast< ml::rubberband::plugin * >( plug ); 
	}
}
