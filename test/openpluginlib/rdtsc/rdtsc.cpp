
// openpluginlib - A plugin interface to openlibraries.

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <time.h>
#endif

#include <iostream>

#include <openpluginlib/pl/timer.hpp>

namespace opl = olib::openpluginlib;

int main( )
{
#if defined WIN32 || ( GCC_VERSION >= 40000 && !defined __APPLE__ )
	typedef opl::rdtsc_default_timer::value_type value_type;
	opl::rdtsc_default_timer r;
	
	r.reset( );
	std::cout << "rdtsc: ticks per microsecond " << r.ticks( ) << std::endl;

#ifndef WIN32
	struct timespec one_second;
	one_second.tv_sec  = 1;
	one_second.tv_nsec = 0;
#endif

	r.start( );

#ifdef WIN32
	Sleep( 1000 );
#else
	nanosleep( &one_second, NULL );
#endif
	
	r.stop( );
	value_type elapsed = r.elapsed( );

	std::cout << "rdtsc: elapsed time is " << elapsed.tv_sec << " seconds and " << elapsed.tv_usec << " microseconds.\n";
#endif
}
