// decode - A decode plugin to ml.
//
// Copyright (C) 2010 Vizrt
// Released under the LGPL.

#include <openmedialib/ml/openmedialib_plugin.hpp>
#include <openmedialib/ml/packet.hpp>
#include <openpluginlib/pl/pcos/observer.hpp>
#include <opencorelib/cl/thread_pool.hpp>
#include <opencorelib/cl/function_job.hpp>

#include <boost/algorithm/string.hpp>

#include <iostream>
#include <cstdlib>
#include <vector>
#include <string>
#include <sstream>

namespace pl = olib::openpluginlib;
namespace ml = olib::openmedialib::ml;
namespace il = olib::openimagelib::il;
namespace pcos = olib::openpluginlib::pcos;
namespace cl = olib::opencorelib;

namespace olib { namespace openmedialib { namespace ml { namespace decode {

class filter_pool
{
	public:
		virtual ~filter_pool( ) { }
		virtual void filter_release( ml::filter_type_ptr ) = 0;
};

class ML_PLUGIN_DECLSPEC frame_lazy : public ml::frame_type 
{
	public:
		/// Constructor
		frame_lazy( const frame_type_ptr &other, filter_pool *pool, ml::filter_type_ptr filter, bool pushed = false )
			: ml::frame_type( *other )
			, parent_( other )
			, pool_( pool )
			, filter_( filter )
			, pushed_( pushed )
		{
		}

		/// Destructor
		virtual ~frame_lazy( )
		{
			if ( filter_ )
				pool_->filter_release( filter_ );
		}

		void evaluate( )
		{
			if ( filter_ )
			{
				ml::frame_type_ptr other = parent_;

				if ( !pushed_ )
				{
					if ( filter_->fetch_slot( 0 ) == 0 )
					{
						ml::input_type_ptr fg = ml::create_input( L"pusher:" );
						fg->property( "length" ) = 1 << 30;
						filter_->connect( fg );
					}

					filter_->fetch_slot( 0 )->push( parent_ );
					filter_->seek( get_position( ) );
					other = filter_->fetch( );
				}

				image_ = other->get_image( );
				alpha_ = other->get_alpha( );
				audio_ = other->get_audio( );
				properties_ = other->properties( );
				stream_ = other->get_stream( );
				pts_ = other->get_pts( );
				duration_ = other->get_duration( );
				sar_num_ = other->get_sar_num( );
				sar_den_ = other->get_sar_den( );
				fps_num_ = other->get_fps_num( );
				fps_den_ = other->get_fps_den( );
				exceptions_ = other->exceptions( );

				pool_->filter_release( filter_ );
				filter_ = ml::filter_type_ptr( );
			}
		}

		/// Indicates if the frame has an image
		virtual bool has_image( )
		{
			return ( filter_ && parent_->has_image( ) ) || image_;
		}

		/// Indicates if the frame has audio
		virtual bool has_audio( )
		{
			return ( filter_ && parent_->has_audio( ) ) || audio_;
		}

		/// Set the image associated to the frame.
		virtual void set_image( olib::openimagelib::il::image_type_ptr image )
		{
			image_ = image;
			if ( filter_ )
				pool_->filter_release( filter_ );
			filter_ = ml::filter_type_ptr( );
		}

		/// Get the image associated to the frame.
		virtual olib::openimagelib::il::image_type_ptr get_image( )
		{
			evaluate( );
			return image_;
		}

		/// Set the audio associated to the frame.
		virtual void set_audio( audio_type_ptr audio )
		{
			audio_ = audio;
		}

		/// Get the audio associated to the frame.
		virtual audio_type_ptr get_audio( )
		{
			evaluate( );
			return audio_;
		}

		virtual stream_type_ptr get_stream( )
		{
			evaluate( );
			return stream_;
		}

	private:
		ml::frame_type_ptr parent_;
		filter_pool *pool_;
		ml::filter_type_ptr filter_;
		bool pushed_;
};

class ML_PLUGIN_DECLSPEC filter_decode : public filter_type, public filter_pool
{
	private:
		boost::recursive_mutex mutex_;
		pl::pcos::property prop_inner_threads_;
		pl::pcos::property prop_filter_;
		std::deque< ml::filter_type_ptr > decoder_;
		ml::filter_type_ptr gop_decoder_;
		ml::frame_type_ptr last_frame_;
 
	public:
		filter_decode( )
			: filter_type( )
			, prop_inner_threads_( pl::pcos::key::from_string( "inner_threads" ) )
			, prop_filter_( pl::pcos::key::from_string( "filter" ) )
			, decoder_( )
		{
			properties( ).append( prop_inner_threads_ = 0 );
			properties( ).append( prop_filter_ = pl::wstring( L"mcdecode" ) );
		}

		~filter_decode( )
		{
		}

		// Indicates if the input will enforce a packet decode
		virtual bool requires_image( ) const { return false; }

		virtual const pl::wstring get_uri( ) const { return pl::wstring( L"decode" ); }

		virtual const size_t slot_count( ) const { return 1; }

	protected:

		ml::filter_type_ptr filter_create( )
		{
			ml::input_type_ptr fg = ml::create_input( L"pusher:" );
			fg->property( "length" ) = 1 << 30;
			ml::filter_type_ptr decode = ml::create_filter( prop_filter_.value< pl::wstring >( ) );
			if ( decode->property( "threads" ).valid( ) ) decode->property( "threads" ) = prop_inner_threads_.value< int >( );
			decode->connect( fg );
			return decode;
		}

 		ml::filter_type_ptr filter_obtain( )
		{
			boost::recursive_mutex::scoped_lock lock( mutex_ );
			ml::filter_type_ptr result;
			if ( decoder_.size( ) == 0 )
				decoder_.push_back( filter_create( ) );
			result = decoder_.back( );
			decoder_.pop_back( );
			return result;
		}

		void filter_release( ml::filter_type_ptr filter )
		{
			boost::recursive_mutex::scoped_lock lock( mutex_ );
			decoder_.push_back( filter );
		}

		bool determine_decode_use( ml::frame_type_ptr &frame )
		{
			bool rc = frame && frame->get_stream( );

			if ( rc && frame->get_stream( )->codec( ) == "mpeg2" )
			{
				gop_decoder_ = ml::create_filter( L"avdecode" );
				gop_decoder_->connect( fetch_slot( 0 ) );
			}

			return rc;
		}
    
		void do_fetch( frame_type_ptr &frame )
		{
			/// TODO: Check for changes in position requests
			if ( last_frame_ == 0 || last_frame_->get_position( ) != get_position( ) )
			{
				int frameno = get_position( );
			
				if ( last_frame_ == 0 )
				{
					frame = fetch_from_slot( );
					determine_decode_use( frame );
				}

				if ( gop_decoder_ )
				{
					gop_decoder_->seek( frameno );
					frame = gop_decoder_->fetch( );
				}
				else
				{
					ml::filter_type_ptr graph = filter_obtain( );
					frame = fetch_from_slot( );
					graph->fetch_slot( 0 )->push( frame );
					graph->seek( get_position( ) );
					frame = graph->fetch( );
					frame = ml::frame_type_ptr( new frame_lazy( frame, this, graph, true ) );
				}
			
				// Keep a reference to the last frame in case of a duplicated request
				last_frame_ = frame;
			}
			else
			{
				frame = last_frame_;
			}
		}
};

class ML_PLUGIN_DECLSPEC filter_encode : public filter_type, public filter_pool
{
	private:
		boost::recursive_mutex mutex_;
		pl::pcos::property prop_filter_;
		std::deque< ml::filter_type_ptr > decoder_;
		ml::filter_type_ptr gop_decoder_;
		ml::frame_type_ptr last_frame_;
 
	public:
		filter_encode( )
			: filter_type( )
			, prop_filter_( pl::pcos::key::from_string( "filter" ) )
			, decoder_( )
		{
			properties( ).append( prop_filter_ = pl::wstring( L"mcencode" ) );
		}

		~filter_encode( )
		{
		}

		// Indicates if the input will enforce a packet decode
		virtual bool requires_image( ) const { return false; }

		virtual const pl::wstring get_uri( ) const { return pl::wstring( L"encode" ); }

		virtual const size_t slot_count( ) const { return 1; }

	protected:

		ml::filter_type_ptr filter_create( )
		{
			ml::filter_type_ptr decode = ml::create_filter( prop_filter_.value< pl::wstring >( ) );
			return decode;
		}

 		ml::filter_type_ptr filter_obtain( )
		{
			boost::recursive_mutex::scoped_lock lock( mutex_ );
			ml::filter_type_ptr result = gop_decoder_;
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
			}
			return result;
		}

		void filter_release( ml::filter_type_ptr filter )
		{
			boost::recursive_mutex::scoped_lock lock( mutex_ );
			if ( filter != gop_decoder_ )
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

		bool create_pushers( ml::frame_type_ptr &frame )
		{
			bool rc = true;

			if ( frame->get_stream( )->codec( ) != "h264" )
			{
				gop_decoder_ = ml::create_filter( L"avdecode" );
				gop_decoder_->connect( fetch_slot( 0 ) );
			}

			return rc;
		}
    
		void do_fetch( frame_type_ptr &frame )
		{
			/// TODO: Check for changes in position requests
			if ( last_frame_ == 0 || last_frame_->get_position( ) != get_position( ) )
			{
				int frameno = get_position( );
			
				if ( last_frame_ == 0 )
				{
					frame = fetch_from_slot( );
					create_pushers( frame );
				}

				if ( gop_decoder_ )
				{
					gop_decoder_->seek( frameno );
					frame = gop_decoder_->fetch( );
				}
				else
				{
					ml::filter_type_ptr graph = filter_obtain( );
					frame = fetch_from_slot( );
					frame = ml::frame_type_ptr( new frame_lazy( frame, this, graph ) );
				}
			
				// Keep a reference to the last frame in case of a duplicated request
				last_frame_ = frame;
			}
			else
			{
				frame = last_frame_;
			}
		}
};

class ML_PLUGIN_DECLSPEC filter_rescale : public filter_type
{
	public:
		filter_rescale( )
			: filter_type( )
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

		~filter_rescale( )
		{
		}
    
		// Indicates if the input will enforce a packet decode
		virtual bool requires_image( ) const { return false; }

		virtual const pl::wstring get_uri( ) const { return pl::wstring( L"rescale" ); }

		virtual const size_t slot_count( ) const { return 1; }

	protected:

		void do_fetch( frame_type_ptr &frame )
		{
			frame = fetch_from_slot( );
			if ( prop_enable_.value< int >( ) && frame && frame->has_image( ) )
			{
				il::image_type_ptr image = frame->get_image( );
				if ( prop_progressive_.value< int >( ) )
					image = il::deinterlace( image );
				image = il::rescale( image, prop_width_.value< int >( ), prop_height_.value< int >( ), il::rescale_filter( prop_interp_.value< int >( ) ) );
				frame->set_image( image );
				frame->set_sar( prop_sar_num_.value< int >( ), prop_sar_den_.value< int >( ) );
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
};

class ML_PLUGIN_DECLSPEC filter_lazy : public filter_type, public filter_pool
{
	public:
		filter_lazy( const pl::wstring &spec )
			: filter_type( )
			, spec_( spec )
		{
			filter_ = filter_create( );
			properties_ = filter_->properties( );
		}

		~filter_lazy( )
		{
		}

		// Indicates if the input will enforce a packet decode
		virtual bool requires_image( ) const { return false; }

		virtual const pl::wstring get_uri( ) const { return spec_; }

		virtual const size_t slot_count( ) const { return filter_ ? filter_->slot_count( ) : 1; }

	protected:

 		ml::filter_type_ptr filter_obtain( )
		{
			boost::recursive_mutex::scoped_lock lock( mutex_ );

			if ( filters_.size( ) == 0 )
				filters_.push_back( filter_create( ) );

			ml::filter_type_ptr result = filters_.back( );
			filters_.pop_back( );

			pcos::key_vector keys = properties_.get_keys( );
			for( pcos::key_vector::iterator it = keys.begin( ); it != keys.end( ); it ++ )
				if ( result->property_with_key( *it ).valid( ) )
					result->property_with_key( *it ).set_from_property( properties_.get_property_with_key( *it ) );

			return result;
		}

		ml::filter_type_ptr filter_create( )
		{
			return ml::create_filter( spec_.substr( 5 ) );
		}

		void filter_release( ml::filter_type_ptr filter )
		{
			boost::recursive_mutex::scoped_lock lock( mutex_ );
			filters_.push_back( filter );
		}

		void do_fetch( frame_type_ptr &frame )
		{
			frame = fetch_from_slot( 0 );
			if ( filter_ )
			{
				ml::filter_type_ptr filter = filter_obtain( );
				frame = ml::frame_type_ptr( new frame_lazy( frame, this, filter ) );
			}
		}

	private:
		boost::recursive_mutex mutex_;
		pl::wstring spec_;
		ml::filter_type_ptr filter_;
		std::deque< ml::filter_type_ptr > filters_;
};

class ML_PLUGIN_DECLSPEC filter_map_reduce : public filter_type
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
        
	public:
		filter_map_reduce( )
			: filter_type( )
			, prop_threads_( pl::pcos::key::from_string( "threads" ) )
			, frameno_( 0 )
			, threads_( 4 )
			, pool_( 0 )
			, mutex_( )
			, cond_( )
		{
			properties( ).append( prop_threads_ = 4 );
		}

		~filter_map_reduce( )
		{
		}

		// Indicates if the input will enforce a packet decode
		virtual bool requires_image( ) const { return false; }

		virtual const pl::wstring get_uri( ) const { return pl::wstring( L"map_reduce" ); }

		virtual const size_t slot_count( ) const { return 1; }

	protected:

		void decode_job ( ml::frame_type_ptr frame )
		{
			frame->get_image( );
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
			/// TODO: Check for changes in position requests
			if ( last_frame_ == 0 || last_frame_->get_position( ) != get_position( ) )
			{
				threads_ = prop_threads_.value< int >( );

				if ( last_frame_ == 0 )
				{
					boost::posix_time::time_duration timeout = boost::posix_time::seconds( 5 );
					pool_ = new cl::thread_pool( threads_, timeout );
				}

				if ( last_frame_ == 0 || get_position( ) != last_frame_->get_position( ) + 1 )
				{
					clear( );
					frameno_ = get_position( );
					for(int i = 0; i < threads_ * 3 && frameno_ < get_frames( ); i ++)
						add_job( frameno_ ++ );
				}
			
				// Wait for the requested frame to finish
				frame = wait_for_available( get_position( ) );
  
				// Add more jobs to the pool
				int fillable = threads_ * 2 - pool_->jobs_pending( );

				if ( frameno_ < get_frames( ) && fillable -- > 0 )
					add_job( frameno_ ++ );

				// Keep a reference to the last frame in case of a duplicated request
				last_frame_ = frame;
			}
			else
			{
				frame = last_frame_;
			}
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
		if ( spec == L"decode" )
			return filter_type_ptr( new filter_decode( ) );
		if ( spec == L"encode" )
			return filter_type_ptr( new filter_encode( ) );
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
