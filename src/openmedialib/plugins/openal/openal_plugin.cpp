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

#ifdef WIN32
#include <windows.h>
#endif // WIN32

#include <iostream>
#include <cstdlib>
#include <deque>
#include <string>

#include <boost/thread/recursive_mutex.hpp>
#include <boost/thread/thread.hpp>

#if defined ( WIN32 )
#include <al.h>
#include <alc.h>
#elif defined ( HAVE_OPENALFRAMEWORK )
#include <OpenAL/al.h>
#include <OpenAL/alc.h>
#else
#include <AL/al.h>
#include <AL/alc.h>
#endif

namespace pl = olib::openpluginlib;
namespace pcos = olib::openpluginlib::pcos;
namespace ml = olib::openmedialib::ml;
namespace il = olib::openimagelib::il;

#if !defined( GCC_VERSION ) && defined( __GNUC__ )
#define GCC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
#endif

namespace olib { namespace openmedialib { namespace ml { 

//
// Library initialisation mechanism
//

namespace
{
	static ALCdevice *device = NULL;
	static ALCcontext *context = NULL;

	void reflib( int init )
	{
		boost::recursive_mutex mutex;

		static long refs = 0;

		assert( refs >= 0 && L"openal_plugin::refinit: refs is negative." );
		
		if( init > 0 && ++refs == 1 )
		{
			// Initialise the openal libs
			device = alcOpenDevice( NULL );
			if( alcGetError( device ) == ALC_NO_ERROR )
				context = alcCreateContext( device, NULL );
			if ( context != NULL )
				alcMakeContextCurrent( context );
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
			, buffers_( )
			, source_( 0 )
			, format_( AL_FORMAT_STEREO16 )
			, timer_( )
		{
			properties( ).append( prop_preroll_ = 8 );

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
   			alGetSourcei( source_, AL_SOURCE_STATE, &state );
			if ( state != AL_PLAYING )
				alSourcePlay( source_ );
		}

		void sleep( int microseconds )
		{
			value_type time = { microseconds / 1000000, microseconds % 1000000 };
			timer_.sleep( time );
		}

		void recover( )
		{
			int processed;

			do
			{
				// Determine how many buffers are processed
   				alGetSourcei( source_, AL_BUFFERS_PROCESSED, &processed );

				// Special case - if we have no buffers processed and no more available 
				// for use, then we need to wait until we recover at least one
				if ( processed == 0 && buffers_.size( ) == 0 )
					sleep( 10000 );
			}
			while ( processed == 0 && buffers_.size( ) == 0 );

			// Unqueue the processed buffers and return them to our pool
   			while( processed -- )
   			{
	   			ALuint buffer;
	  			alSourceUnqueueBuffers( source_, 1, &buffer );
				buffers_.push_back( buffer );
   			}
		}

		virtual bool init( )
		{
			if ( context != NULL )
			{
				// Create the requested number of preroll buffers
				for ( size_t i = 0; i < size_t( prop_preroll_.value< int >( ) ); i ++ )
				{
					ALuint buffer;
					alGenBuffers( 1, &buffer );
					buffers_.push_back( buffer );
				}

				// Specify the listener as sitting directly in front of the speakers
				ALfloat listener_pos[ ] = { 2.0, 0.0, -2.0 };
				ALfloat listener_vel[ ] = { 0.0, 0.0, 0.0 };
				ALfloat	listener_ori[ ] = { 0.0, 0.0, -1.0, 0.0, 1.0, 0.0 };

				alListenerfv( AL_POSITION, listener_pos );
				alListenerfv( AL_VELOCITY, listener_vel );
				alListenerfv( AL_ORIENTATION, listener_ori );

				// Initialise the source
				alGenSources( 1, &source_ );
				alSourcei( source_, AL_LOOPING, AL_FALSE );
				alSource3f( source_, AL_POSITION, 0.0, 0.0, 0.0 );
				alSource3f( source_, AL_VELOCITY, 0.0, 0.0, 0.0 );
				alSourcefv( source_, AL_ORIENTATION, listener_ori );
				alSource3f( source_, AL_DIRECTION, 0.0, 0.0, 0.0);
				alSourcef( source_, AL_ROLLOFF_FACTOR, 0.0 );
			}

			return context != NULL;
		}

		virtual bool push( frame_type_ptr frame )
		{
			// Get the audio from the frame
			audio_type_ptr aud = audio::coerce( audio::FORMAT_PCM16, frame->get_audio( ) );

			// Recover any played out buffers
			recover( );

			// If we have audio in this frame then schedule for playout
   			if ( aud != 0 && buffers_.size( ) > 0 )
			{
				// TODO: complete mapping from audio type (combination of af and channels)
				if( aud->channels( ) == 1 )
				{
					format_ = AL_FORMAT_MONO16;
				}
				else
				{
					aud = audio::channel_convert( aud, 2 );
					format_ = AL_FORMAT_STEREO16;
				}

				// Copy from the frame and queue the buffer
#			ifdef SWAB_AUDIO 
				short* buf = ( short* ) aud->data( );
				for( int i = 0; i < aud->size( ) / sizeof( short ); ++i )
					buf[ i ] = ( buf[ i ] >> 8 ) | ( buf[ i ] << 8 );
#			endif
 				alBufferData( *buffers_.begin( ), format_, aud->pointer( ), aud->size( ), aud->frequency( ) );
				alSourceQueueBuffers( source_, 1, &( *buffers_.begin( ) ) );
				buffers_.pop_front( );
			}

			// Make sure the source is playing
			if ( buffers_.size( ) == 0 )
				play( );

			return aud != 0;
		}

		virtual frame_type_ptr flush( )
		{ 
			alSourceStop( source_ );
			recover( );
			return frame_type_ptr( ); 
		}

		virtual void complete( )
		{
			// Force play
			play( );

			// Recover all buffers
   			ALenum state;
   			alGetSourcei( source_, AL_SOURCE_STATE, &state );
			while ( state == AL_PLAYING && buffers_.size( ) < size_t( prop_preroll_.value< int >( ) ) )
			{
				recover( );
	   			alGetSourcei( source_, AL_SOURCE_STATE, &state );
			}
			alSourceStop( source_ );
		}

		virtual bool empty( )
		{
   			ALenum state;
   			alGetSourcei( source_, AL_SOURCE_STATE, &state );
			return state == AL_PLAYING && buffers_.size( ) > 2;
		}

	private:
		pcos::property prop_preroll_;
		std::deque < ALuint > buffers_;
		ALuint source_;
		ALenum format_;

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
	virtual input_type_ptr input(  const pl::wstring & )
	{
		return input_type_ptr( );
	}

	virtual store_type_ptr store( const pl::wstring &, const frame_type_ptr & )
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
