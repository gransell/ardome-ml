// Thread Cache - provides a mechanism to cache thread pools

// Copyright (C) 2011 Vizrt
// Released under the LGPL.

#include "precompiled_headers.hpp"
#include "thread_cache.hpp"

namespace olib { namespace opencorelib {

// Acquire a thread pool with the specified number of threads
thread_pool_ptr thread_cache::acquire( size_t threads )
{
	boost::recursive_mutex::scoped_lock lock( mutex_ );
	boost::posix_time::time_duration timeout = boost::posix_time::seconds( 5 );
	thread_pool_ptr result;

	if ( instance( )[ threads ].size( ) )
	{
		result = instance( )[ threads ].top( );
		instance( )[ threads ].pop( );
	}
	else
	{
		result = thread_pool_ptr( new thread_pool( threads, timeout ) );
	}

	return result;
}

// Return a thread pool to the cache to allow reuse
void thread_cache::release( thread_pool_ptr pool )
{
	boost::recursive_mutex::scoped_lock lock( mutex_ );
	if ( pool ) instance( )[ pool->get_number_of_workers( ) ].push( pool );
}

/// Create the instance for this type
thread_cache::instance_type &thread_cache::instance( )
{
	if ( !instance_ )
	{
		instance_ = new instance_type( );
		atexit( destroy );
	}
	return *instance_;
}

/// Destroy the whole cache
void thread_cache::destroy( )
{
	boost::recursive_mutex::scoped_lock lock( mutex_ );
	delete instance_;
	instance_ = 0;
}

boost::recursive_mutex thread_cache::mutex_;
thread_cache::instance_type *thread_cache::instance_ = 0;

} }
