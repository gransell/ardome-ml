
// openpluginlib - A plugin interface to openlibraries.

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>
#endif

#include <openpluginlib/pl/timer.hpp>

namespace olib { namespace openpluginlib {

#ifdef GL_EXT_timer_query

gpu_::gpu_( )
	: id_( 0 )
{
}

void gpu_::start( )
{
	glBeginQuery( GL_TIME_ELAPSED_EXT, id_ );
}

void gpu_::stop( )
{
	glEndQuery( GL_TIME_ELAPSED_EXT );
}

gpu_::value_type gpu_::elapsed( ) const
{
	value_type time;

	GLuint64EXT elapsed;
	glGetQueryObjectui64vEXT( id_, GL_TIME_ELAPSED_EXT, &elapsed );

	time.tv_sec  = elapsed / (GLuint64EXT)1e9;
	time.tv_usec = elapsed % (GLuint64EXT)1e9;

	return time;
}
	
bool gpu_::reset( )
{
	if( GLEW_EXT_timer_query )
	{
		if( !glIsQuery( id_ ) )
			glGenQueries( 1, &id_ );

		if( !glIsQuery( id_ ) )
			return false;
	
		return true;
	}

	return false;
}

#endif

} }
