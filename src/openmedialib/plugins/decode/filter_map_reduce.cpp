// decode - A decode plugin to ml.
//
// Copyright (C) 2010 Vizrt
// Released under the LGPL.

#include "filter_map_reduce.hpp"
#include <opencorelib/cl/function_job.hpp>

namespace olib { namespace openmedialib { namespace ml { namespace decode {

filter_map_reduce::filter_map_reduce( )
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

filter_map_reduce::~filter_map_reduce( )
{
	if ( pool_ )
	{
		pool_->terminate_all_threads( boost::posix_time::seconds( 5 ) );
		delete pool_;
	}
}

// Indicates if the input will enforce a packet decode
bool filter_map_reduce::requires_image( ) const { return false; }

const std::wstring filter_map_reduce::get_uri( ) const { return std::wstring( L"map_reduce" ); }

const size_t filter_map_reduce::slot_count( ) const { return 1; }

void filter_map_reduce::decode_job ( ml::frame_type_ptr frame )
{
	frame->get_image( );
	frame->get_stream( );
	make_available( frame );
}

bool filter_map_reduce::check_available( const int frameno )
{
	boost::recursive_mutex::scoped_lock lock( mutex_ );
	return frame_map.find( frameno ) != frame_map.end( );
}

ml::frame_type_ptr filter_map_reduce::wait_for_available( const int frameno )
{
	boost::recursive_mutex::scoped_lock lock( mutex_ );
	/// TODO: timeout and error return - assume that completion will happen for now...
	while( !check_available( frameno ) )
		cond_.wait( lock );
	ml::frame_type_ptr result = frame_map[ frameno ];
	make_unavailable( frameno );
	return result;
}

void filter_map_reduce::make_unavailable( const int frameno )
{
	boost::recursive_mutex::scoped_lock lock( mutex_ );
	if ( check_available( frameno ) )
		frame_map.erase( frame_map.find( frameno ) );
}

void filter_map_reduce::make_available( ml::frame_type_ptr frame )
{
	boost::recursive_mutex::scoped_lock lock( mutex_ );
	frame_map[ frame->get_position( ) ] = frame;
	cond_.notify_all( );
}

void filter_map_reduce::add_job( int frameno )
{
	make_unavailable( frameno );

	ml::input_type_ptr input = fetch_slot( 0 );
	input->seek( frameno );
	ml::frame_type_ptr frame = input->fetch( );

	opencorelib::function_job_ptr fjob( new opencorelib::function_job( boost::bind( &filter_map_reduce::decode_job, this, frame ) ) );
	pool_->add_job( fjob );
}

void filter_map_reduce::clear( )
{
	boost::recursive_mutex::scoped_lock lock( mutex_ );
	frame_map.erase( frame_map.begin( ), frame_map.end( ) );
	pool_->clear_jobs( );
}

void filter_map_reduce::do_fetch( frame_type_ptr &frame )
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

} } } }

