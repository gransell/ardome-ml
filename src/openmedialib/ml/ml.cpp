
// ml - A media library representation.

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#include <openmedialib/ml/ml.hpp>
#include <opencorelib/cl/thread_monitor.hpp>
#include <opencorelib/cl/thread_cache.hpp>

namespace cl = olib::opencorelib;

namespace olib { namespace openmedialib { namespace ml {

ML_DECLSPEC bool init( )
{ return true; }

ML_DECLSPEC bool uninit( )
{
	indexer_shutdown( );
	cl::thread_monitor::destroy( );
	cl::thread_cache::destroy( );
	return true; 
}

} } }

