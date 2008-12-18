
// openpluginlib - A plugin interface to openlibraries.

// Copyright (C) 2005-2006 Visual Media FX Ltd.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#include <iostream>

#include <openpluginlib/pl/timer.hpp>

namespace opl = olib::openpluginlib;

int main( )
{
	typedef opl::gettimeofday_default_timer::value_type value_type;
	opl::gettimeofday_default_timer r;
	
	r.reset( );
	std::cout << "gettimeofday: ticks per microsecond " << r.ticks( ) << std::endl;

	struct timespec one_second;
	one_second.tv_sec  = 1;
	one_second.tv_nsec = 0;

	r.start( );
	nanosleep( &one_second, NULL );
	r.stop( );
	value_type elapsed = r.elapsed( );

	std::cout << "gettimeofday: elapsed time is " << elapsed.tv_sec << " seconds and " << elapsed.tv_usec << " microseconds.\n";
}
