
// Copyright (C) 2011 Vizrt
// Released under the LGPL.

#ifndef OPENCORELIB_THREAD_H_
#define OPENCORELIB_THREAD_H_

#include <boost/thread.hpp>
#include <boost/thread/recursive_mutex.hpp>

#include "export_defines.hpp"

namespace olib { namespace opencorelib {

namespace thread {

class CORE_API scoped_unlock
{
public:
	scoped_unlock( boost::recursive_mutex::scoped_lock& lock )
	: lock_( lock )
	{
		lock_.unlock( );
	}
	~scoped_unlock( )
	{
		lock_.lock( );
	}

private:
	boost::recursive_mutex::scoped_lock& lock_;
};

}

} }

#endif

