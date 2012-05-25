// Thread Monitor - provides a mechanism to poll objects periodically

// Copyright (C) 2011 Vizrt
// Released under the LGPL.

#ifndef CORE_THREAD_MONITOR_H_
#define CORE_THREAD_MONITOR_H_

#include <boost/thread.hpp>

#include "export_defines.hpp"
#include "worker.hpp"

namespace olib { namespace opencorelib {

/// Provides a means to poll objects periodically - specifically intended for 
/// the management of thread pools, but free for other types of use.

class CORE_API thread_monitor
{
	public:
		/// Enroll a job to be executed periodically
		static void enroll( const base_job_ptr &job, boost::posix_time::time_duration timeout = boost::posix_time::seconds( 1 ) );

		/// Withdraw a previously enrolled job
		static void withdraw( const base_job_ptr &job );

		/// Destroy the whole cache
		static void destroy( );

	private:
		/// Create the instance for this type
		static worker *instance( );

		static boost::recursive_mutex mutex_;
		static worker *instance_;
};

} }

#endif
