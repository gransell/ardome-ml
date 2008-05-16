
// openpluginlib - A plugin interface to openlibraries.

// Copyright (C) 2007 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#ifdef HAVE_CONFIG_H
#include <openlibraries_global_config.hpp>
#endif

#ifndef HAVE_FAST_MATH
#include <cmath>
#endif

#include <openpluginlib/pl/fast_math.hpp>

namespace olib { namespace openpluginlib {

// precomputed constants.
const float two_to_pow_23			= 1 << 23;
const float one_over_two_to_pow_23	= 1.0f / ( 1 << 23 );

int fast_floorf( float x )
{
#if defined( HAVE_FAST_MATH ) && !defined( __ppc )
	double y = ( x + 68719476736.0 * 1.5 );

	return ( int ) ( ( *( ( long long* ) &y ) ) >> 16 );
#else
	return ( int ) floorf( x );
#endif
}

float fast_log2f( float v )
{
	float x = ( float ) *( int* ) &v;
	x *= one_over_two_to_pow_23;
	x = x - 127.0f;

	float y = x - fast_floorf( x );
	y = ( y - y * y ) * 0.346607f;

	return x + y;
}

float fast_exp2f( float v )
{
	float y = v - fast_floorf( v );
	y = ( y - y * y ) * 0.33971f;

	float x = v + 127.0f - y;
	x *= two_to_pow_23;
	*( int* ) &x = ( int ) x;

	return x;
}

float fast_powf( float x, float y )
{
#if defined( HAVE_FAST_MATH )
	return fast_exp2f( y * fast_log2f( x ) );
#else
	return powf( x, y );
#endif
}

} }
