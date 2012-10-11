// decode - A decode plugin to ml.
//
// Copyright (C) 2010 Vizrt
// Released under the LGPL.

#include <openmedialib/ml/analyse.hpp>
#include <openmedialib/ml/openmedialib_plugin.hpp>
#include <openmedialib/ml/stream.hpp>
#include <openmedialib/ml/audio_block.hpp>
#include <openpluginlib/pl/pcos/observer.hpp>
#include <opencorelib/cl/thread_pool.hpp>
#include <opencorelib/cl/function_job.hpp>
#include <opencorelib/cl/profile.hpp>
#include <opencorelib/cl/enforce_defines.hpp>
#include <opencorelib/cl/log_defines.hpp>
#include <opencorelib/cl/uuid_16b.hpp>
#include <opencorelib/cl/str_util.hpp>
#include <openmedialib/ml/filter_encode.hpp>
#include <openmedialib/ml/fix_stream.hpp>

#include <boost/algorithm/string.hpp>
#include <boost/enable_shared_from_this.hpp>

#include <iostream>
#include <cstdlib>
#include <vector>
#include <string>
#include <set>

#define LOKI_CLASS_LEVEL_THREADING
#include <loki/Singleton.h>

namespace pl = olib::openpluginlib;
namespace ml = olib::openmedialib::ml;
namespace il = olib::openimagelib::il;
namespace pcos = olib::openpluginlib::pcos;
namespace cl = olib::opencorelib;

namespace olib { namespace openmedialib { namespace ml { namespace decode {

static pl::pcos::key key_length_ = pl::pcos::key::from_string( "length" );
static pl::pcos::key key_force_ = pl::pcos::key::from_string( "force" );
static pl::pcos::key key_width_ = pl::pcos::key::from_string( "width" );
static pl::pcos::key key_height_ = pl::pcos::key::from_string( "height" );
static pl::pcos::key key_fps_num_ = pl::pcos::key::from_string( "fps_num" );
static pl::pcos::key key_fps_den_ = pl::pcos::key::from_string( "fps_den" );
static pl::pcos::key key_sar_num_ = pl::pcos::key::from_string( "sar_num" );
static pl::pcos::key key_sar_den_ = pl::pcos::key::from_string( "sar_den" );
static pl::pcos::key key_interlace_ = pl::pcos::key::from_string( "interlace" );
static pl::pcos::key key_bit_rate_ = pl::pcos::key::from_string( "bit_rate" );
static pl::pcos::key key_complete_ = pl::pcos::key::from_string( "complete" );
static pl::pcos::key key_instances_ = pl::pcos::key::from_string( "instances" );
static pl::pcos::key key_threads_ = pl::pcos::key::from_string( "threads" );

class filter_pool
{
public:
	virtual ~filter_pool( ) { }
	
	friend class shared_filter_pool;
	friend class frame_lazy;
	
protected:
	virtual ml::filter_type_ptr filter_obtain( ) = 0;
	virtual void filter_release( ml::filter_type_ptr ) = 0;
	
};

class shared_filter_pool
{
public:
	ml::filter_type_ptr filter_obtain( filter_pool *pool_token )
	{
		boost::recursive_mutex::scoped_lock lck( mtx_ );
		std::set< filter_pool * >::iterator it = pools_.begin( );
		for( ; it != pools_.end( ); ++it )
		{
			if( *it == pool_token )
			{
				return (*it)->filter_obtain( );
			}
		}
		return ml::filter_type_ptr( );
	}

	void filter_release( ml::filter_type_ptr filter, filter_pool *pool_token )
	{
		ARENFORCE_MSG( filter, "Releasing null filter" );
		
		boost::recursive_mutex::scoped_lock lck( mtx_ );
		std::set< filter_pool * >::iterator it = pools_.begin( );
		for( ; it != pools_.end( ); ++it )
		{
			if( *it == pool_token )
			{
				(*it)->filter_release( filter );
			}
		}
	}
	
	void add_pool( filter_pool *pool )
	{
		boost::recursive_mutex::scoped_lock lck( mtx_ );
		pools_.insert( pool );
	}
	
	void remove_pool( filter_pool * pool )
	{
		boost::recursive_mutex::scoped_lock lck( mtx_ );
		pools_.erase( pool );
	}
		
private:
	std::set< filter_pool * > pools_;
	boost::recursive_mutex mtx_;
		   
};

typedef enum
{
	eval_image = 1,
	eval_stream = 2
}
component;

typedef Loki::SingletonHolder< shared_filter_pool, Loki::CreateUsingNew, Loki::PhoenixSingleton > the_shared_filter_pool;

class ML_PLUGIN_DECLSPEC frame_lazy : public ml::frame_type 
{
	private:

		//Helper class that will make sure that the filter is returned to the
		//filter pool, once all users of it are gone.
		class filter_pool_holder
		{
		public:
			filter_pool_holder( filter_pool *pool )
			: pool_( pool )
			{ }

			virtual ~filter_pool_holder()
			{
			}

			ml::filter_type_ptr filter()
			{
				filter_ = filter_ ? filter_ : the_shared_filter_pool::Instance().filter_obtain( pool_ );
				return filter_;
			}

			void release( )
			{
				if ( filter_ )
					the_shared_filter_pool::Instance().filter_release( filter_, pool_ );
				filter_ = ml::filter_type_ptr( );
			}

		private:
			ml::filter_type_ptr filter_;
			filter_pool *pool_;
		};


	public:
		/// Constructor
		frame_lazy( const frame_type_ptr &other, int frames, filter_pool *pool_token, bool validated = true )
			: ml::frame_type( other.get( ) )
			, parent_( other )
			, frames_( frames )
			, pool_holder_( new filter_pool_holder( pool_token ) )
			, validated_( validated )
			, evaluated_( 0 )
		{
			ARENFORCE( other->get_position() < frames )(other->get_position())(frames);
		}

		/// Destructor
		virtual ~frame_lazy( )
		{
		}

		void evaluate( component comp )
		{
			filter_type_ptr filter;
			evaluated_ |= comp;
			if ( pool_holder_ && ( filter = pool_holder_->filter( ) ) )
			{
				ml::frame_type_ptr other = parent_;

				{
					ml::input_type_ptr pusher = filter->fetch_slot( 0 );

					if ( pusher == 0 )
					{
						pusher = ml::create_input( L"pusher:" );
						pusher->property_with_key( key_length_ ) = frames_;
						filter->connect( pusher );
						filter->sync( );
					}
					else if ( pusher->get_uri( ) == L"pusher:" && pusher->get_frames( ) != frames_ )
					{
						pusher->property_with_key( key_length_ ) = frames_;
						filter->sync( );
					}

					// Do any codec specific modifications of the packet to make it decodable
					ml::fix_stream( other );

					pusher->push( other );
					filter->seek( other->get_position( ) );
					other = filter->fetch( );

					ARENFORCE( !other->in_error() );

					//Empty pusher in case filter didn't read from it due to caching
					pusher->fetch( );
				}

				if ( comp == eval_image )
				{
					image_ = other->get_image( );
					alpha_ = other->get_alpha( );
				}

				audio_ = other->get_audio( );
				//properties_ = other->properties( );
				if ( comp == eval_stream )
				{
					stream_ = other->get_stream( );
				}
				pts_ = other->get_pts( );
				duration_ = other->get_duration( );
				//sar_num_ = other->get_sar_num( );
				//sar_den_ = other->get_sar_den( );
				fps_num_ = other->get_fps_num( );
				fps_den_ = other->get_fps_den( );
				exceptions_ = other->exceptions( );
				queue_ = other->queue( );

				//We will not have any use for the filter now
				pool_holder_->release();
			}
		}

		/// Provide a shallow copy of the frame (and all attached frames)
		virtual frame_type_ptr shallow( ) const
		{
			ml::frame_type_ptr clone( new frame_lazy( this, parent_->shallow() ) );
			return clone;
		}

		/// Provide a deep copy of the frame (and all attached frames)
		virtual frame_type_ptr deep( )
		{
			frame_type_ptr result = parent_->deep( );
			result->set_image( get_image( ) );
			return result;
		}

		/// Indicates if the frame has an image
		virtual bool has_image( )
		{
			return !( evaluated_ & eval_image ) || image_;
		}

		/// Indicates if the frame has audio
		virtual bool has_audio( )
		{
			return ( pool_holder_ && parent_->has_audio( ) ) || audio_;
		}

		/// Set the image associated to the frame.
		virtual void set_image( olib::openimagelib::il::image_type_ptr image, bool decoded )
		{
			frame_type::set_image( image, decoded );
			evaluated_ = eval_image | eval_stream;
			pool_holder_.reset();
			if( parent_ )
				parent_->set_image( image, decoded );
		}

		/// Get the image associated to the frame.
		virtual olib::openimagelib::il::image_type_ptr get_image( )
		{
			if( !( evaluated_ & eval_image ) ) evaluate( eval_image );
			return image_;
		}

		/// Set the audio associated to the frame.
		virtual void set_audio( audio_type_ptr audio, bool decoded )
		{
			frame_type::set_audio( audio, decoded );
			//We need to set the audio object on the inner frame as well to avoid
			//our own audio object being overwritten incorrectly on an evaluate() call
			if( parent_ )
				parent_->set_audio( audio, decoded );
		}

		/// Get the audio associated to the frame.
		virtual audio_type_ptr get_audio( )
		{
			return audio_;
		}

		virtual void set_stream( stream_type_ptr stream )
		{
			frame_type::set_stream( stream );
			if( parent_ )
				parent_->set_stream( stream );
		}

		virtual stream_type_ptr get_stream( )
		{
			if( !( evaluated_ & eval_stream ) ) evaluate( eval_stream );
			return stream_;
		}

		virtual void set_position( int position )
		{
			frame_type::set_position( position );
		}

	protected:
		
		frame_lazy( const frame_lazy *org, const frame_type_ptr &other )
			: ml::frame_type( org )
			, parent_( other )
			, frames_( org->frames_ )
			, pool_holder_( org->pool_holder_ )
			, validated_( org->validated_ )
			, evaluated_( org->evaluated_ )
		{
		}

	private:
		ml::frame_type_ptr parent_;
		int frames_;
		boost::shared_ptr< filter_pool_holder > pool_holder_;
		bool validated_;
		int evaluated_;
};

class filter_analyse : public ml::filter_simple
{
public:
    // Filter_type overloads
    filter_analyse( )
    	: ml::filter_simple( )
    {
	}
    
    virtual ~filter_analyse( )
    { }
    
    // Indicates if the input will enforce a packet decode
    virtual bool requires_image( ) const { return false; }
    
    // This provides the name of the plugin (used in serialisation)
    virtual const pl::wstring get_uri( ) const { return L"analyse"; }
    
protected:
    void do_fetch( ml::frame_type_ptr &result )
	{
		if ( gop_.find( get_position( ) ) == gop_.end( ) )
		{
			// Remove previously cached gop
			gop_.erase( gop_.begin( ), gop_.end( ) );

			// Fetch the current frame
			ml::frame_type_ptr temp;

			if ( inner_fetch( temp, get_position( ), false ) && temp->get_stream( ) )
			{
				std::map< int, ml::stream_type_ptr > streams;
				int start = temp->get_stream( )->key( );
				int index = start;

				// Read the current gop
				while( index < get_frames( ) )
				{
					inner_fetch( temp, index ++ );
					if ( !temp || !temp->get_stream( ) || start != temp->get_stream( )->key( ) ) break;
					int sorted = start + temp->get_stream( )->properties( ).get_property_with_key( ml::analyse_type::key_temporal_reference_ ).value< int >( );
					pl::pcos::property temporal_offset( ml::analyse_type::key_temporal_offset_ );
					temp->get_stream( )->properties( ).append( temporal_offset = boost::int64_t( sorted ) );
					gop_[ temp->get_position( ) ] = temp;
					streams[ sorted ] = temp->get_stream( );
				}

				// Ensure that the recipient has access to the temporally sorted frames too
				for ( std::map< int, ml::frame_type_ptr >::iterator iter = gop_.begin( ); iter != gop_.end( ); ++iter )
				{
					pl::pcos::property temporal_stream( ml::analyse_type::key_temporal_stream_ );
					if ( streams.find( ( *iter ).first ) != streams.end( ) )
						( *iter ).second->properties( ).append( temporal_stream = streams[ ( *iter ).first ] );
				}

				result = gop_[ get_position( ) ];
			}
			else
			{
				result = temp;
			}
		}
		else
		{
			result = gop_[ get_position( ) ];
		}
	}
    
    // The main access point to the filter
    bool inner_fetch( ml::frame_type_ptr &result, int position, bool parse = true )
    {
		fetch_slot( 0 )->seek( position );
		result = fetch_slot( 0 )->fetch( );
		if( !result ) return false;

		if ( !parse ) return true;
		
		ml::stream_type_ptr stream = result->get_stream( );
		
		// If the frame has no stream then there is nothing to analyse.
		if( !stream ) return false;

		if ( !analyse_ )
			analyse_ = ml::analyse_factory( stream );
		
		return analyse_ ? analyse_->analyse( stream ) : false;
	}

	std::map< int, ml::frame_type_ptr > gop_;
	ml::analyse_ptr analyse_;
};

class ML_PLUGIN_DECLSPEC filter_decode : public filter_type, public filter_pool, public boost::enable_shared_from_this< filter_decode >
{
	private:
		boost::recursive_mutex mutex_;
		pl::pcos::property prop_inner_threads_;
		pl::pcos::property prop_filter_;
		pl::pcos::property prop_audio_filter_;
		pl::pcos::property prop_scope_;
		pl::pcos::property prop_source_uri_;
		std::deque< ml::filter_type_ptr > decoder_;
		ml::filter_type_ptr gop_decoder_;
		ml::filter_type_ptr audio_decoder_;
		ml::frame_type_ptr last_frame_;
		cl::profile_ptr codec_to_decoder_;
		bool initialized_;
		std::string video_codec_;
		int total_frames_;
		ml::frame_type_ptr prefetched_frame_;
		boost::int64_t precharged_frames_;
		mutable int estimated_gop_size_;

	public:
		filter_decode( )
			: filter_type( )
			, prop_inner_threads_( pl::pcos::key::from_string( "inner_threads" ) )
			, prop_filter_( pl::pcos::key::from_string( "filter" ) )
			, prop_audio_filter_( pl::pcos::key::from_string( "audio_filter" ) )
			, prop_scope_( pl::pcos::key::from_string( "scope" ) )
			, prop_source_uri_( pl::pcos::key::from_string( "source_uri" ) )
			, decoder_( )
			, codec_to_decoder_( )
			, initialized_( false )
			, total_frames_( 0 )
			, prefetched_frame_( )
			, precharged_frames_( 0 )
			, estimated_gop_size_( 0 )
		{
			properties( ).append( prop_inner_threads_ = 1 );
			properties( ).append( prop_filter_ = pl::wstring( L"mcdecode" ) );
			properties( ).append( prop_audio_filter_ = pl::wstring( L"" ) );
			properties( ).append( prop_scope_ = pl::wstring( cl::str_util::to_wstring( cl::uuid_16b().to_hex_string( ) ) ) );
			properties( ).append( prop_source_uri_ = pl::wstring( L"" ) );
			
			// Load the profile that contains the mappings between codec string and codec filter name
			codec_to_decoder_ = cl::profile_load( "codec_mappings" );
			
			the_shared_filter_pool::Instance( ).add_pool( this );
		}

		virtual ~filter_decode( )
		{
			the_shared_filter_pool::Instance( ).remove_pool( this );
		}

		// Indicates if the input will enforce a packet decode
		virtual bool requires_image( ) const { return false; }

		virtual const pl::wstring get_uri( ) const { return pl::wstring( L"decode" ); }

		virtual const size_t slot_count( ) const { return 1; }

		void sync( )
		{
			total_frames_ = 0;

			if ( gop_decoder_ )
			{
				gop_decoder_->sync( );
				total_frames_ = gop_decoder_->get_frames( );

				// Avoid smeared frames due to incomplete sources
				if ( !fetch_slot( 0 )->complete( ) )
					total_frames_ -= 2;
			}
			else if ( fetch_slot( 0 ) )
			{
				fetch_slot( 0 )->sync( );
				total_frames_ = fetch_slot( 0 )->get_frames( );
			}
		}

	protected:

		ml::filter_type_ptr filter_create( const pl::wstring& codec_filter )
		{
			boost::recursive_mutex::scoped_lock lck( mutex_ );
			ml::input_type_ptr fg = ml::create_input( L"pusher:" );
			fg->property( "length" ) = get_frames( );
			
			// FIXME: Create audio decode filter
			ml::filter_type_ptr decode = ml::create_filter( codec_filter );
			if ( decode->property( "threads" ).valid( ) ) 
				decode->property( "threads" ) = prop_inner_threads_.value< int >( );
			if ( decode->property( "scope" ).valid( ) ) 
				decode->property( "scope" ) = prop_scope_.value< pl::wstring >( );
			if ( decode->property( "source_uri" ).valid( ) ) 
				decode->property( "source_uri" ) = prop_source_uri_.value< pl::wstring >( );
			if ( decode->property( "decode_video" ).valid( ) )
				decode->property( "decode_video" ) = 1;
			
			decode->connect( fg );
			decode->sync( );
			
			return decode;
		}

 		ml::filter_type_ptr filter_obtain( )
		{
			boost::recursive_mutex::scoped_lock lck( mutex_ );
			ml::filter_type_ptr result;
			
			if ( decoder_.size( ) == 0 )
			{
				result = filter_create( prop_filter_.value< pl::wstring >( ) );
				ARENFORCE_MSG( result, "Could not get a valid decoder filter" );
				decoder_.push_back( result );
				ARLOG_INFO( "Creating decoder. This = %1%, source_uri = %2%, scope = %3%" )( this )( prop_source_uri_.value< std::wstring >( ) )( prop_scope_.value< std::wstring >( ) );
			}
			
			result = decoder_.back( );
			decoder_.pop_back( );

			return result;
		}
	
		void filter_release( ml::filter_type_ptr filter )
		{
			boost::recursive_mutex::scoped_lock lck( mutex_ );
			decoder_.push_back( filter );
		}

		bool determine_decode_use( ml::frame_type_ptr &frame )
		{
			if( frame && frame->get_stream() && frame->get_stream( )->estimated_gop_size( ) != 1 )
			{
				ml::filter_type_ptr analyse = ml::filter_type_ptr( new filter_analyse( ) );
				analyse->connect( fetch_slot( 0 ) );
				gop_decoder_ = ml::create_filter( prop_filter_.value< pl::wstring >( ) );
				if ( gop_decoder_->property( "threads" ).valid( ) ) 
					gop_decoder_->property( "threads" ) = prop_inner_threads_.value< int >( );
				if ( gop_decoder_->property( "scope" ).valid( ) ) 
					gop_decoder_->property( "scope" ) = prop_scope_.value< pl::wstring >( );
				if ( gop_decoder_->property( "source_uri" ).valid( ) ) 
					gop_decoder_->property( "source_uri" ) = prop_source_uri_.value< pl::wstring >( );
				if ( gop_decoder_->property( "decode_video" ).valid( ) )
					gop_decoder_->property( "decode_video" ) = 1;
				
				gop_decoder_->connect( analyse );
				gop_decoder_->sync( );
				return true;
			}
			return false;
		}

		void do_fetch( frame_type_ptr &frame )
		{
			if ( last_frame_ == 0 )
			{
				frame = fetch_from_slot( );
				
				// If there is nothing to decode just return frame
				if( !frame->get_stream( ) && !frame->audio_block( ) )
					return;
		
				if( !initialized_ )
				{
					if( frame->get_stream( ) )
						initialize_video( frame );
					
					if( frame->audio_block( ) )
						initialize_audio( frame );
					initialized_ = true;
				}
				
				determine_decode_use( frame );

				//Set last_frame_ here, since get_frames() below needs it to be able
				//to estimate the GOP size.
				last_frame_ = frame->shallow();
			}

			int frameno = get_position( );
			frameno += (int)precharged_frames_;

			if ( gop_decoder_ )
			{
				gop_decoder_->seek( frameno );
				frame = gop_decoder_->fetch( );
			}
			else 
			{
				if ( !frame ) frame = fetch_from_slot( );
				if ( frame && frame->get_stream( ) )
					frame = ml::frame_type_ptr( new frame_lazy( frame, get_frames( ), this ) );
			}
			
			// If the frame has an audio block we need to decode that so pass it to the
			// audio decoder
			if( audio_decoder_ )
			{
				frame_type_ptr audio_frame = perform_audio_decode( frame );
				frame->set_audio( audio_frame->get_audio( ), true );
			}

			last_frame_ = frame->shallow();
		}

		virtual int get_frames( ) const {
			int frames = filter_type::get_frames( );

			ARLOG_DEBUG4( "filter:decode::get_frames() says %1% frames, subtracting %2% precharged frames." )( frames )( precharged_frames_ );

			frames -= precharged_frames_;

			input_type_ptr slot = fetch_slot( 0 );

			if ( slot->has_valid_duration( ) && slot->get_valid_duration( ) != -1 ) {
				frames = slot->get_valid_duration( );
				ARLOG_DEBUG4( "filter:decode::get_frames() found valid_duration property with value %1%, using it." )( frames );
			}

			// Check if the source is complete. If it isn't, we'll
			// subtract a gop from the duration in case of rollout.
			if ( !slot->complete( ) && estimated_gop_size() != 1 )
				frames -= estimated_gop_size( );

			return frames;
		}

		void initialize_video( ml::frame_type_ptr &first_frame )
		{
			ARENFORCE_MSG( codec_to_decoder_, "Need mappings from codec string to name of decoder" );
			ARENFORCE_MSG( first_frame && first_frame->get_stream( ), "No frame or no stream on frame" );

			last_frame_ = first_frame->shallow();

			precharged_frames_ = 0;
			input_type_ptr slot = fetch_slot( 0 );
			if ( slot->has_first_valid_frame( ) )
				precharged_frames_ = slot->get_first_valid_frame( );

			cl::profile::list::const_iterator it = codec_to_decoder_->find( first_frame->get_stream( )->codec( ) );
			ARENFORCE_MSG( it != codec_to_decoder_->end( ), "Failed to find a apropriate codec" )( first_frame->get_stream( )->codec( ) );
			
			ARLOG_DEBUG( "Stream identifier is %1%. Using decode filter %2%" )( first_frame->get_stream( )->codec( ) )( it->value );
			
			prop_filter_ = pl::wstring( cl::str_util::to_wstring( it->value ) );
		}

		void initialize_audio( ml::frame_type_ptr &first_frame )
		{
			// If the user has set the audio_filter property to something meaningfull
			// then we honor that.
			if( prop_audio_filter_.value< pl::wstring >() == L"" )
			{
				ARENFORCE_MSG( codec_to_decoder_, "Need mappings from codec string to name of decoder" );
				ARENFORCE_MSG( first_frame && first_frame->audio_block( ), "No frame or no stream on frame" );
				
				ml::audio::block_type_ptr audio_block = first_frame->audio_block();
				ARENFORCE_MSG( audio_block->tracks.size(), "No tracks in audio block" );
				ARENFORCE_MSG( audio_block->tracks.begin()->second.packets.size(),
							   "No packets in first track in audio block" );
				
				std::string codec = audio_block->tracks.begin()->second.packets.begin( )->second->codec( );
				cl::profile::list::const_iterator it = codec_to_decoder_->find( codec );
				ARENFORCE_MSG( it != codec_to_decoder_->end( ), "Failed to find a apropriate codec" )( codec );
				
				ARLOG_INFO( "Stream itentifier is %1%. Using decode filter %2%" )( codec )( it->value );
				
				prop_audio_filter_ = pl::wstring( cl::str_util::to_wstring( it->value ) );
			}
			
			ARENFORCE_MSG( audio_decoder_ = filter_create( prop_audio_filter_.value< pl::wstring >( ) ),
						   "Failed to create audio decoder filter." )( prop_audio_filter_.value< pl::wstring >( ) );
			if( audio_decoder_->property( "decode_video" ).valid( ) )
				audio_decoder_->property( "decode_video" ) = 0;
		}

	private:
	
		frame_type_ptr perform_audio_decode( const frame_type_ptr& frame )
		{
			input_type_ptr pusher = audio_decoder_->fetch_slot( 0 );
			pusher->property_with_key( key_length_ ) = get_frames( );
			audio_decoder_->sync( );
			pusher->push( frame );
			audio_decoder_->seek( get_position( ) );
			return audio_decoder_->fetch( );
		}
	
		int estimated_gop_size( ) const
		{
			if ( estimated_gop_size_ <= 0 )
			{
				frame_type_ptr frame = last_frame_;
				if ( !frame )
				{
					//We need a frame to be able to draw any conclusions
					//about gop size.
					input_type_ptr input = fetch_slot(0);
					ARENFORCE( input );
					ARENFORCE( frame = input->fetch() );
				}

				stream_type_ptr stream = frame->get_stream( );
				int gop = 0;

				if ( stream )
					gop = stream->estimated_gop_size( );

				if ( gop <= 0 )
				{
					// Guess gop size = framerate / 2
					int fps = ::ceil( frame->fps() );
					gop = fps / 2;
				}

				estimated_gop_size_ = gop;
			}

			return estimated_gop_size_;
		}
};

class ML_PLUGIN_DECLSPEC filter_encode : public filter_encode_type, public filter_pool
{
	private:
		boost::recursive_mutex mutex_;
		pl::pcos::property prop_filter_;
		pl::pcos::property prop_profile_;
		pl::pcos::property prop_instances_;
		pl::pcos::property prop_enable_;
		std::deque< ml::filter_type_ptr > decoder_;
		ml::filter_type_ptr gop_encoder_;
		bool is_long_gop_;
		ml::frame_type_ptr last_frame_;
		cl::profile_ptr profile_to_encoder_mappings_;
		bool stream_validation_;
 
	public:
		filter_encode( )
			: filter_encode_type( )
			, prop_filter_( pl::pcos::key::from_string( "filter" ) )
			, prop_profile_( pl::pcos::key::from_string( "profile" ) )
			, prop_instances_( pl::pcos::key::from_string( "instances" ) )
			, prop_enable_( pl::pcos::key::from_string( "enable" ) )
			, decoder_( )
			, gop_encoder_( )
			, is_long_gop_( false )
			, last_frame_( )
			, profile_to_encoder_mappings_( )
			, stream_validation_( false )
		{
			// Default to something. Will be overriden anyway as soon as we init
			properties( ).append( prop_filter_ = pl::wstring( L"mcencode" ) );
			// Default to something. Should be overriden anyway.
			properties( ).append( prop_profile_ = pl::wstring( L"vcodecs/avcintra100" ) );
			properties( ).append( prop_instances_ = 4 );

			// Enabled by default
			properties( ).append( prop_enable_ = 1 );
			
			the_shared_filter_pool::Instance( ).add_pool( this );
		}

		virtual ~filter_encode( )
		{
			the_shared_filter_pool::Instance( ).remove_pool( this );
		}

		// Indicates if the input will enforce a packet decode
		virtual bool requires_image( ) const { return false; }

		virtual const pl::wstring get_uri( ) const { return pl::wstring( L"encode" ); }

		virtual const size_t slot_count( ) const { return 1; }

	protected:

		ml::filter_type_ptr filter_create( )
		{
			boost::recursive_mutex::scoped_lock lck( mutex_ );
			// Create the encoder that we have mapped from our profile
			ml::filter_type_ptr encode = ml::create_filter( prop_filter_.value< pl::wstring >( ) );
			ARENFORCE_MSG( encode, "Failed to create encoder" )( prop_filter_.value< pl::wstring >( ) );

			// Set the profile property on the encoder
			ARENFORCE_MSG( encode->properties( ).get_property_with_string( "profile" ).valid( ),
				"Encode filter must have a profile property" );
			encode->properties( ).get_property_with_string( "profile" ) = prop_profile_.value< pl::wstring >( );

			if ( encode->properties( ).get_property_with_key( key_instances_ ).valid( ) )
				encode->properties( ).get_property_with_key( key_instances_ ) = prop_instances_.value< int >( );
			
			return encode;
		}

 		ml::filter_type_ptr filter_obtain( )
		{
			boost::recursive_mutex::scoped_lock lck( mutex_ );
			ml::filter_type_ptr result = gop_encoder_;
			if ( !result )
			{
				if ( decoder_.size( ) )
				{
					result = decoder_.back( );
					decoder_.pop_back( );
				}
				else
				{
					result = filter_create( );
				}

				if( result->properties( ).get_property_with_key( key_force_ ).valid( ) ) 
					result->properties( ).get_property_with_key( key_force_ ) = prop_force_.value< int >( );
				if( result->properties( ).get_property_with_key( key_width_ ).valid( ) ) 
					result->properties( ).get_property_with_key( key_width_ ) = prop_width_.value< int >( );
				if( result->properties( ).get_property_with_key( key_height_ ).valid( ) ) 
					result->properties( ).get_property_with_key( key_height_ ) = prop_height_.value< int >( );
				if( result->properties( ).get_property_with_key( key_fps_num_ ).valid( ) ) 
					result->properties( ).get_property_with_key( key_fps_num_ ) = prop_fps_num_.value< int >( );
				if( result->properties( ).get_property_with_key( key_fps_den_ ).valid( ) ) 
					result->properties( ).get_property_with_key( key_fps_den_ ) = prop_fps_den_.value< int >( );
				if( result->properties( ).get_property_with_key( key_sar_num_ ).valid( ) ) 
					result->properties( ).get_property_with_key( key_sar_num_ ) = prop_sar_num_.value< int >( );
				if( result->properties( ).get_property_with_key( key_sar_den_ ).valid( ) ) 
					result->properties( ).get_property_with_key( key_sar_den_ ) = prop_sar_den_.value< int >( );
				if( result->properties( ).get_property_with_key( key_interlace_ ).valid( ) ) 
					result->properties( ).get_property_with_key( key_interlace_ ) = prop_interlace_.value< int >( );
				if( result->properties( ).get_property_with_key( key_bit_rate_ ).valid( ) ) 
					result->properties( ).get_property_with_key( key_bit_rate_ ) = prop_bit_rate_.value< int >( );
			}
			return result;
		}

		void filter_release( ml::filter_type_ptr filter )
		{
			boost::recursive_mutex::scoped_lock lck( mutex_ );
			if ( filter != gop_encoder_ )
				decoder_.push_back( filter );
		}

		void push( ml::input_type_ptr input, ml::frame_type_ptr frame )
		{
			if ( input && frame )
			{
				if ( input->get_uri( ) == L"pusher:" )
					input->push( frame );
				for( size_t i = 0; i < input->slot_count( ); i ++ )
					push( input->fetch_slot( i ), frame );
			}
		}

		bool create_pushers( )
		{
			if( is_long_gop_ )
			{
				ARLOG_DEBUG3( "Creating gop_encoder in encode filter" );
				gop_encoder_ = filter_create( );
				gop_encoder_->connect( fetch_slot( 0 ) );
				
				return gop_encoder_.get( ) != 0;
			}
			return false;
		}

		void do_fetch( frame_type_ptr &frame )
		{
			if ( prop_enable_.value<int>() == 0)
			{
				frame = fetch_from_slot( );
				return;
			}

			int frameno = get_position( );
		
			if ( last_frame_ == 0 )
			{
				initialize_encoder_mapping( );
				create_pushers( );
			}

			if ( last_frame_ && last_frame_->get_position( ) == get_position( ) )
			{
				frame = last_frame_->shallow();
			}
			else if ( gop_encoder_ )
			{
				gop_encoder_->seek( frameno );
				frame = gop_encoder_->fetch( );
			}
			else
			{
				ml::filter_type_ptr graph;
				frame = fetch_from_slot( );
				frame->set_position( get_position( ) );

				// Check the first frame here so that any unspecified properties are picked up at this point
				if ( last_frame_ == 0 )
				{
					matching( frame );
					ARENFORCE_MSG( valid( frame ), "Invalid frame for encoder" );
				}

				bool validate = matching( frame );
				if ( validate && stream_validation_ )
					validate = true;

				frame = ml::frame_type_ptr( new frame_lazy( frame, get_frames( ), this, validate ) );
			}
		
			// Keep a reference to the last frame in case of a duplicated request
			last_frame_ = frame->shallow();
		}

		void initialize_encoder_mapping( )
		{
			// Load the profile that contains the mappings between codec string and codec filter name
			profile_to_encoder_mappings_ = cl::profile_load( "encoder_mappings" );
			
			// First figure out what encoder to use based on our profile
			ARENFORCE_MSG( profile_to_encoder_mappings_, "Need mappings from profile string to name of encoder" );
			
			std::string prof = cl::str_util::to_string( prop_profile_.value< pl::wstring >( ).c_str( ) );
			cl::profile::list::const_iterator it = profile_to_encoder_mappings_->find( prof );
			ARENFORCE_MSG( it != profile_to_encoder_mappings_->end( ), "Failed to find a apropriate encoder" )( prof );

			ARLOG_DEBUG3( "Will use encode filter \"%1%\" for profile \"%2%\"" )( it->value )( prop_profile_.value< pl::wstring >( ) );
			
			prop_filter_ = pl::wstring( cl::str_util::to_wstring( it->value ) );
			
			// Now load the actual profile to find out what video codec string we will produce
			cl::profile_ptr encoder_profile = cl::profile_load( prof );
			ARENFORCE_MSG( encoder_profile, "Failed to load encode profile" )( prof );
			
			cl::profile::list::const_iterator vc_it = encoder_profile->find( "stream_codec_id" );
			ARENFORCE_MSG( vc_it != encoder_profile->end( ), "Profile is missing a stream_codec_id" );

			video_codec_ = vc_it->value;

			vc_it = encoder_profile->find( "video_gop_size" );
			if( vc_it != encoder_profile->end() && vc_it->value != "1" )
			{
				ARLOG_DEBUG3( "Video codec profile %1% is long gop, since video_gop_size is %2%" )( prof )( vc_it->value );
				is_long_gop_ = true;
			}

			vc_it = encoder_profile->find( "stream_validation" );
			stream_validation_ = is_long_gop_ || ( vc_it != encoder_profile->end() && vc_it->value == "1" );
			if ( stream_validation_ )
			{
				ARLOG_DEBUG3( "Encoder plugin is responsible for validating the stream" );
			}
		}
};

class ML_PLUGIN_DECLSPEC filter_rescale : public filter_simple
{
	public:
		filter_rescale( )
			: filter_simple( )
			, prop_enable_( pl::pcos::key::from_string( "enable" ) )
			, prop_progressive_( pl::pcos::key::from_string( "progressive" ) )
			, prop_interp_( pl::pcos::key::from_string( "interp" ) )
			, prop_width_( pl::pcos::key::from_string( "width" ) )
			, prop_height_( pl::pcos::key::from_string( "height" ) )
			, prop_sar_num_( pl::pcos::key::from_string( "sar_num" ) )
			, prop_sar_den_( pl::pcos::key::from_string( "sar_den" ) )
		{
			properties( ).append( prop_enable_ = 1 );
			properties( ).append( prop_progressive_ = 1 );
			properties( ).append( prop_interp_ = 1 );
			properties( ).append( prop_width_ = 640 );
			properties( ).append( prop_height_ = 360 );
			properties( ).append( prop_sar_num_ = 1 );
			properties( ).append( prop_sar_den_ = 1 );
		}

		virtual ~filter_rescale( )
		{
		}

		// Indicates if the input will enforce a packet decode
		virtual bool requires_image( ) const { return false; }

		virtual const pl::wstring get_uri( ) const { return pl::wstring( L"rescale" ); }

		virtual const size_t slot_count( ) const { return 1; }

	protected:

		void do_fetch( frame_type_ptr &frame )
		{
			if ( last_frame_ && last_frame_->get_position( ) == get_position( ) )
			{
				frame = last_frame_->shallow( );
			}
			else
			{
				frame = fetch_from_slot( );
				if ( prop_enable_.value< int >( ) && frame && frame->has_image( ) )
				{
					il::image_type_ptr image = frame->get_image( );
					if ( prop_progressive_.value< int >( ) == 1 )
						image = il::deinterlace( image );
					else if ( prop_progressive_.value< int >( ) == -1 )
						image->set_field_order( il::progressive );
					image = il::rescale( image, prop_width_.value< int >( ), prop_height_.value< int >( ), il::rescale_filter( prop_interp_.value< int >( ) ) );
					frame->set_image( image );
					frame->set_sar( prop_sar_num_.value< int >( ), prop_sar_den_.value< int >( ) );
				}
				last_frame_ = frame->shallow( );
			}
		}

	private:
		pl::pcos::property prop_enable_;
		pl::pcos::property prop_progressive_;
		pl::pcos::property prop_interp_;
		pl::pcos::property prop_width_;
		pl::pcos::property prop_height_;
		pl::pcos::property prop_sar_num_;
		pl::pcos::property prop_sar_den_;
		ml::frame_type_ptr last_frame_;
};

class ML_PLUGIN_DECLSPEC filter_field_order : public filter_simple
{
	public:
		filter_field_order( )
			: filter_simple( )
			, prop_order_( pl::pcos::key::from_string( "order" ) )
		{
			properties( ).append( prop_order_ = 2 );
		}

		virtual ~filter_field_order( )
		{
		}

		// Indicates if the input will enforce a packet decode
		virtual bool requires_image( ) const { return true; }

		virtual const pl::wstring get_uri( ) const { return pl::wstring( L"field_order" ); }

		virtual const size_t slot_count( ) const { return 1; }

	protected:

		void do_fetch( frame_type_ptr &frame )
		{
			frame = fetch_from_slot( );

			if ( prop_order_.value< int >( ) && frame && frame->has_image( ) )
			{
				// Fetch image
				il::image_type_ptr image = frame->get_image( );

				if ( image->field_order( ) != il::progressive && image->field_order( ) != il::field_order_flags( prop_order_.value< int >( ) ) )
				{
					// Do we have a previous frame and is it the one preceding this request?
					if ( !previous_ || previous_->position( ) != get_position( ) - 1 )
					{
						if ( get_position( ) > 0 )
							previous_ = fetch_slot( 0 )->fetch( get_position( ) - 1 )->get_image( );
						else
							previous_ = il::image_type_ptr( );
					}

					// If we have a previous frame, merge the fields accordingly, otherwise duplicate the first field
					if ( previous_ )
					{
						// Image is definitely in the wrong order - think this is correct for both?
						il::image_type_ptr merged;
						if ( prop_order_.value< int >( ) == 2 )
							merged = merge( image, 0, previous_, 1 );
						else
							merged = merge( previous_, 0, image, 1 );
						merged->set_field_order( il::field_order_flags( prop_order_.value< int >( ) ) );
						merged->set_position( get_position( ) );
						frame->set_image( merged );
					}
					else
					{
						il::image_type_ptr merged = merge( image, 0, image, 0 );
						merged->set_field_order( il::field_order_flags( prop_order_.value< int >( ) ) );
						merged->set_position( get_position( ) );
						frame->set_image( merged );
					}

					// We need to remember this frame for reuse on the next request
					previous_ = image;
					previous_->set_position( get_position( ) );
				}
			}
		}

		il::image_type_ptr merge( il::image_type_ptr image1, int scan1, il::image_type_ptr image2, int scan2 )
		{
			// Create a new image which has the same dimensions
			// NOTE: we assume that image dimensions are consistent (should we?)
			il::image_type_ptr result = il::allocate( image1 );

			for ( int i = 0; i < result->plane_count( ); i ++ )
			{
				boost::uint8_t *dst_row = result->data( i );
				boost::uint8_t *src1_row = image1->data( i ) + image1->pitch( i ) * scan1;
				boost::uint8_t *src2_row = image2->data( i ) + image2->pitch( i ) * scan2;
				int dst_linesize = result->linesize( i );
				int dst_stride = result->pitch( i );
				int src_stride = image1->pitch( i ) * 2;
				int height = result->height( i ) / 2;

				while( height -- )
				{
					memcpy( dst_row, src1_row, dst_linesize );
					dst_row += dst_stride;
					src1_row += src_stride;

					memcpy( dst_row, src2_row, dst_linesize );
					dst_row += dst_stride;
					src2_row += src_stride;
				}
			}

			return result;
		}

	private:
		pl::pcos::property prop_order_;
		il::image_type_ptr previous_;
};

class ML_PLUGIN_DECLSPEC filter_lazy : public filter_type, public filter_pool
{
	public:
		filter_lazy( const pl::wstring &spec )
			: filter_type( )
			, spec_( spec )
		{
			filter_ = ml::create_filter( spec_.substr( 5 ) );
			properties_ = filter_->properties( );
			
			the_shared_filter_pool::Instance( ).add_pool( this );
		}

		virtual ~filter_lazy( )
		{
			the_shared_filter_pool::Instance( ).remove_pool( this );
		}

		// Indicates if the input will enforce a packet decode
		virtual bool requires_image( ) const { return false; }

		virtual const pl::wstring get_uri( ) const { return spec_; }

		virtual const size_t slot_count( ) const { return filter_ ? filter_->slot_count( ) : 1; }

		virtual bool is_thread_safe( ) { return filter_ ? filter_->is_thread_safe( ) : false; }

		void on_slot_change( input_type_ptr input, int slot )
		{
			if ( filter_ )
			{
				filter_->connect( input, slot );
				filter_->sync( );
			}
		}

		int get_frames( ) const
		{
			return filter_ ? filter_->get_frames( ) : 0;
		}

	protected:

 		ml::filter_type_ptr filter_obtain( )
		{
			if ( filters_.size( ) == 0 )
				filters_.push_back( filter_create( ) );

			ml::filter_type_ptr result = filters_.back( );
			filters_.pop_back( );

			pcos::key_vector keys = properties_.get_keys( );
			for( pcos::key_vector::iterator it = keys.begin( ); it != keys.end( ); ++it )
			{
				if ( result->property_with_key( *it ).valid( ) )
					result->property_with_key( *it ).set_from_property( properties_.get_property_with_key( *it ) );
				else
				{
					pl::pcos::property prop( *it );
					prop.set_from_property( properties_.get_property_with_key( *it ) );
					result->properties( ).append( prop );
				}
			}

			return result;
		}

		ml::filter_type_ptr filter_create( )
		{
			return ml::create_filter( spec_.substr( 5 ) );
		}

		void filter_release( ml::filter_type_ptr filter )
		{
			filters_.push_back( filter );
		}

		void do_fetch( frame_type_ptr &frame )
		{
			if ( last_frame_ && get_position( ) == last_frame_->get_position( ) )
			{
				frame = last_frame_->shallow();
			}
			else
			{
				frame = fetch_from_slot( 0 );
				if ( filter_ )
				{
					frame = ml::frame_type_ptr( new frame_lazy( frame, get_frames( ), this ) );
				}
			}

			last_frame_ = frame->shallow();
		}

		void sync_frames( )
		{
			if ( filter_ )
				filter_->sync( );
		}

	private:
		boost::recursive_mutex mutex_;
		pl::wstring spec_;
		ml::filter_type_ptr filter_;
		std::deque< ml::filter_type_ptr > filters_;
		ml::frame_type_ptr last_frame_;
};

class ML_PLUGIN_DECLSPEC filter_map_reduce : public filter_simple
{
	private:
		pl::pcos::property prop_threads_;
		long int frameno_;
		int threads_;
		std::map< int, frame_type_ptr > frame_map;
		cl::thread_pool *pool_;
		boost::recursive_mutex mutex_;
		boost::condition_variable_any cond_;
		ml::frame_type_ptr last_frame_;
		int expected_;
		int incr_;
		bool thread_safe_;

	public:
		filter_map_reduce( )
			: filter_simple( )
			, prop_threads_( pl::pcos::key::from_string( "threads" ) )
			, frameno_( 0 )
			, threads_( 4 )
			, pool_( 0 )
			, mutex_( )
			, cond_( )
			, expected_( 0 )
			, incr_( 1 )
			, thread_safe_( false )
		{
			properties( ).append( prop_threads_ = 4 );
		}

		virtual ~filter_map_reduce( )
		{
			if ( pool_ )
			{
				pool_->terminate_all_threads( boost::posix_time::seconds( 5 ) );
				delete pool_;
			}
		}

		// Indicates if the input will enforce a packet decode
		virtual bool requires_image( ) const { return false; }

		virtual const pl::wstring get_uri( ) const { return pl::wstring( L"map_reduce" ); }

		virtual const size_t slot_count( ) const { return 1; }

	protected:

		void decode_job ( ml::frame_type_ptr frame )
		{
			frame->get_image( );
			frame->get_stream( );
			make_available( frame );
		}
 
		bool check_available( const int frameno )
		{
			boost::recursive_mutex::scoped_lock lock( mutex_ );
			return frame_map.find( frameno ) != frame_map.end( );
		}

		ml::frame_type_ptr wait_for_available( const int frameno )
		{
			boost::recursive_mutex::scoped_lock lock( mutex_ );
			/// TODO: timeout and error return - assume that completion will happen for now...
			while( !check_available( frameno ) )
				cond_.wait( lock );
			ml::frame_type_ptr result = frame_map[ frameno ];
			make_unavailable( frameno );
			return result;
		}

		void make_unavailable( const int frameno )
		{
			boost::recursive_mutex::scoped_lock lock( mutex_ );
			if ( check_available( frameno ) )
				frame_map.erase( frame_map.find( frameno ) );
		}

		void make_available( ml::frame_type_ptr frame )
		{
			boost::recursive_mutex::scoped_lock lock( mutex_ );
			frame_map[ frame->get_position( ) ] = frame;
			cond_.notify_all( );
		}

		void add_job( int frameno )
		{
			make_unavailable( frameno );

			ml::input_type_ptr input = fetch_slot( 0 );
			input->seek( frameno );
			ml::frame_type_ptr frame = input->fetch( );

			opencorelib::function_job_ptr fjob( new opencorelib::function_job( boost::bind( &filter_map_reduce::decode_job, this, frame ) ) );
			pool_->add_job( fjob );
		}

		void clear( )
		{
			boost::recursive_mutex::scoped_lock lock( mutex_ );
			frame_map.erase( frame_map.begin( ), frame_map.end( ) );
			pool_->clear_jobs( );
		}

		void do_fetch( frame_type_ptr &frame )
		{
			threads_ = prop_threads_.value< int >( );

			if ( last_frame_ == 0 )
			{
				thread_safe_ = fetch_slot( 0 ) && is_thread_safe( );
				if ( thread_safe_ )
				{
					boost::posix_time::time_duration timeout = boost::posix_time::seconds( 5 );
					pool_ = new cl::thread_pool( threads_, timeout );
				}
			}

			if ( last_frame_ && get_position( ) == last_frame_->get_position( ) )
			{
				frame = last_frame_->shallow();
				return;
			}
			else if ( !thread_safe_ )
			{
				frame = fetch_from_slot( );
				last_frame_ = frame->shallow();
				return;
			}

			if ( last_frame_ == 0 || get_position( ) != expected_ )
			{
				incr_ = get_position( ) >= expected_ ? 1 : -1;
				clear( );
				frameno_ = get_position( );
				for(int i = 0; i < threads_ * 2 && frameno_ < get_frames( ) && frameno_ >= 0; i ++)
				{
					add_job( frameno_ );
					frameno_ += incr_;
				}
			}
		
			// Wait for the requested frame to finish
			frame = wait_for_available( get_position( ) );

			// Add more jobs to the pool
			int fillable = pool_->get_number_of_idle_workers( );

			while ( frameno_ >= 0 && frameno_ < get_frames( ) && fillable -- > 0 && frameno_ <= get_position( ) + 2 * threads_ )
			{
				add_job( frameno_ );
				frameno_ += incr_;
			}

			// Keep a reference to the last frame in case of a duplicated request
			last_frame_ = frame->shallow();
			expected_ = get_position( ) + incr_;
		}
};

//
// Plugin object
//

class ML_PLUGIN_DECLSPEC plugin : public openmedialib_plugin
{
public:
	virtual input_type_ptr input( const pl::wstring & )
	{
		return input_type_ptr( );
	}

	virtual store_type_ptr store( const pl::wstring &, const frame_type_ptr & )
	{
		return store_type_ptr( );
	}

	virtual filter_type_ptr filter( const pl::wstring &spec )
	{
		if ( spec == L"analyse" )
			return filter_type_ptr( new filter_analyse( ) );
		if ( spec == L"decode" )
			return filter_type_ptr( new filter_decode( ) );
		if ( spec == L"encode" )
			return filter_type_ptr( new filter_encode( ) );
		if ( spec == L"field_order" )
			return filter_type_ptr( new filter_field_order( ) );
		if ( spec.find( L"lazy:" ) == 0 )
			return filter_type_ptr( new filter_lazy( spec ) );
		if ( spec == L"map_reduce" )
			return filter_type_ptr( new filter_map_reduce( ) );
		if ( spec == L"rescale" || spec == L"aml_rescale" )
			return filter_type_ptr( new filter_rescale( ) );
		return ml::filter_type_ptr( );
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
		*plug = new ml::decode::plugin;
		return true;
	}
	
	ML_PLUGIN_DECLSPEC void openplugin_destroy_plugin( pl::openplugin* plug )
	{ 
		delete static_cast< ml::decode::plugin * >( plug ); 
	}
}
