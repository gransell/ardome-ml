// Thread Monitor - provides a mechanism to poll objects periodically

// Copyright (C) 2011 Vizrt
// Released under the LGPL.

#include "precompiled_headers.hpp"
#include "thread_monitor.hpp"

namespace olib { namespace opencorelib {

/// Enroll a job to be executed periodically
void thread_monitor::enroll( const base_job_ptr &job, boost::posix_time::time_duration timeout )
{
	instance( )->add_reoccurring_job( job, timeout );
}

/// Withdraw a previously enrolled job
void thread_monitor::withdraw( const base_job_ptr &job )
{
	instance( )->remove_reoccurring_job( job );
}

/// Create the instance for this type
worker *thread_monitor::instance( )
{
	boost::recursive_mutex::scoped_lock lock( mutex_ );
	if ( !instance_ )
	{
		instance_ = new worker( );
		instance_->start( );
		atexit( destroy );
	}
	return instance_;
}

/// Destroy the whole cache
void thread_monitor::destroy( )
{
	boost::recursive_mutex::scoped_lock lock( mutex_ );
	delete instance_;
	instance_ = 0;
}

boost::recursive_mutex thread_monitor::mutex_;
worker *thread_monitor::instance_ = 0;

} }


