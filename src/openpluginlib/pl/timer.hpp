
// openpluginlib - A plugin interface to openlibraries.

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#ifndef TIMER_INC_
#define TIMER_INC_

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#pragma warning ( push )
#	pragma warning ( disable:4201 )
#include <windows.h>
#include <mmsystem.h>
#	pragma warning ( default:4201 )
#pragma warning ( pop )
#else
#include <sys/time.h>
#include <time.h>
#endif

#if !defined( GCC_VERSION ) && defined( __GNUC__ )
#define GCC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
#endif

#ifdef HAVE_GL_GLEW_H
#include <GL/glew.h>
#endif

#include <openpluginlib/pl/config.hpp>

namespace olib { namespace openpluginlib {

struct time_value
{	
#ifdef WIN32
	typedef __int64	ticks_type;
#else
	typedef unsigned long long int ticks_type;
#endif

	ticks_type tv_sec;
	ticks_type tv_usec;
};

#ifdef WIN32
template<class time_value_policy>
struct sleep
{
	void snooze( const time_value_policy& t ) const
	{
		// ??? will it fit? we're API restricted here... - Goncalo
		DWORD milli = static_cast<DWORD>( t.tv_sec * 1000 + t.tv_usec / 1000 );
		Sleep( milli );
	}
};

template<class time_value_policy>
class event_sleep
{
public:
	explicit event_sleep( )
#ifdef UNICODE
		: event_( CreateEvent( NULL, FALSE, FALSE, L"" ) )
#else
		: event_( CreateEvent( NULL, FALSE, FALSE, "" ) )
#endif
	{ }
	
	~event_sleep( )
	{ CloseHandle( event_ ); }
	
public:
	void snooze( const time_value_policy& t ) const
	{
		// ??? will it fit? we're API restricted here... - Goncalo
		DWORD milli = static_cast<DWORD>( t.tv_sec * 1000 + t.tv_usec / 1000 );
		WaitForSingleObjectEx( event_, milli, FALSE );
	}
	
private:
	HANDLE event_;
};
#else
template<class time_value_policy>
struct nanosleep_
{
	void snooze( const time_value_policy& t ) const
	{
		struct timespec s_time;

		s_time.tv_sec  = t.tv_sec;
		s_time.tv_nsec = t.tv_usec * 1000;
		
		nanosleep( &s_time, NULL );
	}
};
#endif

template<class time_value_policy, template<class> class sleep_policy>
class rdtsc : public time_value_policy, public sleep_policy<time_value_policy>
{
public:
	typedef time_value_policy	 					value_type;
	typedef typename time_value_policy::ticks_type	ticks_type;

public:
	explicit rdtsc( )
		: start_( 0 )
		, stop_( 0 )
		, ticks_per_microsec_( 1 )
	{ reset( ); }

public:
	void start( ) { start_ = rdtsc_( ); }
	void stop( )  { stop_  = rdtsc_( ); }

	value_type elapsed( ) const
	{
		value_type time;
		
		time.tv_sec  = ( ( stop_ - start_ ) / ticks_per_microsec_ ) / 1000000;
		time.tv_usec = ( ( stop_ - start_ ) / ticks_per_microsec_ ) - time.tv_sec * 1000000;
		
		return time;
	}
	
	void snooze( const value_type& val )
	{ sleep_policy<time_value_policy>::snooze( val ); }
	
	ticks_type ticks( ) const { return ticks_per_microsec_; }
	
	bool reset( )
	{
		// TODO: do proper detection of rdtsc.
		ticks_per_microsec_ = estimate_ticks_per_microsec( );
		return true;
	}
	
private:
#ifdef WIN32
	__int64 estimate_ticks_per_microsec( )
	{ return with_mmtimer( ); }
	
	__int64 with_mmtimer( )
	{
		DWORD t_start, t_stop;
		__int64 r1, r2;
	
		timeBeginPeriod( 1 );
	
		t_start = timeGetTime( );
		for( ;; )
		{
			t_stop = timeGetTime( );
			if( ( t_stop - t_start ) > 0 )
			{
				r1 = rdtsc_( );
				break;
			}
		}
	
		t_start = t_stop;

		for( ;; )
		{
			t_stop = timeGetTime( );
			if( ( t_stop - t_start ) > 100 )
			{
				r2 = rdtsc_( );
				break;
			}
		}
	
		timeEndPeriod( 1 );
	
		DWORD elapsed = t_stop - t_start;
		__int64 diff  = r2 - r1;
	
		return diff / ( elapsed * 1000 );
	}
	__int64 rdtsc_( ) const
	{
		union integer64
		{
			__int32 i32[ 2 ];
			__int64 i64;
		};
		
		integer64 i64;

		_asm
		{
			RDTSC
			mov i64.i32[ 0 ], eax
			mov i64.i32[ 4 ], edx
		}
		
		return i64.i64;
	}
#else
	ticks_type estimate_ticks_per_microsec( )
	{ return with_gettimeofday( ); }
	
	ticks_type with_gettimeofday( )
	{
		struct timeval t_start, t_stop;
		ticks_type r1, r2;
	
		gettimeofday( &t_start, NULL );
		for( ;; )
		{
			gettimeofday( &t_stop, NULL );

			ticks_type elapsed_usec = ( t_stop.tv_usec - t_start.tv_usec ) - ( t_stop.tv_sec  - t_start.tv_sec ) * 1000000;
			if( elapsed_usec > 0 )
			{
				r1 = rdtsc_( );
				break;
			}
		}
	
		t_start = t_stop;

		for( ;; )
		{
			gettimeofday( &t_stop, NULL );

			ticks_type elapsed_usec = ( t_stop.tv_usec - t_start.tv_usec ) - ( t_stop.tv_sec  - t_start.tv_sec ) * 1000000;
			if( elapsed_usec > 200 )
			{
				r2 = rdtsc_( );
				break;
			}
		}
	
		ticks_type elapsed = ( t_stop.tv_usec - t_start.tv_usec ) - ( t_stop.tv_sec  - t_start.tv_sec ) * 1000000;
		ticks_type diff    = r2 - r1;
	
		return diff / elapsed;
	}

	ticks_type rdtsc_( ) const
	{
		ticks_type x;
		
		__asm__ volatile ( ".byte 0x0f, 0x31" : "=A" ( x ) );
		
		return x;
	}	
#endif
private:
	ticks_type start_;
	ticks_type stop_;
	ticks_type ticks_per_microsec_;
};

#ifndef WIN32
template<class time_value_policy, template<class> class sleep_policy>
class gettimeofday_ : public time_value_policy, public sleep_policy<time_value_policy>
{
public:
	typedef time_value_policy	 					value_type;
	typedef typename time_value_policy::ticks_type	ticks_type;

public:
	void start( ) { gettimeofday( &start_, NULL ); }
	void stop( )  { gettimeofday( &stop_, NULL ); }

	value_type elapsed( ) const
	{
		value_type time;
		
		time.tv_sec  = stop_.tv_sec  - start_.tv_sec;
		time.tv_usec = stop_.tv_usec - start_.tv_usec;
		
		return time;
	}
	
	void snooze( const value_type& val )
	{ sleep_policy<time_value_policy>::snooze( val ); }
	
	ticks_type ticks( ) const { return 1; }
	
	bool reset( )
	{ return true; }

private:
	struct timeval start_;
	struct timeval stop_;
};
#endif

#ifdef GL_EXT_timer_query
class OPENPLUGINLIB_DECLSPEC gpu_
{
public:
	typedef time_value  value_type;
	typedef GLuint64EXT ticks_type;

public:
	explicit gpu_( );

	void start( );
	void stop( );

	value_type elapsed( ) const;

	bool reset( );

	ticks_type ticks( ) const { return 1; }

private:
	GLuint id_;
};
#endif

template<class timer_policy>
struct timer : public timer_policy
{
	typedef typename timer_policy::value_type value_type;

	void		start( )						{ timer_policy::start( ); }
	void		stop( )							{ timer_policy::stop( );  }
	void		sleep( const value_type& val )	{ timer_policy::snooze( val );  }
	value_type	elapsed( ) const				{ return timer_policy::elapsed( ); }
	bool		reset( )						{ return timer_policy::reset( );   }
};

#ifdef WIN32
	typedef timer<rdtsc<time_value, event_sleep> > 			rdtsc_default_timer;
#else
#	if GCC_VERSION >= 40000 && !defined __APPLE__
		typedef timer<rdtsc<time_value, nanosleep_> > 		rdtsc_default_timer;
#	endif
	typedef timer<gettimeofday_<time_value, nanosleep_> >	gettimeofday_default_timer;
#endif

#ifdef WIN32
#	pragma comment( lib, "winmm.lib" )
#endif

} }

#endif
