// openal - A openal plugin to ml.
//
// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.
//
// #store:openal:
//
// Provides an audio playout store based on openal.

#ifdef HAVE_CONFIG_H
#include <openlibraries_global_config.hpp>
#endif

#include <openmedialib/ml/openmedialib_plugin.hpp>

#include <openpluginlib/pl/timer.hpp>
#include <opencorelib/cl/assert_defines.hpp>
#include <opencorelib/cl/enforce_defines.hpp>
#include <opencorelib/cl/guard_define.hpp>
#include <opencorelib/cl/media_definitions.hpp>

#if defined( WIN32 )
#include <windows.h>
#include <initguid.h> //To get the DirectSound8 related GUID:s without linking statically
#include <DSound.h>

#define DEFAULT_OPENAL_SOFT_DSOUND_DEVICE "DirectSound Default"

//New enums for speaker in later DSound versions. We'll check for these
//as well just to be sure.
#ifndef DSSPEAKER_7POINT1_SURROUND
	#define DSSPEAKER_7POINT1_SURROUND 0x00000008
#endif
#ifndef DSSPEAKER_5POINT1_SURROUND
	#define DSSPEAKER_5POINT1_SURROUND 0x00000009
#endif

#include <opencorelib/cl/assert_defines.hpp>
#endif //defined( WIN32 )

#include <iostream>
#include <cstdlib>
#include <deque>
#include <string>
#include <set>

#include <boost/thread/recursive_mutex.hpp>
#include <boost/thread/thread.hpp>
#include <boost/foreach.hpp>

#if defined ( WIN32 )
#include <al.h>
#include <alc.h>
#elif defined ( HAVE_OPENALFRAMEWORK )
#include <OpenAL/al.h>
#include <OpenAL/alc.h>
#else
#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alext.h>
#endif

namespace cl = olib::opencorelib;
namespace pl = olib::openpluginlib;
namespace pcos = olib::openpluginlib::pcos;
namespace ml = olib::openmedialib::ml;


#if !defined( GCC_VERSION ) && defined( __GNUC__ )
#define GCC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
#endif

//Error checking macro for OpenAL alSomething functions.
#define AL_ENFORCE( expr ) \
	do { \
		(expr); \
		ALenum result = alGetError(); \
		ARENFORCE_MSG( result == AL_NO_ERROR, "OpenAL command \"%1%\" failed with code 0x%|2$x|" )( #expr )( result ); \
	} while(0);

//Error checking macro for OpenAL alcSomething functions.
#define ALC_ENFORCE( expr ) \
	do { \
	(expr); \
	ALCenum result = alcGetError( device ); \
	ARENFORCE_MSG( result == ALC_NO_ERROR, "OpenAL command \"%1%\" failed with code 0x%|2$x|" )( #expr )( result ); \
	} while(0);


namespace olib { namespace openmedialib { namespace ml { 

//
// Library initialisation mechanism
//

namespace
{
	static ALCdevice *device = NULL;
	static ALCcontext *context = NULL;
	static boost::recursive_mutex openal_mutex;

	using std::ios;

	void reflib( int init )
	{
		boost::recursive_mutex::scoped_lock lock( openal_mutex );

		static long refs = 0;

		assert( refs >= 0 && L"openal_plugin::refinit: refs is negative." );
		
		if( init > 0 && ++refs == 1 )
		{
			// Initialise the openal libs

			try
			{
#if defined( WIN32 )
				//If on Windows, try with the OpenAL soft DirectSound wrapper first.
				//If this succeeds, we're connected to the default Windows sound device.
				try
				{
					ALC_ENFORCE( device = alcOpenDevice( DEFAULT_OPENAL_SOFT_DSOUND_DEVICE ) );
				}
				catch( cl::base_exception & )
				{
					device = NULL;
				}

				if( device )
				{
					ARLOG_INFO( "Succeeded opening OpenAL with DirectSound backend" );
				}
				else
				{
					//Open the default OpenAL device. We won't know what
					//capabilities it has.
					ARLOG_WARN( "Failed opening OpenAL with DSound backend. Will fall back to default OpenAL device. " );

					if( IsDebuggerPresent() )
					{
						//Some Creative OpenAL drivers contain anti-debugging code and will not
						//load properly if a debugger is detected. We throw up a warning about
						//it to avoid developer confusion.
						//The ARASSERT_MSG macro will only show when _DEBUG is defined, but
						//this behaviour occurs in release builds as well, so we build an assert
						//"manually".
						olib::opencorelib::invoke_assert::make_assert()
							.add_context(__FILE__, __LINE__, OLIB_CURRENT_FUNC_NAME, "IsDebuggerPresent()" )
							.msg( _CT("A debugger is attached while running with hardware OpenAL sound output.\n")
							_CT("Certain audio drivers intentionally fail to load if they detect a debugger.\n")
							_CT("You may safely ignore this warning, but expected hardware audio acceleration features may not be available.") );
					}

					ALC_ENFORCE( device = alcOpenDevice( NULL ) );
				}
#else
				ALC_ENFORCE( device = alcOpenDevice( NULL ) );
#endif

				ALC_ENFORCE( context = alcCreateContext( device, NULL ) );
				ALC_ENFORCE( alcMakeContextCurrent( context ) );
			}
			catch( cl::base_exception & )
			{
				ARLOG_ERR( "OpenAL device creation failed. Sound will not be available." );
			}
		}
		else if( init < 0 && --refs == 0 )
		{
			// Uninitialise
			if ( context != NULL )
			{
				alcDestroyContext( context );
				alcCloseDevice( device );
			}
		}
	}
}

class ML_PLUGIN_DECLSPEC openal_store : public store_type
{
	public:
		openal_store( ) 
			: store_type( )
			, prop_preroll_( pcos::key::from_string( "preroll" ) )
			, prop_supported_listen_modes_( pcos::key::from_string( "supported_listen_modes" ) )
			, buffers_( )
			, source_( 0 )
			, format_( AL_FORMAT_STEREO16 )
			, last_recover_failed_( false )
			, timer_( )
		{
			properties( ).append( prop_preroll_ = 2 );
			properties( ).append( prop_supported_listen_modes_ = std::vector<int>() );

			timer_.reset( );

			// Initialise
			reflib( 1 );
		}

		virtual ~openal_store( )
		{
			if ( context != NULL )
			{
				// Clean up all the buffers
				flush( );
				while( buffers_.size( ) )
				{
					alDeleteBuffers( 1, &( *buffers_.begin( ) ) );
					buffers_.pop_front( );
				}
				alDeleteSources( 1, &source_ );
			}

			// Uninitialise
			reflib( -1 );
		}

		void play( )
		{
   			ALenum state;
   			AL_ENFORCE( alGetSourcei( source_, AL_SOURCE_STATE, &state ) );
			if ( state != AL_PLAYING )
				AL_ENFORCE( alSourcePlay( source_ ) );
		}

		void sleep( int microseconds )
		{
			value_type time = { microseconds / 1000000, microseconds % 1000000 };
			timer_.sleep( time );
		}

		//wait_ms: The maximum number of milliseconds to wait for buffers to become available.
		//Return value: true if there is at least one buffer available in buffers_, false
		//				otherwise.
		bool recover( int wait_ms )
		{
			ARASSERT( context );
			ARASSERT( device );
			ARASSERT( source_ );

			int processed = 0;
			int num_waits = 0;

			do
			{
				// Determine how many buffers are processed
   				AL_ENFORCE( alGetSourcei( source_, AL_BUFFERS_PROCESSED, &processed ) );

				// Special case - if we have no buffers processed and no more available 
				// for use, then we need to wait until we recover at least one
				
				if ( processed == 0 && buffers_.size( ) == 0 )
				{
					//If the last recover call failed, the sound card is probably in
					//a bad state, unplugged, or similar. In that case we don't want
					//to retry at all.
					//If the last call succeeded, we can expect to be able to recover
					//buffers eventually, so we wait the maximum period.
					if( last_recover_failed_ )
					{
						return false;
					}
					else if( num_waits >= wait_ms )
					{
						ARLOG_ERR("OpenAL buffer recovery failed. Sound card unplugged?");
						last_recover_failed_ = true;
						return false;
					}
					else
					{
						sleep( 1000 );
					}
					
					++num_waits;
				}
			}
			while ( processed == 0 && buffers_.size( ) == 0 );

			// Unqueue the processed buffers and return them to our pool
   			while( processed -- )
   			{
	   			ALuint buffer;
	  			AL_ENFORCE( alSourceUnqueueBuffers( source_, 1, &buffer ) );
				buffers_.push_back( buffer );
   			}

			if( last_recover_failed_ )
			{
				ARLOG_INFO("OpenAL buffer recovery succeded after previously failing. Sound card plugged in again?");
				last_recover_failed_ = false;
			}
			return true;
		}

		virtual bool init( )
		{
			if ( context != NULL )
			{
				// Create the requested number of preroll buffers
				for ( size_t i = 0; i < size_t( prop_preroll_.value< int >( ) ); i ++ )
				{
					ALuint buffer;
					AL_ENFORCE( alGenBuffers( 1, &buffer ) );
					buffers_.push_back( buffer );
				}

				// Specify the listener as sitting directly in front of the speakers
				ALfloat listener_pos[ ] = { 2.0, 0.0, -2.0 };
				ALfloat listener_vel[ ] = { 0.0, 0.0, 0.0 };
				ALfloat	listener_ori[ ] = { 0.0, 0.0, -1.0, 0.0, 1.0, 0.0 };

				AL_ENFORCE( alListenerfv( AL_POSITION, listener_pos ) );
				AL_ENFORCE( alListenerfv( AL_VELOCITY, listener_vel ) );
				AL_ENFORCE( alListenerfv( AL_ORIENTATION, listener_ori ) );

				// Initialise the source
				AL_ENFORCE( alGenSources( 1, &source_ ) );
				AL_ENFORCE( alSourcei( source_, AL_LOOPING, AL_FALSE ) );
				AL_ENFORCE( alSource3f( source_, AL_POSITION, 0.0, 0.0, 0.0 ) );
				AL_ENFORCE( alSource3f( source_, AL_VELOCITY, 0.0, 0.0, 0.0 ) );
				AL_ENFORCE( alSource3f( source_, AL_DIRECTION, 0.0, 0.0, 0.0) );
				AL_ENFORCE( alSourcef( source_, AL_ROLLOFF_FACTOR, 0.0 ) );

				AL_ENFORCE( al_format_quad16_ = alGetEnumValue("AL_FORMAT_QUAD16") );
				AL_ENFORCE( al_format_51chn16_ = alGetEnumValue("AL_FORMAT_51CHN16") );
				AL_ENFORCE( al_format_71chn16_ = alGetEnumValue("AL_FORMAT_71CHN16") );

				find_supported_listen_modes();
			}

			return context != NULL;
		}

		virtual bool push( frame_type_ptr frame )
		{
			if( !context )
			{
				//Initialization failed, there's nothing we can do here.
				return false;
			}

			//If there's no audio on the frame, there's nothing to do.
			if( !frame->get_audio() )
			{
				return true;
			}

			boost::int32_t recover_wait_ms = 2 * ( frame->get_audio( )->samples() * 1000 / frame->get_audio( )->frequency() );

			// Recover any played out buffers
			if( !recover( recover_wait_ms ) )
			{
				//We could not recover any buffers, and we have no free ones.
				//The most likely reason for this that the sound card was disabled.
				//Some sound cards disable themselves when the headphones are unplugged,
				//or this could be an unplugged USB sound card.
				//In any case, we want to be able to handle this gracefully.
				return false;
			}

			// If we have audio in this frame then schedule for playout
   			if ( buffers_.size( ) > 0 )
			{
				audio_type_ptr converted_audio;
				ARENFORCE( !supported_channels_.empty() );

				ALenum new_format;
				const int incoming_channels = frame->get_audio( )->channels();
				int output_channels = 0;
				//Choose biggest supported channel setup. If we have more output- than input channels 
				//they will be dropped.
				output_channels = *supported_channels_.rbegin();
#ifdef WIN32
				//Mono does not seem well supported on Windows, so we'll upmix it manually to "stereo" instead
				if( output_channels == 1 )
					output_channels = 2;
#endif

				converted_audio = audio::coerce( audio::FORMAT_PCM16, frame->get_audio( ) );
				if( incoming_channels != output_channels )
				{
					if( output_channels == 2 )
					{
						//Special case, since downconverting to stereo is supported for
						//all input configurations.
						//This will mix every other channel to the left and right speakers.
						converted_audio = audio::channel_convert( converted_audio, 2 );
					}
					else
					{
						//Map the input channels one by one to the output channels. If there
						//are more input channels than outputs, we drop the extra ones.
						audio_type_ptr original = converted_audio;
						const int samples = original->samples( );
						converted_audio = audio::allocate( audio::FORMAT_PCM16, original->frequency( ), output_channels, samples, false );

						int channels_to_copy = std::min< int >( incoming_channels, output_channels );
						const boost::int16_t *src = static_cast< boost::int16_t * >( original->pointer( ) );
						boost::int16_t *dst = static_cast< boost::int16_t * >( converted_audio->pointer( ) );
						for(int i = 0; i < samples; ++i )
						{
							memcpy( dst, src, channels_to_copy * 2 );
							src += incoming_channels;
							dst += output_channels;
						}
					}
				}

				assert( converted_audio );

				switch( output_channels )
				{
					case 1: new_format = AL_FORMAT_MONO16; break;
					case 2: new_format = AL_FORMAT_STEREO16; break;
					case 4: new_format = al_format_quad16_; break;
					case 6: new_format = al_format_51chn16_; break;
					case 8: new_format = al_format_71chn16_; break;
					default:
						ARENFORCE_MSG( false, "Speaker configuration with %1% channels was marked as supported, but no OpenAL buffer format could be found for it!" )
							( output_channels );
				}

				if( new_format != format_ )
				{
					ARLOG_INFO("OpenAL store setting new buffer format. Input channels are %1%, output channels are %2%.")
						( incoming_channels )( output_channels );

					//The audio format has changed. We may not mix
					//buffers with different formats in the play queue, so we
					//stop the playback to clear out all current buffers in the queue.
					AL_ENFORCE( alSourceStop( source_ ) );
					if( !recover( 100 ) )
						return false;

					format_ = new_format;
				}

				// Copy from the frame and queue the buffer
#			ifdef SWAB_AUDIO 
				short* buf = ( short* ) aud->data( );
				for( int i = 0; i < aud->size( ) / sizeof( short ); ++i )
					buf[ i ] = ( buf[ i ] >> 8 ) | ( buf[ i ] << 8 );
#			endif
				
 				AL_ENFORCE( alBufferData( *buffers_.begin( ), format_, converted_audio->pointer( ), converted_audio->size( ), converted_audio->frequency( ) ) );
				AL_ENFORCE( alSourceQueueBuffers( source_, 1, &( *buffers_.begin( ) ) ) );
				buffers_.pop_front( );
			}

			// Make sure the source is playing
			if ( buffers_.size( ) == 0 )
				play( );

			return true;
		}

		virtual frame_type_ptr flush( )
		{ 
			if( context )
			{
				AL_ENFORCE( alSourceStop( source_ ) );
				recover( 100 );
			}

			return frame_type_ptr( );
		}

		virtual void complete( )
		{
			if( !context )
				return;

			// Force play
			play( );

			// Recover all buffers
   			ALenum state;
   			AL_ENFORCE( alGetSourcei( source_, AL_SOURCE_STATE, &state ) );
			while ( state == AL_PLAYING && buffers_.size( ) < size_t( prop_preroll_.value< int >( ) ) )
			{
				recover( 100 );
	   			AL_ENFORCE( alGetSourcei( source_, AL_SOURCE_STATE, &state ) );
			}
			AL_ENFORCE( alSourceStop( source_ ) );
		}

		virtual bool empty( )
		{
			if( !context )
				return true;

   			ALenum state;
   			AL_ENFORCE( alGetSourcei( source_, AL_SOURCE_STATE, &state ) );
			return state == AL_PLAYING && buffers_.size( ) > 2;
		}


	private:
		
#if defined( WIN32 )
		//Helper functions to cleanup COM related objects
		static void release( IDirectSound8 **ds8 )
		{
			if( *ds8 )
			{
				(*ds8)->Release( );
				*ds8 = 0;
			}
		}

		static void co_uninitialize()
		{
			CoUninitialize();
		}
#endif

		void find_supported_listen_modes()
		{
			std::vector<int> supported_listen_modes;

#if defined( WIN32 )
			//Did we succeed in opening the DirectSound backend device?
			const std::string device_name( alcGetString( device, ALC_DEVICE_SPECIFIER ) );
			if( device_name == DEFAULT_OPENAL_SOFT_DSOUND_DEVICE )
			{
				try
				{
					//We succeeded in opening the DirectSound backend device, so we should be able to open
					//it through DirectSound manually as well in order to check it for speaker config.
					LPDIRECTSOUND8 dsound8 = NULL;

					CoInitialize( NULL );
					ARENFORCE_HR( CoCreateInstance( CLSID_DirectSound8, NULL, CLSCTX_ALL, IID_IDirectSound8, (void**)(&dsound8) ) );
					ARGUARD( boost::bind( &release, &dsound8 ) );
					ARGUARD( boost::bind( &co_uninitialize ) );
					ARENFORCE( dsound8 != NULL );

					ARLOG_INFO( "Succeeded opening the DirectSound8 COM interface" );

					//Open the primary DirectSound device, which should be the same as
					//the one OpenAL opened.
					ARENFORCE( dsound8->Initialize( NULL ) == DS_OK );

					DWORD speakerConfig = 0;
					ARENFORCE_HR( dsound8->GetSpeakerConfig( &speakerConfig ) );
					//Mask away speaker geometry info
					speakerConfig = DSSPEAKER_CONFIG(speakerConfig);

					std::string speakerMode;
					switch( speakerConfig )
					{
					case DSSPEAKER_MONO:
						speakerMode = "Mono";
						supported_listen_modes.push_back( cl::listen_mode::mono );
						break;
					case DSSPEAKER_HEADPHONE:
					case DSSPEAKER_STEREO:
						speakerMode = "Stereo";
						supported_listen_modes.push_back( cl::listen_mode::mono );
						supported_listen_modes.push_back( cl::listen_mode::stereo_2_0 );
						break;
					case DSSPEAKER_QUAD:
						speakerMode = "Quad";
						supported_listen_modes.push_back( cl::listen_mode::mono );
						supported_listen_modes.push_back( cl::listen_mode::stereo_2_0 );
						supported_listen_modes.push_back( cl::listen_mode::quad );
						break;
					case DSSPEAKER_SURROUND:
					case DSSPEAKER_5POINT1:
					case DSSPEAKER_5POINT1_SURROUND:
						speakerMode = "Surround 5.1";
						supported_listen_modes.push_back( cl::listen_mode::mono );
						supported_listen_modes.push_back( cl::listen_mode::stereo_2_0 );
						supported_listen_modes.push_back( cl::listen_mode::surround_5_1 );
						break;
					case DSSPEAKER_7POINT1:
					case DSSPEAKER_7POINT1_SURROUND:
						speakerMode = "Surround 7.1";
						supported_listen_modes.push_back( cl::listen_mode::mono );
						supported_listen_modes.push_back( cl::listen_mode::stereo_2_0 );
						supported_listen_modes.push_back( cl::listen_mode::surround_7_1 );
						break;
					default:
						speakerMode = "Unknown";
						break;
					}

					ARLOG_INFO( "DirectSound reports speaker mode as %1% (Code: 0x%|2$x|" )( speakerMode )( speakerConfig );
				}
				catch( cl::base_exception & )
				{
					ARLOG_WARN( "Could not open the DirectSound8 COM interface, even though the OpenAL DirectSound backend worked." );
				}
			}
#endif
			if( supported_listen_modes.empty() )
			{
				//If the DirectSound probe failed, or we're on a non-Windows platform we end up here.
				//TODO: Implement checks for Linux and OSX.
				ARLOG_WARN( "Unable to query for sound card speaker setup; multichannel audio may be downmixed to stereo." );

				//Since we couldn't check for supported listen modes, we'll hope that 5.1
				//is available. If not, the usual result is that it gets downmixed to
				//stereo anyway, so no harm done.
				supported_listen_modes.push_back( (int)cl::listen_mode::mono );
				supported_listen_modes.push_back( (int)cl::listen_mode::stereo_2_0 );

#ifndef OLIB_ON_MAC 
				// 5.1 audio not supported by OpenAL on OS X
				supported_listen_modes.push_back( (int)cl::listen_mode::surround_5_1 );
#endif
			}

			ARLOG_INFO( "Supported listening modes are: " );
			BOOST_FOREACH( int lm, supported_listen_modes )
			{
				ARLOG_INFO( "  * %1%" )( cl::listen_mode::to_string( (cl::listen_mode::type)lm ) );
				supported_channels_.insert( cl::listen_mode::num_channels( (cl::listen_mode::type)lm ) );
			}

			prop_supported_listen_modes_ = supported_listen_modes;
		}

	private:
		pcos::property prop_preroll_;
		pcos::property prop_supported_listen_modes_;
		std::set< int > supported_channels_;
		std::deque < ALuint > buffers_;
		ALuint source_;
		ALenum format_;
		ALenum al_format_quad16_;
		ALenum al_format_51chn16_;
		ALenum al_format_71chn16_;

		bool last_recover_failed_;

#if defined WIN32 || ( GCC_VERSION >= 40000 && !defined __APPLE__ ) // GC - shouldn't this Apple thing be PPC?
		typedef pl::rdtsc_default_timer default_timer;
#else
		typedef pl::gettimeofday_default_timer default_timer;
#endif
		typedef default_timer::value_type value_type;
		default_timer timer_;
};


//
// Plugin object
//

class ML_PLUGIN_DECLSPEC openal_plugin : public openmedialib_plugin
{
public:
	virtual input_type_ptr input(  const std::wstring & )
	{
		return input_type_ptr( );
	}

	virtual store_type_ptr store( const std::wstring &, const frame_type_ptr & )
	{
		return store_type_ptr( new openal_store( ) );
	}
};

} } }

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
		*plug = new ml::openal_plugin;
		return true;
	}
	
	ML_PLUGIN_DECLSPEC void openplugin_destroy_plugin( pl::openplugin* plug )
	{ 
		delete static_cast< ml::openal_plugin * >( plug ); 
	}
}
