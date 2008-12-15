// pool - A pooled memory allocator

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#ifndef PL_POOL_INC_
#define PL_POOL_INC_

#if _MSC_VER
#pragma warning ( push )
#	pragma warning ( disable:4251 )
#endif

#include <openpluginlib/pl/config.hpp>

#ifndef BOOST_THREAD_DYN_DLL
    #define BOOST_THREAD_DYN_DLL
#endif
#include <boost/thread/mutex.hpp>
#include <vector>

namespace olib { namespace openpluginlib {

class OPENPLUGINLIB_DECLSPEC pool
{
	private:
		// Private constructor and destructor
		pool( );
		~pool( );

		// Singleton accessor and destructor
		static pool *get_instance( );
		static void destroy( );

		// The boost pools
		std::vector< unsigned char * > pools_[ 32 ];
		static boost::mutex mutex_;

	public:
		// Provide a chunk >= size
		static unsigned char *malloc( int size );

		// Reallocate a chunk to a new size
		static unsigned char *realloc( unsigned char *chunk, int size );

		// Return a chunk to the pool
		static void free( unsigned char *chunk );
};

} }

#if _MSC_VER
#	pragma warning ( default:4251 )
#pragma warning ( pop )
#endif

#endif
