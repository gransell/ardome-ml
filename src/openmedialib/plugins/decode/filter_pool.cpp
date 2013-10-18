// decode - A decode plugin to ml.
//
// Copyright (C) 2010 Vizrt
// Released under the LGPL.

#include "filter_pool.hpp"
#include <opencorelib/cl/enforce_defines.hpp>

namespace pl = olib::openpluginlib;
namespace ml = olib::openmedialib::ml;
namespace il = olib::openmedialib::ml;
namespace pcos = olib::openpluginlib::pcos;
namespace cl = olib::opencorelib;

namespace olib { namespace openmedialib { namespace ml { namespace decode {

ml::filter_type_ptr shared_filter_pool::filter_obtain( filter_pool *pool_token )
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

void shared_filter_pool::filter_release( ml::filter_type_ptr filter, filter_pool *pool_token )
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

void shared_filter_pool::add_pool( filter_pool *pool )
{
	boost::recursive_mutex::scoped_lock lck( mtx_ );
	pools_.insert( pool );
}

void shared_filter_pool::remove_pool( filter_pool * pool )
{
	boost::recursive_mutex::scoped_lock lck( mtx_ );
	pools_.erase( pool );
}
		

} } } }
