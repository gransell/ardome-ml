// Thread Cache - provides a mechanism to cache thread pools

// Copyright (C) 2011 Vizrt
// Released under the LGPL.

#ifndef CORE_THREAD_CACHE_H_
#define CORE_THREAD_CACHE_H_

#include <stack>
#include <map>
#include <boost/thread.hpp>

#include "export_defines.hpp"
#include "thread_pool.hpp"

namespace olib { namespace opencorelib {

/// Provides a means to cache and reuse allocated thread pool objects.

class CORE_API thread_cache
{
	public:
		typedef std::stack< thread_pool_ptr > pool;
		typedef std::map< size_t, pool > instance_type;

		/// Acquire a thread pool with the specified number of threads
		static thread_pool_ptr acquire( size_t threads );

		/// Return a thread pool to the cache to allow reuse
		static void release( thread_pool_ptr pool );

	private:
		/// Create the instance for this type
		static instance_type &instance( );

		/// Destroy the whole cache
		static void destroy( );

		static boost::recursive_mutex mutex_;
		static instance_type *instance_;
};

} }

#endif
