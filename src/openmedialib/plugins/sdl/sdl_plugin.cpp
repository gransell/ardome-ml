// sdl - An sdl plugin to ml.
//
// Copyright (C) 2006 Visual Media FX Ltd.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.
//
// #store:sdl_video:
//
// Provides a store for video playout.
//
// #store:sdl_audio:
//
// Provides a store for audio playout.

#ifdef WIN32
    #pragma warning(push)
    #pragma warning (disable : 4312 )
    #pragma warning (disable : 4355 ) // warning C4355: 'this' : used in base member initializer list
#endif

#include <SDL.h>

#include <openmedialib/ml/openmedialib_plugin.hpp>
#include <openpluginlib/pl/pcos/observer.hpp>

#include <opencorelib/cl/minimal_string_defines.hpp>
#include <opencorelib/cl/boost_link_defines.hpp>
#include <opencorelib/cl/utilities.hpp>

#ifdef WIN32
    #include <windows.h>
    #include <winbase.h>
#endif // WIN32

#include <iostream>
#include <cstdlib>
#include <vector>
#include <string>
#include <fstream>

#ifndef BOOST_THREAD_DYN_DLL
    #define BOOST_THREAD_DYN_DLL
#endif
#include <boost/thread/recursive_mutex.hpp>
#include <boost/thread/condition.hpp>
#include <boost/thread/thread.hpp>
#include <boost/cstdint.hpp>

#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>

namespace pl = olib::openpluginlib;
namespace pcos = olib::openpluginlib::pcos;
namespace ml = olib::openmedialib::ml;
namespace il = olib::openimagelib::il;

namespace olib { namespace openmedialib { namespace ml { 

static int init_count_ = 0;
static bool init_status_ = false;
static bool video_init_ = false;
static bool audio_init_ = false;

static SDL_Overlay *sdl_overlay_ = NULL;
static SDL_Rect sdl_rect_ = SDL_Rect( );
static int default_width_ = 640;
static int default_height_ = 480;
static int default_full_ = 0;

static void sdl_setenv( const std::string name, int value )
{
#ifdef NEED_SDL_GETENV
	boost::format fmt( "%s=%d" );
	SDL_putenv( ( fmt % name % value ).str( ).c_str( ) );
#else
	boost::format fmt( "%d" );
#	ifndef _WIN32
	setenv( name.c_str( ), ( fmt % value ).str( ).c_str( ), 1 );
#	else
	SetEnvironmentVariableA( name.c_str(), ( fmt % value ).str( ).c_str( ) );
#	endif
#endif
}

static bool sdl_init( int subsystem )
{
	if ( init_count_ ++ == 0 )
		init_status_ = SDL_Init( subsystem | SDL_INIT_NOPARACHUTE ) >= 0;
	else
		return SDL_InitSubSystem( subsystem ) >= 0;
	return init_status_;
}

static bool sdl_init_video( )
{
	video_init_ = sdl_init( SDL_INIT_VIDEO );
	return video_init_;
}

static bool sdl_init_audio( )
{
	audio_init_ = sdl_init( SDL_INIT_AUDIO );
	return audio_init_;
}

class ML_PLUGIN_DECLSPEC sdl_video : public store_type
{
	public:
        typedef boost::recursive_mutex::scoped_lock scoped_lock;

		sdl_video( const pl::wstring &, const frame_type_ptr & ) 
			: store_type( )
			, last_sar_num_( 1 )
			, last_sar_den_( 1 )
			, prop_winid_( pcos::key::from_string( "winid" ) )
			, prop_flags_( pcos::key::from_string( "flags" ) )
			, prop_width_( pcos::key::from_string( "width" ) )
			, prop_height_( pcos::key::from_string( "height" ) )
			, prop_keydown_( pcos::key::from_string( "keydown" ) )
			, prop_keymod_( pcos::key::from_string( "keymod" ) )
			, prop_box_( pcos::key::from_string( "box" ) )
			, prop_pf_( pcos::key::from_string( "pf" ) )
			, prop_full_( pcos::key::from_string( "fullscreen" ) )
			, prop_native_( pcos::key::from_string( "native" ) )
			, mutex_( )
		{
			// Allow the specification of a window id
			properties( ).append( prop_winid_ = boost::uint64_t( 0 ) );

			// TODO: Determine capabilities and set flags as appropriate
			properties( ).append( prop_flags_ = SDL_HWSURFACE | SDL_ASYNCBLIT | SDL_HWACCEL | SDL_RESIZABLE | SDL_DOUBLEBUF );

			// Default to european-centric 4:3
			properties( ).append( prop_width_ = default_width_ );
			properties( ).append( prop_height_ = default_height_ );
			properties( ).append( prop_keydown_ = 0 );
			properties( ).append( prop_keymod_ = 0 );
			properties( ).append( prop_box_ = pl::wstring( L"fill" ) );
			properties( ).append( prop_full_ = 0 );
			properties( ).append( prop_native_ = 0.0 );

			// 422 is best for os/x
#ifdef __APPLE__
			properties( ).append( prop_pf_ = pl::wstring( L"yuv422" ) );
#else
			properties( ).append( prop_pf_ = pl::wstring( L"yuv420p" ) );
#endif
		}

		virtual ~sdl_video( )
		{
			// Make sure we turn off full screen now
			if ( prop_full_.value< int >( ) )
			{
				SDL_SetVideoMode( prop_width_.value< int >( ), prop_height_.value< int >( ), 0, prop_flags_.value< int >( ) );
			}

			SDL_FreeYUVOverlay( sdl_overlay_ );
			sdl_overlay_ = 0;
			SDL_FreeSurface( SDL_GetVideoSurface( ) );
		}

		virtual bool init( )
		{
			scoped_lock lock( mutex_ );

			if ( prop_winid_.value< boost::uint64_t >( ) != 0 )
				sdl_setenv( "SDL_WINDOWID", static_cast<int>( prop_winid_.value< boost::uint64_t >( ) ) );

			bool grab_video = sdl_init_video( );

			if ( grab_video )
				grab_video = SDL_SetVideoMode( prop_width_.value< int >( ), prop_height_.value< int >( ), 0, prop_flags_.value< int >( ) ) != NULL;

			return grab_video;
		}

		virtual bool push( frame_type_ptr frame )
		{
			// Check that we have a frame to work with
			if ( frame == 0 && last_frame_ == 0 )
				return false;

			il::image_type_ptr img;

			// Obtain the current image or force previous frame
			if ( frame )
				img = frame->get_image( );
			else
				frame = last_frame_;

			// Use the previously converted image if current frame has no image or we're repeating
			if ( img == 0 )
			{
				img = last_image_;
				frame->set_sar( last_sar_num_, last_sar_den_ );
			}

			// If we still don't have an image, bail now
			if ( img == 0 )
				return false;

			// Check that we have something in the image...
 			if ( img->width( ) * img->height( ) == 0 || img->width( ) < 4 )
			{
				wipe_overlay( sdl_overlay_ );
				return false;
			}

			// Convert to requested colour space
			img = il::convert( img, prop_pf_.value< pl::wstring >( ).c_str( ) );
			last_image_ = img;
			frame->get_sar( last_sar_num_, last_sar_den_ );

			// TODO: Provide an alternative mechanism window event handling (via properties)
			SDL_Event event;
			while ( SDL_PollEvent( &event ) )
			{
				switch( event.type )
				{
					case SDL_VIDEORESIZE:
						prop_width_ = event.resize.w;
						prop_height_ = event.resize.h;
						break;

					case SDL_QUIT:
						return false;

					case SDL_KEYDOWN:
						prop_keymod_ = int( event.key.keysym.mod );
						prop_keydown_ = int( event.key.keysym.sym );
						break;

					case SDL_KEYUP:
						// The mod is always correct in a keyup event, but the key is
						// the key which went up, so we need to reset it
						prop_keymod_ = int( event.key.keysym.mod );
						prop_keydown_ = 0;
						break;

					default:
						break;
				}
			}

			if ( prop_native_.value< double >( ) > 0.0 )
			{
				prop_width_ = int( prop_native_.value< double >( ) * img->width( ) * last_sar_num_ / last_sar_den_ );
				prop_height_ = int( prop_native_.value< double >( ) * img->height( ) );
				prop_native_ = 0.0;
			}

			if ( default_width_ != prop_width_.value< int >( ) || default_height_ != prop_height_.value< int >( ) || default_full_ != prop_full_.value< int >( ) )
			{
				default_width_ = prop_width_.value< int >( );
				default_height_ = prop_height_.value< int >( );
				default_full_ = prop_full_.value< int >( );
				int full_flag = default_full_ ? SDL_FULLSCREEN : 0;

				if ( full_flag )
				{
					SDL_Rect **modes = SDL_ListModes( NULL, prop_flags_.value< int >( ) | full_flag );

					if ( modes )
					{
						int large_w = 0;
						int large_h = 0;

						for ( int i = 0; modes[ i ]; i ++ ) 
						{
							if ( modes[ i ]->w > large_w && modes[ i ]->h > large_h )
							{
								large_w = int( modes[ i ]->w );
								large_h = int( modes[ i ]->h );
							}
						}

						prop_width_ = int( large_w );
						prop_height_ = int( large_h );
					}

					SDL_SetVideoMode( prop_width_.value< int >( ), prop_height_.value< int >( ), 0, prop_flags_.value< int >( ) | full_flag );
				}
				else
				{
					SDL_Surface *screen = SDL_SetVideoMode( prop_width_.value< int >( ), prop_height_.value< int >( ), 0, prop_flags_.value< int >( ) );
					prop_width_ = int( screen->w );
					prop_height_ = int( screen->h );
				}
			}

			lock_display( );

			SDL_Surface *screen = SDL_GetVideoSurface( );
			SDL_Overlay *overlay = fetch_overlay( frame, img );

			if ( overlay != NULL && screen != NULL )
			{
				if ( SDL_LockYUVOverlay( overlay ) >= 0 )
				{
					for ( int plane = 0; plane < overlay->planes; plane ++ )
					{
						uint8_t *dst = overlay->pixels[ plane ];
						int dst_pitch = overlay->pitches[ plane ];
						int w = img->linesize( plane );
						int h = img->height( plane );
						uint8_t *src = img->data( plane );
						int src_pitch = img->pitch( plane );

						// TODO: Fix this work around to correct chroma order 
						if ( plane == 1 )
							dst = overlay->pixels[ 2 ];
						else if ( plane == 2 )
							dst = overlay->pixels[ 1 ];

						while( h -- )
						{
							memcpy( dst, src, w );
							dst += dst_pitch;
							src += src_pitch;
						}
					}

					SDL_UnlockYUVOverlay( overlay );
					SDL_DisplayYUVOverlay( overlay, &screen->clip_rect );
				}

				img->crop_clear( );
			}

			unlock_display( );

			return img != 0;
		}

		virtual void complete( )
		{
		}

		virtual frame_type_ptr flush( )
		{
            scoped_lock lock( mutex_ );
            return flush( lock );
		}

        frame_type_ptr flush( scoped_lock& lck )
        {
            return frame_type_ptr( );
        }

	private:

		// Conditionally lock the display to allow updates
		int lock_display( )
		{
			SDL_Surface *screen = SDL_GetVideoSurface( );
			return screen != NULL && ( !SDL_MUSTLOCK( screen ) || SDL_LockSurface( screen ) >= 0 );
		}

		// Unlock the display
		void unlock_display( )
		{
			SDL_Surface *screen = SDL_GetVideoSurface( );
			if ( screen != NULL && SDL_MUSTLOCK( screen ) )
				SDL_UnlockSurface( screen );
		}

		// Fetch a configured overlay for the the image 
		// TODO: Paramerise colour space
		SDL_Overlay *fetch_overlay( frame_type_ptr frame, il::image_type_ptr &img )
		{
			double this_aspect = double( prop_width_.value< int >( ) ) / double( prop_height_.value< int >( ) );
			int cx = 0;
			int cy = 0;
			int cw = img->width( );
			int ch = img->height( );
			int sn = frame->get_sar_num( );
			int sd = frame->get_sar_den( );
			sn = sn == 0 ? 1 : sn;
			sd = sd == 0 ? 1 : sd;

			if ( prop_box_.value< pl::wstring >( ) == L"letter" )
			{
				ch = int( double( ( cw * sn ) / sd ) / this_aspect );

				if ( ch > img->height( ) )
				{
					cy = ( ch - img->height( ) ) / 2;
					ch = img->height( );
				}
				else if ( ch < img->height( ) )
				{
					cy = ( img->height( ) - ch ) / 2;
				}
			}
			else if ( prop_box_.value< pl::wstring >( ) == L"pillar" )
			{
				cw = int( double( ( ch * sd ) / sn ) * this_aspect );
				if ( cw > img->width( ) )
				{
					cx = ( cw - img->width( ) ) / 2;
					cw = img->width( );
				}
				else if ( cw < img->width( ) )
				{
					cx = ( img->width( ) - cw ) / 2;
				}
			}

			ch += ch % 2;

			img->crop( cx, cy, cw, ch );
			frame->set_image( img );
			double frame_aspect = frame->aspect_ratio( );

			sdl_rect_.w = int( frame_aspect / this_aspect * prop_width_.value< int >( ) );
			sdl_rect_.h = prop_height_.value< int >( );
			if ( sdl_rect_.w > prop_width_.value< int >( ) )
			{
				sdl_rect_.w = prop_width_.value< int >( );
				sdl_rect_.h = int( this_aspect / frame_aspect * prop_height_.value< int >( ) );
			}

			sdl_rect_.x = ( prop_width_.value< int >( ) - sdl_rect_.w ) / 2;
			sdl_rect_.y = ( prop_height_.value< int >( ) - sdl_rect_.h ) / 2;

			int overlay_width = ( img->width( ) / 8 ) * 8;

			if ( sdl_overlay_ != NULL && ( overlay_width != sdl_overlay_->w || img->height( ) != sdl_overlay_->h ) )
			{
				SDL_FreeYUVOverlay( sdl_overlay_ );
				sdl_overlay_ = SDL_CreateYUVOverlay( prop_width_.value< int >( ), prop_height_.value< int >( ), get_format( ), SDL_GetVideoSurface( ) );
				wipe_overlay( sdl_overlay_ );
				SDL_FreeYUVOverlay( sdl_overlay_ );
				SDL_Flip( SDL_GetVideoSurface( ) );
				sdl_overlay_ = NULL;
			}

			if ( sdl_overlay_ == NULL )
			{
				sdl_overlay_ = SDL_CreateYUVOverlay( overlay_width, img->height( ), get_format( ), SDL_GetVideoSurface( ) );
			}

			SDL_SetClipRect( SDL_GetVideoSurface( ), &sdl_rect_ );

			return sdl_overlay_;
		}

		void wipe_overlay( SDL_Overlay *overlay )
		{
			if ( overlay != NULL )
			{
				if ( SDL_LockYUVOverlay( overlay ) >= 0 )
				{
					if ( get_format( ) == SDL_YV12_OVERLAY )
					{
						for ( int plane = 0; plane < overlay->planes; plane ++ )
						{
							uint8_t *dst = overlay->pixels[ plane ];
							int dst_pitch = overlay->pitches[ plane ];
							int w = plane == 0 ? overlay->w : overlay->w / 2;
							int h = plane == 0 ? overlay->h : overlay->h / 2;
							unsigned char v = plane == 0 ? 16 : 128;

							while( h -- )
							{
								memset( dst, v, w );
								dst += dst_pitch;
							}
						}
					}
					else if ( get_format( ) == SDL_YUY2_OVERLAY )
					{
						uint8_t *dst = overlay->pixels[ 0 ];
						uint8_t *ptr;
						int t;
						int dst_pitch = overlay->pitches[ 0 ];
						int w = overlay->w;
						int h = overlay->h;

						while( h -- )
						{
							ptr = dst;
							t = w;
							while( t -- )
							{
								*ptr ++ = 16;
								*ptr ++ = 128;
							}
							dst += dst_pitch;
						}
					}

					SDL_Rect rect = { 0, 0, overlay->w, overlay->h };
					SDL_UnlockYUVOverlay( overlay );
					SDL_DisplayYUVOverlay( overlay, &rect );
				}
			}
		}

		Uint32 get_format( )
		{
			std::wstring pf = prop_pf_.value< pl::wstring >( ).c_str( );
			if ( pf == L"yuv422" )
				return SDL_YUY2_OVERLAY;
			return SDL_YV12_OVERLAY;
		}

		frame_type_ptr last_frame_;
		il::image_type_ptr last_image_;
		int last_sar_num_;
		int last_sar_den_;
		pcos::property prop_winid_;
		pcos::property prop_flags_;
		pcos::property prop_width_;
		pcos::property prop_height_;
		pcos::property prop_keydown_;
		pcos::property prop_keymod_;
		pcos::property prop_box_;
		pcos::property prop_pf_;
		pcos::property prop_full_;
		pcos::property prop_native_;
		boost::recursive_mutex mutex_;
};

class chunk_type 
{
	public:
		chunk_type( int size ) { chunk_ = new uint8_t[ size ]; }
		~chunk_type( ) { delete [] chunk_; }
		uint8_t *ptr( ) { return chunk_; }
	private:
		uint8_t *chunk_;
};

typedef boost::shared_ptr< chunk_type > chunk_type_ptr;

namespace
{
    template < typename C > 
    class fn_observer : public pcos::observer
    {
    public:
        fn_observer( C *instance, void ( C::*fn )( ) )
            : instance_( instance )
            , fn_( fn )
        { }

        virtual void updated( pcos::isubject * )
        { ( instance_->*fn_ )( ); }

    private:
        C *instance_;
        void ( C::*fn_ )( );
    };
}

class ML_PLUGIN_DECLSPEC sdl_audio : public store_type
{
	public:
        typedef fn_observer< sdl_audio > observer;
        typedef boost::recursive_mutex::scoped_lock scoped_lock;

		sdl_audio( const pl::wstring &, const frame_type_ptr & ) 
			: store_type( )
			, prop_buffer_( pcos::key::from_string( "buffer" ) )
			, prop_preroll_( pcos::key::from_string( "preroll" ) )
            , prop_pause_( pcos::key::from_string( "pause" ) )
			, position_( 0 )
			, chunks_at_start_( 0 )
			, audio_acquired_( false )
			, audio_spec_( )
			, buffer_( )
			, used_( 0 )
			, chunks_( )
			, mutex_( )
			, cond_( )
            , obs_pause_( new observer( const_cast<sdl_audio *>(this),&sdl_audio::update_pause ) ) 
            , is_paused_(true)
			, frame_duration_(40)

		{
			properties( ).append( prop_buffer_ = 1024 );
			properties( ).append( prop_preroll_ = 8 );
            properties( ).append( prop_pause_ = 1 );
            prop_pause_.attach( obs_pause_ );
		}

		virtual ~sdl_audio( )
		{
			{
				scoped_lock lock( mutex_ );
				flush( lock );
			}
			SDL_PauseAudio(1);
			SDL_CloseAudio( );
		}

		virtual bool init( )
		{
			return sdl_init_audio( );
		}

		virtual bool push( frame_type_ptr frame )
		{
            scoped_lock lock( mutex_ );
			if( frame )
				frame_duration_ = static_cast<int>((1.0 / frame->fps()) * 1000);

        	bool result = frame && frame->get_audio( );
			if ( result )
				result = queue_audio( frame->get_audio( ), lock );

			return result;
		}

        // Someone has altered the pause property
        // If it is set to anything but 0, make sure SDL is paused.
        void update_pause()
        {
            boost::recursive_mutex::scoped_lock lock( mutex_ );
            int should_pause = prop_pause_.value< int >();
            if( should_pause != 0 )
            {
				is_paused_ = true;
				complete( lock );
            }
        }

        void complete( )
        {
            scoped_lock lock( mutex_ );
            complete( lock );
        }

		virtual void complete( scoped_lock& lck )
		{
			if ( chunks_.size( ) > 0 && is_paused_ )
			{
				is_paused_ = false;
				while( chunks_.size( ) > 0 )
					cond_.wait( lck );
				cond_.notify_all( );
			}

			is_paused_ = true;
			cond_.notify_all( );
			chunks_.clear();
			position_ = 0;
			used_ = 0;
		}

        virtual frame_type_ptr flush( )
        {
            scoped_lock lock( mutex_ );
            return flush( lock );
        }

		frame_type_ptr flush( scoped_lock& lck )
		{
			is_paused_ = true;
			cond_.notify_all( );
			chunks_.clear( );
			position_ = 0;
			used_ = 0;

			return frame_type_ptr( );
		}

	protected:
		bool queue_audio( audio_type_ptr input, scoped_lock& lck )
		{
			bool result = true;

			audio_type_ptr audio = audio::coerce( audio::FORMAT_PCM16, input );

			if ( audio->channels( ) > 2 )
				audio = audio::channel_convert( audio, 2 );

			result = acquire_audio( audio, lck );

			if ( result )
				split_audio( audio, lck  );

			return result;
		}

		bool acquire_audio( audio_type_ptr audio, scoped_lock& lck )
		{
			int channels = audio->channels( );
			int frequency = audio->frequency( );

			if ( audio_acquired_ && ( channels != audio_spec_.channels || frequency != audio_spec_.freq ) )
			{	
				cond_.notify_all( );
                is_paused_ = true;
		
				flush( lck );
				audio_acquired_ = false;
                SDL_PauseAudio(1);
				SDL_CloseAudio( );
				buffer_ = chunk_type_ptr( );
				used_ = 0;
				position_ = 0;
			}

			if ( !audio_acquired_ )
			{
				SDL_AudioSpec request;

				memset( &request, 0, sizeof( SDL_AudioSpec ) );
				request.freq = frequency;
				request.format = AUDIO_S16SYS;
				request.channels = channels;
				request.samples = prop_buffer_.value< int >( );
				request.callback = callback;
				request.userdata = ( void * )this;

				audio_acquired_ = SDL_OpenAudio( &request, &audio_spec_ ) == 0;
                is_paused_ = true;
                // Start the SDL callback, will never stop even if we
                // are paused. When we are paused, we will fill the 
                // buffer with zeroes.
                SDL_PauseAudio(0);
			}

			return audio_acquired_;
		}

		bool split_audio( audio_type_ptr audio, scoped_lock& lck )
		{
			uint8_t *ptr = ( uint8_t * )audio->pointer( );
			int bytes = audio->samples( ) * audio->channels( ) * 2;
			int buffer_size = audio_spec_.samples * audio->channels( ) * 2;

			position_ ++;

			if ( position_ > prop_preroll_.value< int >( ) )
			{
			   	if ( is_paused_ )
				{
					chunks_at_start_ = static_cast<int>(chunks_.size( ));
					is_paused_ = false;
				}
			}

			while( bytes )
			{
				if ( !buffer_ )
					buffer_ = chunk_type_ptr( new chunk_type( buffer_size ) );

				if ( used_ + bytes >= buffer_size )
				{
					memcpy( buffer_->ptr( ) + used_, ptr, buffer_size - used_ );
					ptr += buffer_size - used_;
					bytes -= buffer_size - used_;
					{
						chunks_.push_back( buffer_ );
						cond_.notify_all( );
					}
					buffer_ = chunk_type_ptr( );
					used_ = 0;
				}
				else
				{
					memcpy( buffer_->ptr( ) + used_, ptr, bytes );
					used_ += bytes;
					bytes = 0;
				}
			}

            if ( position_ > prop_preroll_.value< int >( ) )
            {
                while( static_cast<int>(chunks_.size( )) > chunks_at_start_ )
                {
                    if( is_paused_ ) return true;
                    cond_.wait( lck );
                }
            }

			return true;
		}

		static void callback( void *data, uint8_t *buffer, int len )
		{
			( static_cast< sdl_audio * >( data ) )->fill_buffer( buffer, len );
		}

		void fill_buffer( uint8_t *buffer, int len )
		{
            scoped_lock lock(mutex_);

            if( is_paused_ )
            {
                memset( buffer, 0, len);
                return;
            }

			if( chunks_.size( ) == 0 )
			{
				// Never wait longer than the duration of one frame.
                cond_.timed_wait( lock, boost::posix_time::milliseconds(frame_duration_) );
			}
			
			if ( chunks_.size( ) > 0 && !is_paused_ )
			{
        		chunk_type_ptr chunk = chunks_[ 0 ];
				chunks_.pop_front( );
				memcpy( buffer, chunk->ptr( ), len );
				cond_.notify_all( );
			}
		    else
            {
                memset( buffer, 0, len);
                used_ = 0;
            }
		}

		pcos::property prop_buffer_;
		pcos::property prop_preroll_;
        pcos::property prop_pause_;
		int position_;
		int chunks_at_start_;
		bool audio_acquired_;
		SDL_AudioSpec audio_spec_;
		chunk_type_ptr buffer_;
		int used_;
		std::deque< chunk_type_ptr > chunks_;
		boost::recursive_mutex mutex_;
		boost::condition_variable_any cond_;
        boost::shared_ptr< pcos::observer > obs_pause_;
        bool is_paused_;
		int frame_duration_;
};

//
// Plugin object
//

class ML_PLUGIN_DECLSPEC sdl_plugin : public openmedialib_plugin
{
public:
	virtual input_type_ptr input( const pl::wstring & )
	{
		return input_type_ptr( );
	}

	virtual store_type_ptr store( const pl::wstring &args, const frame_type_ptr &frame )
	{
		if ( args == L"sdl_audio:" )
			return store_type_ptr( new sdl_audio( args, frame ) );
		else
			return store_type_ptr( new sdl_video( args, frame ) );
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
		*plug = new ml::sdl_plugin;
		return true;
	}
	
	ML_PLUGIN_DECLSPEC void openplugin_destroy_plugin( pl::openplugin* plug )
	{ 
		delete static_cast< ml::sdl_plugin * >( plug ); 
	}
}

#ifdef WIN32 
    #pragma warning(pop)
#endif
