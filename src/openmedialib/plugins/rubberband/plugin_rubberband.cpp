// rubberband - A rubberband plugin to ml.
//
// Copyright (C) 2009 Charles Yates
// Released under the LGPL.

#include <rubberband/RubberBandStretcher.h>

#include <openmedialib/ml/openmedialib_plugin.hpp>
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

class rubber
{
	public:
		typedef float * float_ptr;

		rubber( )
			: rubber_( 0 )
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

		bool start( ml::input_type_ptr input, double speed, double pitch )
		{
			bool result = input;

			if ( result )
			{
				// Obtain a frame
				ml::frame_type_ptr frame = input->fetch( );

				// Check that we have audio
				result = frame && frame->has_audio( );

				if ( result )
				{
					// Need an audio sample
					ml::audio_type_ptr audio = frame->get_audio( );
					size_t frequency = size_t( audio->frequency( ) );
					size_t channels = size_t( audio->channels( ) );

					// Options for realtime
					int options = RubberBandStretcher::OptionProcessRealTime | 
								  RubberBandStretcher::OptionStretchPrecise | 
								  RubberBandStretcher::OptionWindowStandard;

					// Create the rubberband
					rubber_ = new RubberBandStretcher( frequency, channels, options, 1.0 / speed, pitch );

					// Determine the resultant frame rate
					boost::int64_t n = boost::int64_t( boost::int64_t( 10000LL * frame->get_fps_num( ) ) * speed );
					boost::int64_t d = boost::int64_t( 10000LL * frame->get_fps_den( ) );
					ml::remove_gcd( n, d );

					// Needed in order to determine how many samples are expected for each frame
					input_ = input;
					frequency_ = audio->frequency( );
					channels_ = audio->channels( );
					fps_num_ = n;
					fps_den_ = d;

					// Current seek state
					expected_ = 0;
					source_ = 0;
				}
			}

			return result;
		}

		void sync( double speed, double pitch )
		{
		}

		ml::frame_type_ptr fetch( int position )
		{
			// Ensure that we have something to work with
			if ( !started( ) )
				return ml::frame_type_ptr( );

			// Reset the state if necessary
			if ( position != expected_ )
			{
				rubber_->reset( );
				expected_ = position;
				source_ = position;
			}

			// Determine how many samples we need for this frame
			int samples = ml::audio::samples_for_frame( position, frequency_, fps_num_, fps_den_ );

			// Loop until we have the requisite samples
			while( rubber_->available( ) < samples && source_ < input_->get_frames( ) )
			{
				ml::frame_type_ptr frame = input_->fetch( source_ ++ );
				ml::audio_type_ptr audio = frame ? frame->get_audio( ) : ml::audio_type_ptr( );
				ARENFORCE_MSG( audio, "Audio required for rubberband" );
				ml::audio_type_ptr floats = ml::audio::coerce( ml::audio::FORMAT_FLOAT, audio );
				ARENFORCE_MSG( floats->pointer( ), "Audio conversion failed for rubberband" );
				float_ptr *channels = convert( floats );
				rubber_->process( channels, size_t( floats->samples( ) ), source_ == input_->get_frames( ) );
				clean( channels );
			}

			// Create the result frame
			ml::frame_type_ptr result = ml::frame_type_ptr( new frame_type( ) );
			result->set_fps( fps_num_, fps_den_ );
			result->set_pts( position * double( fps_den_ ) / fps_num_ );
			result->set_duration( double( fps_den_ ) / fps_num_ );
			result->set_position( position );

			// Obtain the samples required
			result->set_audio( retrieve( position, samples ) );

			// We expect the next frame to follow...
			expected_ ++;

			return result;
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
				delete channels[ c ];
			delete channels;
		}

		ml::audio_type_ptr retrieve( int position, int samples )
		{
			float_ptr *array = allocate( samples );
			ml::audio_type_ptr output = ml::audio::allocate( L"float", frequency_, channels_, samples );
			rubber_->retrieve( array, samples );
			float_ptr dst = ( float_ptr )output->pointer( );
			for ( int s = 0; s < samples; s ++ )
				for ( int c = 0; c < channels_; c ++ )
					*dst ++ = array[ c ][ s ];
			output->set_position( position );
			return output;
		}

	private:
		ml::input_type_ptr input_;
		RubberBandStretcher *rubber_;
		int fps_num_;
		int fps_den_;
		int frequency_;
		int channels_;
		int expected_;
		int source_;
};

class ML_PLUGIN_DECLSPEC filter_rubberband : public filter_type
{
	public:
		filter_rubberband( )
			: filter_type( )
			, prop_speed_( pl::pcos::key::from_string( "speed" ) )
			, prop_pitch_( pl::pcos::key::from_string( "pitch" ) )
		{
			properties( ).append( prop_speed_ = 1.0 );
			properties( ).append( prop_pitch_ = 1.0 );
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
			result = last_frame_;

			// If we have a duplicated request, return the last frame
			if( last_frame_ && last_frame_->get_position( ) == get_position( ) )
				return;

			// Make sure we have the correct state
			if ( !rubber_.started( ) )
				rubber_.start( fetch_slot( 0 ), prop_speed_.value< double >( ), prop_pitch_.value< double >( ) );
			else
				rubber_.sync( prop_speed_.value< double >( ), prop_pitch_.value< double >( ) );

			// Obtain the current frame
			result = rubber_.fetch( get_position( ) );

			// Hold a reference in case of reuse
			last_frame_ = result;
		}
	
private:
	pl::pcos::property prop_speed_;
	pl::pcos::property prop_pitch_;
	ml::frame_type_ptr last_frame_;
	rubber rubber_;
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

	virtual filter_type_ptr filter( const pl::wstring & )
	{
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
