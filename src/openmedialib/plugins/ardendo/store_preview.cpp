// preview store
//
// Copyright (C) 2007 Ardendo
// Released under the terms of the LGPL.
//
// #store:preview:
// 
// Provides a simple/configurable audio/video playout store.

#include "precompiled_headers.hpp"

#ifdef WIN32
#pragma warning(push)
#pragma warning( disable : 4355 )
#endif

#include "amf_filter_plugin.hpp"
#include "utility.hpp"
#include <iostream>

#include <openpluginlib/pl/timer.hpp>
#include <opencorelib/cl/media_definitions.hpp>
#include <boost/operators.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <boost/weak_ptr.hpp>

#include <boost/cstdint.hpp>

namespace aml 
{ 
    namespace openmedialib 
    {
        class time_value : public boost::addable< time_value >,
                           public boost::subtractable< time_value >,
                           public boost::equality_comparable< time_value >
        {
        public:
            time_value( boost::int64_t secs, boost::int64_t micro_secs )
                : m_secs(secs), m_micro_secs(micro_secs)
            {
            }

            boost::int64_t get_seconds() const { return m_secs; }
            void set_seconds( boost::int64_t t ) { m_secs = t; }

            boost::int64_t get_micro_seconds() const { return m_micro_secs; }
            void set_micro_seconds( boost::int64_t t ) { m_micro_secs = t; }

        private:
            boost::int64_t m_secs;
            boost::int64_t m_micro_secs;
        };

        class timer
        {
        public:
            virtual ~timer() {}
            
            virtual void start() = 0;
            virtual void stop() = 0;	
            virtual void sleep( const time_value& val ) = 0;
            virtual time_value elapsed() const = 0;
            virtual bool reset() = 0;
        };

        #ifdef WIN32
        class win_timer 
        {
        public:
            typedef olib::openpluginlib::time_value value_type;
            win_timer() : m_started(0), m_start_time(0), m_duration(0)
            {
                m_event = ::CreateEvent(NULL, FALSE, FALSE, NULL );
            }

            virtual ~win_timer()
            {
                if(m_event) CloseHandle(m_event);
            }


            void start() 
            {
                m_duration = 0;
                if( timeBeginPeriod(1) != TIMERR_NOERROR )
                    std::cerr << "Could not set timer resolution to 1 ms\n"; 
                m_start_time = timeGetTime();
                timeEndPeriod(1);
            }

            void stop()
            {
                if( timeBeginPeriod(1) != TIMERR_NOERROR )
                    std::cerr << "Could not set timer resolution to 1 ms\n"; 

                m_duration = timeGetTime() - m_start_time;
                timeEndPeriod(1);
            }

            void sleep( const value_type& val ) 
            {
                DWORD to_wait = static_cast<DWORD>( val.tv_sec * 1000 + val.tv_usec / 1000 );
                if( to_wait <= 0 ) return;
                //std::cout << "sleeping " << to_wait << " ms\n";
                timeSetEvent( to_wait , 0, (LPTIMECALLBACK)m_event, 0, TIME_CALLBACK_EVENT_SET );
                ::WaitForSingleObject(m_event, INFINITE);
            }

            value_type elapsed() const
            {
                value_type vt;
                vt.tv_sec = m_duration / 1000;
                vt.tv_usec = ( m_duration - (vt.tv_sec * 1000) )* 1000;
                return vt;
            }

            bool reset() 
            {
                m_started = false;
                m_duration = 0;
                m_start_time = 0;
                return true;
            }

        private:

            bool m_started;
            DWORD m_start_time, m_duration;
            HANDLE m_event;
        };
        #endif

        class timer_impl : public timer
        {
        public:
            
            virtual ~timer_impl() {}

            virtual void start()
            {
                m_timer.start();
            }

            virtual void stop()
            {
                m_timer.stop();
            }

            virtual void sleep( const time_value& val )
            {
               value_type vt;
               vt.tv_sec = val.get_seconds();
               vt.tv_usec = val.get_micro_seconds();
               m_timer.sleep(vt);
            }

            virtual time_value elapsed() const
            {
                value_type vt = m_timer.elapsed();
                return time_value(vt.tv_sec, vt.tv_usec);
            }

            virtual bool reset()
            {
                return m_timer.reset();
            }

        private:
            #ifdef WIN32
                //typedef pl::rdtsc_default_timer::value_type value_type;
				//pl::rdtsc_default_timer m_timer;
                typedef win_timer::value_type value_type;
                win_timer m_timer;
            #else
                typedef pl::gettimeofday_default_timer::value_type value_type;
				pl::gettimeofday_default_timer m_timer;
            #endif
        };

		typedef boost::shared_ptr< timer > timer_ptr;

        static timer_ptr create_timer()
        {
            timer_ptr ptr( new timer_impl());
            return ptr;
        }
    }
}

namespace aml { namespace openmedialib {

static boost::mutex audio_lock_;
static boost::weak_ptr< ml::store_type > audio_store;
static boost::int64_t audio_owner = 0;

#define const_preview const_cast< store_preview * >

class ML_PLUGIN_DECLSPEC store_preview : public ml::store_type
{
	typedef boost::mutex::scoped_lock scoped_lock;

	public:
		store_preview( const std::wstring &resource, const ml::frame_type_ptr &frame )
			: ml::store_type( )
			, prop_video_( pcos::key::from_string( "video" ) )
			, prop_audio_( pcos::key::from_string( "audio" ) )
			, prop_keydown_( pcos::key::from_string( "keydown" ) )
			, prop_audio_scrub_( pcos::key::from_string( "audio_scrub" ) )
			, prop_callback_( pcos::key::from_string( "callback" ) )
			, prop_callback_arg_( pcos::key::from_string( "callback_arg" ) )
			, prop_box_( pcos::key::from_string( "box" ) )
			, prop_threaded_( pcos::key::from_string( "threaded" ) )
			, prop_grab_audio_( pcos::key::from_string( "grab_audio" ) )
			, prop_video_store_( pcos::key::from_string( "video_store" ) )
			, prop_audio_store_( pcos::key::from_string( "audio_store" ) )
			, prop_supported_listen_modes_( pcos::key::from_string( "supported_listen_modes" ) )
			, prop_deferrable_( pcos::key::from_string( "deferrable" ) )
			, prop_timed_( pcos::key::from_string( "timed" ) )
			, prop_ttn_( pcos::key::from_string( "ttn" ) )
			, prop_sync_( pcos::key::from_string( "sync" ) )
			, prop_preroll_( pcos::key::from_string( "preroll" ) )
			, prop_keys_wanted_( pcos::key::from_string( "keys_wanted" ) )
			, prop_deinterlace_( pcos::key::from_string( "deinterlace" ) )
			, prop_native_( pcos::key::from_string( "native" ) )
			, key_box_( pcos::key::from_string( "box" ) )
			, key_deferrable_( pcos::key::from_string( "deferrable" ) )
			, key_preroll_( pcos::key::from_string( "preroll" ) )
			, key_scrubframe_( pcos::key::from_string( "scrubframe" ) )
			, key_keydown_( pcos::key::from_string( "keydown" ) )
			, obs_box_( new fn_observer< store_preview >( const_preview( this ), &store_preview::update_box ) )
			, obs_grab_audio_( new fn_observer< store_preview >( const_preview( this ), &store_preview::grab_audio ) )
			, obs_sync_( new fn_observer< store_preview >( const_preview( this ), &store_preview::sync ) )
			, video_( )
			, cache_( )
			, count_( 0 )
			, start_( 0.0 )
			, pts_( 0.0 )
			, last_speed_( 1 )
			, last_position_( -1 )
			, next_image_( 0.0 )
			, video_mutex_( )
			, last_frame_shown_( )
			, first_( true )
			, last_audio_push_result_( false )
			, first_frame_( frame )
		{
			timer_ = aml::openmedialib::create_timer( );

			properties( ).append( prop_video_ = std::wstring( L"sdl_video:" ) );
			properties( ).append( prop_audio_ = std::wstring( L"sdl_audio:" ) );
			properties( ).append( prop_keydown_ = 0 );
			properties( ).append( prop_audio_scrub_ = 1 );
			properties( ).append( prop_callback_ = boost::uint64_t( 0 ) );
			properties( ).append( prop_callback_arg_ = boost::uint64_t( 0 ) );
			properties( ).append( prop_box_ = std::wstring( L"" ) );
			properties( ).append( prop_threaded_ = 0 );
			properties( ).append( prop_grab_audio_ = 0 );
            properties( ).append( prop_video_store_ = ml::store_type_ptr() );
            properties( ).append( prop_audio_store_ = ml::store_type_ptr() );
			std::vector<int> listen_modes;
			listen_modes.push_back( (int)cl::listen_mode::mono );
			listen_modes.push_back( (int)cl::listen_mode::stereo_2_0 );
			properties( ).append( prop_supported_listen_modes_ = listen_modes );
			properties( ).append( prop_deferrable_ = 0 );
			properties( ).append( prop_timed_ = 0 );
			properties( ).append( prop_ttn_ = 0 );
			properties( ).append( prop_sync_ = 0 );
			properties( ).append( prop_preroll_ = 1 );
			properties( ).append( prop_keys_wanted_ = 1 );
			properties( ).append( prop_deinterlace_ = 1 );
			properties( ).append( prop_native_ = 0.0 );
			prop_box_.attach( obs_box_ );
			prop_grab_audio_.attach( obs_grab_audio_ );
			prop_sync_.attach( obs_sync_ );
			prop_sync_.set_always_notify( true );
		}

		virtual ~store_preview( )
		{
			scoped_lock lock( audio_lock_ );
            if ( audio_owner == boost::int64_t( this ) && audio_ )
				audio_owner = 0;
			timer_->stop( );
		}

		void sync( )
		{
			if ( video_ )
				pass_all_props( video_, "@video." );
			if ( audio_ )
				pass_all_props( audio_, "@audio." );
		}

		void update_box( )
		{
			std::wstring value = prop_box_.value< std::wstring >( );
			if ( video_ && value != L"" )
			{
				pl::pcos::property box = video_->properties( ).get_property_with_key( key_box_ );
				if ( box.valid( ) )
					box = value;
			}
		}

		void grab_audio( )
		{
			scoped_lock lock( audio_lock_ );

            if ( audio_owner != boost::int64_t( this ) )
			{
				// Create or obtain the audio
            	ml::store_type_ptr stp = audio_store.lock();
				if ( !stp )
				{
                    audio_owner = boost::int64_t( this );
					audio_ = ml::create_store( prop_audio_.value< std::wstring >( ), ml::frame_type_ptr( ) );
                	audio_store = audio_;
					if ( audio_ )
					{
						pass_all_props( audio_, "@audio." );
						audio_->init( );
						if( audio_->properties().get_property_with_string("supported_listen_modes").valid() )
						{
							prop_supported_listen_modes_ = audio_->properties().get_property_with_string("supported_listen_modes").value< std::vector<int> >();
						}
						if( audio_->properties().get_property_with_string("preroll").valid() )
						{
							prop_preroll_ = audio_->properties().get_property_with_string("preroll").value< int >();
						}
					}
				}
				else
				{
                    audio_owner = boost::int64_t( this );
                	audio_ = stp;
					audio_->flush( );
				}

				// We need to ensure that the preroll is filled when we obtain audio
				cache_.erase( cache_.begin( ), cache_.end( ) );

				// Reset everything
				count_ = 0;
				start_ = 0.0;
				next_image_ = 0.0;
				pts_ = 0.0;
			}

            prop_audio_store_ = audio_;
		}

		virtual bool init( )
		{
			ARENFORCE( internal_pusher_ = ml::create_input( "pusher:" ) );
			ARENFORCE( deinterlace_filter_ = ml::create_filter( L"deinterlace" ) );
			deinterlace_filter_->connect(internal_pusher_);
			
			if ( !video_ )
			{
				std::wstring store = prop_video_.value< std::wstring >( );

				video_ = ml::create_store( store, first_frame_ );

				if ( !video_ )
					throw std::runtime_error( ( boost::format( "Unable to create store %s" ) % olib::opencorelib::str_util::to_string( store ) ).str( ) );

				if ( video_->properties( ).get_property_with_key( key_deferrable_ ).valid( ) )
                    prop_deferrable_ = video_->properties( ).get_property_with_key( key_deferrable_ ).value< int >( );

				if ( prop_native_.value< double >( ) > 0.0 )
				{
					int num, den;
					first_frame_->get_sar( num, den );
					int native_width = first_frame_->width( ) * num / den;
					int native_height = first_frame_->height( );
					video_->property( "width" ) = int( prop_native_.value< double >( ) * native_width );
					video_->property( "height" ) = int( prop_native_.value< double >( ) * native_height );
				}

            	prop_video_store_ = video_;
				update_box( );

				if ( video_ )
					pass_all_props( video_, "@video." );

				grab_audio( );

				timer_->start( );

				first_frame_ = ml::frame_type_ptr( );
			
				return video_ && video_->init( );
			}
			else
			{
				grab_audio( );
			}

			return video_;
		}

		void reset( ml::frame_type_ptr frame )
		{
			start_ = 0.0;
			pts_ = 0.0;
			last_speed_ = 1;
			last_position_ = frame->get_position( ) - 1;
			count_ = 0;
			cache_.erase( cache_.begin( ), cache_.end( ) );
		}

		ml::frame_type_ptr next_shown( ml::frame_type_ptr frame )
		{
			if ( cache_.begin( ) != cache_.end( ) )
				return cache_[ 0 ];
			return frame;
		}

		double duration( ml::frame_type_ptr frame )
		{
			if ( frame->get_audio( ) )
				return double( frame->get_audio( )->samples( ) ) / frame->get_audio( )->frequency( );
			else
				return double( frame->get_fps_den( ) ) / frame->get_fps_num( );
		}

		bool active( )
		{ return count_ > prop_preroll_.value< int >( ); }

		double time_now( )
		{
			timer_->stop( );
			aml::openmedialib::time_value tv = timer_->elapsed( );
            return double( tv.get_seconds( ) & 0xfffff ) + ( double( tv.get_micro_seconds( ) ) / 1000000 );
		}

		void time_sleep( double t )
		{
			if ( t > 0 )
			{
				aml::openmedialib::time_value tv( boost::int64_t( t ), boost::int64_t( ( t - int( t ) ) * 1000000 ) );
#				ifdef WIN32
				// Windows timer seems to be inaccurate - for now, we'll rely on 
				// audio blocking to handle the sync.
				if ( audio_owner != boost::int64_t( this ) || last_audio_push_result_ == false )
					timer_->sleep( tv );
#				else
					timer_->sleep( tv );
#				endif
			}
		}

		virtual bool push( ml::frame_type_ptr frame )
		{
            bool result = false;
			{
				if ( prop_deinterlace_.value<int>() == 1 )
				{
					internal_pusher_->push(frame);
					deinterlace_filter_->seek( frame->get_position( ) );
					ARENFORCE( frame = deinterlace_filter_->fetch() );
				}

				if ( first_ && frame && frame->get_image( ) )
				{
					video_->push( frame );
					first_ = false;
				}

				// Handle refresh of last frame
				if ( !frame )
				{
					scoped_lock lock( video_mutex_ );
					if ( last_frame_shown_ )
						video_->push( last_frame_shown_ );
					return true;
				}

				// Deal with the gap between this frame and the last
				int position = frame->get_position( );
				if ( position - last_position_ != last_speed_ )
                	last_speed_ = position - last_position_;

				if ( prop_preroll_.value< int >( ) == 1 )
				{
					if ( last_speed_ == 0 )
						time_sleep( duration( frame ) );
				}
				else if ( !prop_timed_.value< int >( ) && next_image_ != 0.0 )
				{
					double diff = next_image_ - time_now( );
					time_sleep( diff );
				}

				//Since we rely on the audio story for sync, we need to sleep
				//manually if the audio push fails (for example, if there is
				//no sound card).
				if( last_audio_push_result_ == false || !frame->get_audio() || audio_owner != boost::int64_t( this ) )
				{
					double diff = next_image_ - time_now( );
					time_sleep( diff );
				}

				// Set the pts of this frame
				frame->set_pts( pts_ );
				pts_ += duration( frame );

				// Determine the display time of the next image
				double next = next_shown( frame )->get_pts( );

				// If we're paused, silence audio and reset the clock
				if ( position == last_position_ )
				{
					ml::frame_volume( frame, 0.0 );
					next = duration( frame );
					next_image_ = 0.0;
				}

				pcos::property scrub = frame->properties().get_property_with_key( key_scrubframe_ );

				// Handle the preroll now
                if ( std::abs( last_speed_ ) == 1 && frame->get_audio( ) && 
						audio_owner == boost::int64_t( this ) && !scrub.valid() )
				{
					audio_->properties( ).get_property_with_key( key_preroll_ ) = prop_preroll_.value< int >( );
					count_ ++;
					cache_.push_back( frame );
					last_audio_push_result_ = result = audio_->push( frame );
					if ( active( ) && start_ == 0.0 )
						start_ = time_now( );
					if ( count_ >= prop_preroll_.value< int >( ))
					{
						ml::frame_type_ptr image = cache_[ 0 ];
						cache_.erase( cache_.begin( ) );
						if ( video_ )
							result = video_push( image );
					}
				}
				else if ( frame->get_audio( ) && frame->get_audio( )->channels( ) > 0 &&
						( audio_owner == boost::int64_t( this ) || scrub.valid() )  )
				{
					if( audio_owner !=  boost::int64_t( this ) && scrub.valid() ) 
					{
						grab_audio();
					}

					audio_->properties( ).get_property_with_key( key_preroll_ ) = 0;
					count_ = 1;
					cache_.erase( cache_.begin( ), cache_.end( ) );
					if ( video_ )
						result = video_push( frame );
					if ( prop_audio_scrub_.value< int >( ) || std::abs( last_speed_ ) == 1 )
						last_audio_push_result_ = result = audio_->push( frame );
					else
						start_ -= duration( frame );

					if( scrub.valid() ) audio_->complete();
				}
				else if ( video_ )
				{
					count_ = prop_preroll_.value< int >( );
					result = video_push( frame );
				}
	
				// Calculate the delay required before the next frame is displayed
				last_position_ = position;
                if ( audio_owner == boost::int64_t( this ) )
				{
					double diff = start_ + next - time_now( );
					if ( last_speed_ == 0 )
					{
						next_image_ = time_now( ) + duration( frame );
					}
					else if ( !active( ) )
					{
						next_image_ = time_now( ) + duration( frame );
					}
					else if ( diff < 0.0 )
					{
						start_ -= diff;
					}
					else
					{
						next_image_ = start_ + next;
					}
				}
				else if ( audio_owner == 0 )
				{
					grab_audio( );
				}
				else if ( prop_threaded_.value< int >( ) )
				{
					next_image_ = time_now( ) + duration( frame );
				}

				if ( next_image_ > 0.0 )
				{
					int ms = int( ( next_image_ - time_now( ) ) * 1000 );
					if ( ms > 0 )
						prop_ttn_ = ms;
					else
						prop_ttn_ = 0;
				}

				if ( video_ && prop_keys_wanted_.value< int >( ) )
				{
					pl::pcos::property keydown = video_->properties( ).get_property_with_key( key_keydown_ );
					if ( keydown.valid( ) )
					{
						int key = keydown.value< int >( );
						if ( key != 0 )
						{
							prop_keydown_ = key;
							keydown.set( 0 );
						}
					}
				}
			}

			typedef void ( *callback )( boost::uint64_t , ml::frame_type_ptr );
			callback cb = callback( prop_callback_.value< boost::uint64_t >( ) );
			if ( last_frame_shown_ && cb )
				cb( prop_callback_arg_.value< boost::uint64_t >( ), last_frame_shown_ );

			return result;
		}

		virtual ml::frame_type_ptr flush( )
		{
			ml::frame_type_ptr frame;
			if ( video_ ) 
				video_->flush( );
            if ( audio_owner == boost::int64_t( this ) )
				audio_->flush( );
			if ( cache_.size( ) )
				frame = cache_[ 0 ];
			cache_.erase( cache_.begin( ), cache_.end( ) );
			count_ = 0;
			return frame;
		}

		virtual void complete( )
		{
			if ( active( ) && start_ == 0.0 )
				start_ = time_now( );
			bool result = true;
			while ( result && cache_.size( ) )
			{
				ml::frame_type_ptr image = cache_[ 0 ];
				cache_.erase( cache_.begin( ) );
				if ( video_ )
					result = video_push( image );
				double diff = start_ + image->get_pts( ) - time_now( );
				if ( diff > 0.0 )
					time_sleep( diff );
			}
            if ( audio_owner == boost::int64_t( this ) )
				audio_->complete( );
			count_ = 0;
		}

	private:

		bool video_push( ml::frame_type_ptr frame )
		{
			scoped_lock lock( video_mutex_ );
			if ( frame )
			{
				last_frame_shown_ = frame;
            			return video_->push( frame );
			}
			return true;
		}

		void pass_all_props( ml::store_type_ptr &store, std::string prefix )
		{
			int ps = static_cast<int>(prefix.size( ));
			pcos::key_vector keys = properties( ).get_keys( );
			for( pcos::key_vector::iterator it = keys.begin( ); it != keys.end( ); ++it )
			{
				std::string name( ( *it ).as_string( ) );
				if ( name.find( prefix ) == 0 )
				{
					std::string prop = name.substr( ps );
					pcos::property p = store->properties( ).get_property_with_string( prop.c_str( ) );
					pcos::property v = properties( ).get_property_with_string( name.c_str( ) );
					if ( p.valid( ) && v.valid( ) )
					{
						if ( p.is_a< int >( ) && v.is_a< int >( ) )
							p = v.value< int >( );
						else if ( p.is_a< double >( ) && v.is_a< double >( ) )
							p = v.value< double >( );
                        else if ( p.is_a< boost::uint64_t >( ) && v.is_a< boost::uint64_t >( ) )
                            p = v.value< boost::uint64_t >( );
                        else if ( p.is_a< boost::int64_t >( ) && v.is_a< boost::int64_t >( ) )
                            p = v.value< boost::int64_t >( ) ;
						else if ( v.is_a< std::wstring >( ) )
							p.set_from_string( v.value< std::wstring >( ) );
						else
							std::cerr << "Don't know how to match property " << name << std::endl;
					}
					else
						std::cerr << "Unknown property " << name << std::endl;
				}
			}
		}

		aml::openmedialib::timer_ptr timer_;
		pl::pcos::property prop_video_;
		pl::pcos::property prop_audio_;
		pl::pcos::property prop_keydown_;
		pl::pcos::property prop_audio_scrub_;
		pl::pcos::property prop_callback_;
		pl::pcos::property prop_callback_arg_;
		pl::pcos::property prop_box_;
		pl::pcos::property prop_threaded_;
		pl::pcos::property prop_grab_audio_;
		pl::pcos::property prop_video_store_;
		pl::pcos::property prop_audio_store_;
		pl::pcos::property prop_supported_listen_modes_;
		pl::pcos::property prop_deferrable_;
		pl::pcos::property prop_timed_;
		pl::pcos::property prop_ttn_;
		pl::pcos::property prop_sync_;
		pl::pcos::property prop_preroll_;
		pl::pcos::property prop_keys_wanted_;
		pl::pcos::property prop_deinterlace_;
		pl::pcos::property prop_native_;
		pl::pcos::key key_box_;
		pl::pcos::key key_deferrable_;
		pl::pcos::key key_preroll_;
		pl::pcos::key key_scrubframe_;
		pl::pcos::key key_keydown_;
		boost::shared_ptr< pl::pcos::observer > obs_box_;
		boost::shared_ptr< pl::pcos::observer > obs_grab_audio_;
		boost::shared_ptr< pl::pcos::observer > obs_sync_;
		ml::store_type_ptr video_, audio_;
		ml::input_type_ptr internal_pusher_;
		ml::filter_type_ptr deinterlace_filter_;
		std::deque< ml::frame_type_ptr > cache_;
		int count_;
		double start_;
		double pts_;
		int last_speed_;
		int last_position_;
		double next_image_;
		mutable boost::mutex video_mutex_;
		ml::frame_type_ptr last_frame_shown_;
		bool first_;
		bool last_audio_push_result_;
		ml::frame_type_ptr first_frame_;
};

ml::store_type_ptr ML_PLUGIN_DECLSPEC create_store_preview( const std::wstring &resource, const ml::frame_type_ptr &frame )
{
	return ml::store_type_ptr( new store_preview( resource, frame ) );
}

} }


#ifdef WIN32
#pragma warning(pop)
#endif

