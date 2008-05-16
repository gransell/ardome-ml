
// openpluginlib - An plugin interface to openlibraries.

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#ifndef GEOMETRY_INC_
#define GEOMETRY_INC_

#ifdef WIN32
#define _USE_MATH_DEFINES
#endif

#include <algorithm>

#include <cmath>
#include <cstddef>

namespace olib { namespace openpluginlib {

namespace detail
{
	template<class T, size_t N>
	class vector
	{
	public:
		typedef T 			value_type;
		typedef T*			iterator;
		typedef const T*	const_iterator;
		typedef T& 			reference;
		typedef const T&	const_reference;
		typedef size_t		size_type;

		explicit vector( T v = T( ) )
		{ std::fill( x, x + N, v ); }
			
		explicit vector( T y[ N ] )
		{
#if _MSC_VER >= 1400
			std::copy( y, y + N, stdext::checked_array_iterator<T*>( x, N ) );
#else
			std::copy( y, y + N, x );
#endif
		}
			
		vector( const vector<T, N>& y )
		{
#if _MSC_VER >= 1400
			std::copy( y.x, y.x + N, stdext::checked_array_iterator<T*>( x, N ) );
#else
			std::copy( y.x, y.x + N, x );
#endif
		}
			
		vector& operator=( const vector<T, N>& y )
		{
			vector<T, N>( y ).swap( *this );

			return *this;
		}
			
		void swap( vector<T, N>& y )
		{ std::swap_ranges( x, x + N, y.x ); }
			
		T operator[ ]( size_type i ) const
		{ return x[ i ]; }

		T& operator[ ]( size_type i )
		{ return x[ i ]; }
			
		value_type x[ N ];
	};
	
	template<class T, size_t N>
	void swap( vector<T, N>& x, vector<T, N>& y )
	{ x.swap( y ); }
}

// specialisation for 2D vectors
template<class T>
class vector_2
{
public:
	typedef T 		value_type;
	typedef size_t	size_type;

	explicit vector_2( T x = T( ), T y = T( ) )
	{ x_[ 0 ] = x; x_[ 1 ] = y; }

	T operator[ ]( size_type i ) const
	{ return x_[ i ]; }

	T& operator[ ]( size_type i )
	{ return x_[ i ]; }

	T get( size_t i ) const
	{ return x_[ i ]; }
	void set( size_t i, T x )
	{ x_[ i ] = x; }

private:
	detail::vector<T, 2> x_;
};
		
template<class T>
vector_2<T> operator+( const vector_2<T>& x, const vector_2<T>& y )
{
	vector_2<T> tmp( x );

	tmp[ 0 ] += y[ 0 ];
	tmp[ 1 ] += y[ 1 ];
			
	return tmp;
}

template<class T>
vector_2<T> operator+=( vector_2<T>& x, const vector_2<T>& y )
{			
	return x = x + y;
}
		
template<class T>
vector_2<T> operator-( const vector_2<T>& x, const vector_2<T>& y )
{
	vector_2<T> tmp( x );

	tmp[ 0 ] -= y[ 0 ];
	tmp[ 1 ] -= y[ 1 ];
			
	return tmp;
}
		
template<class T>
vector_2<T> operator-( const vector_2<T>& x )
{
	vector_2<T> tmp( x );

	tmp[ 0 ] = -tmp[ 0 ];
	tmp[ 1 ] = -tmp[ 1 ];
			
	return tmp;
}

template<class T>
vector_2<T> operator-=( vector_2<T>& x, const vector_2<T>& y )
{			
	return x = x - y;
}

template<class T, class R>
vector_2<T> operator*( const vector_2<T>& x, R s )
{
	vector_2<T> y( x );

	y[ 0 ] *= s;
	y[ 1 ] *= s;

	return y;
}

template<class T, class R>
vector_2<T> operator*( R s, const vector_2<T>& x )
{
	return x * s;
}

template<class T, class R>
vector_2<T>& operator*=( vector_2<T>& x, R s )
{
	return x = x * s;
}

template<class T, class R>
vector_2<T> operator/( const vector_2<T>& x, R s )
{
	return x * ( T( 1.0f ) / s );
}

template<class T, class R>
vector_2<T>& operator/=( vector_2<T>& x, R s )
{
	return x = x / s;
}

// specialisation for 3D vectors
template<class T>
class vector_3
{
public:
	typedef T 		value_type;
	typedef size_t	size_type;
			
	explicit vector_3( T x = T( ), T y = T( ), T z = T( ) )
		: x_( )
	{ x_[ 0 ] = x; x_[ 1 ] = y; x_[ 2 ] = z; }

	T operator[ ]( size_type i ) const
	{ return x_[ i ]; }

	T& operator[ ]( size_type i )
	{ return x_[ i ]; }

	T get( size_t i ) const
	{ return x_[ i ]; }
	void set( size_t i, T x )
	{ x_[ i ] = x; }
			
private:
	detail::vector<T, 3> x_;
};
		
template<class T>
vector_3<T> operator+( const vector_3<T>& x, const vector_3<T>& y )
{
	vector_3<T> tmp( x );

	tmp[ 0 ] += y[ 0 ];
	tmp[ 1 ] += y[ 1 ];
	tmp[ 2 ] += y[ 2 ];
			
	return tmp;
}

template<class T>
vector_3<T> operator+=( vector_3<T>& x, const vector_3<T>& y )
{			
	return x = x + y;
}
		
template<class T>
vector_3<T> operator-( const vector_3<T>& x, const vector_3<T>& y )
{
	vector_3<T> tmp( x );

	tmp[ 0 ] -= y[ 0 ];
	tmp[ 1 ] -= y[ 1 ];
	tmp[ 2 ] -= y[ 2 ];
			
	return tmp;
}
		
template<class T>
vector_3<T> operator-( const vector_3<T>& x )
{
	vector_3<T> tmp( x );

	tmp[ 0 ] = -tmp[ 0 ];
	tmp[ 1 ] = -tmp[ 1 ];
	tmp[ 2 ] = -tmp[ 2 ];
			
	return tmp;
}

template<class T>
vector_3<T> operator-=( vector_3<T>& x, const vector_3<T>& y )
{			
	return x = x - y;
}
		
template<class T, class R>
vector_3<T> operator*( const vector_3<T>& x, R s )
{
	vector_3<T> y( x );

	y[ 0 ] *= s;
	y[ 1 ] *= s;
	y[ 2 ] *= s;

	return y;
}

template<class T, class R>
vector_3<T> operator*( R s, const vector_3<T>& x )
{
	return x * s;
}

template<class T, class R>
vector_3<T>& operator*=( vector_3<T>& x, R s )
{
	return x = x * s;
}

template<class T, class R>
vector_3<T> operator/( const vector_3<T>& x, R s )
{
	return x * ( T( 1.0f ) / s );
}

template<class T, class R>
vector_3<T>& operator/=( vector_3<T>& x, R s )
{
	return x = x / s;
}

template<class T>
float dot_product( const vector_3<T>& x, const vector_3<T>& y )
{ return x[ 0 ] * y[ 0 ] + x[ 1 ] * y[ 1 ] + x[ 2 ] * y[ 2 ]; }
		
template<class T>
vector_3<T> cross_product( const vector_3<T>& x, const vector_3<T>& y )
{
	vector_3<T> tmp;
 
	tmp[ 0 ] = x[ 1 ] * y[ 2 ] - x[ 2 ] * y[ 1 ];
	tmp[ 1 ] = x[ 2 ] * y[ 0 ] - x[ 0 ] * y[ 2 ];
	tmp[ 2 ] = x[ 0 ] * y[ 1 ] - x[ 1 ] * y[ 0 ];
			
	return tmp;
}
		
template<class T>
vector_3<T> normalize( const vector_3<T>& x )
{
	vector_3<T> tmp( x );
			
	float inv_dot = 1.0f / sqrtf( dot_product( x, x ) );
	tmp[ 0 ] *= inv_dot;
	tmp[ 1 ] *= inv_dot;
	tmp[ 2 ] *= inv_dot;
			
	return tmp;
}

template<class T>
bool operator<( const vector_3<T>& x, const vector_3<T>& y )
{
	if( x[ 0 ] == y[ 0 ] )
	{
		if( x[ 1 ] == y[ 1 ] )
		{
			if ( x[ 2 ] == y[ 2 ] )
				return false;
			else
				return x[ 2 ] < y[ 2 ];
		}
		else
		{
			return x[ 1 ] < y[ 1 ];
		}
	}
	else
	{
		return x[ 0 ] < y[ 0 ];
	}
}

// specialisation for 4D vectors
template<class T>
class vector_4
{
public:
	typedef T 		value_type;
	typedef size_t	size_type;
			
	explicit vector_4( T x = T( ), T y = T( ), T z = T( ), T w = T( ) )
	{ x_[ 0 ] = x; x_[ 1 ] = y; x_[ 2 ] = z; x_[ 3 ] = w; }

	T operator[ ]( size_type i ) const
	{ return x_[ i ]; }

	T& operator[ ]( size_type i )
	{ return x_[ i ]; }

	T get( size_t i ) const
	{ return x_[ i ]; }
	void set( size_t i, T x )
	{ x_[ i ] = x; }
			
private:
	detail::vector<T, 4> x_;
};
		
template<class T>
vector_4<T> operator+( const vector_4<T>& x, const vector_4<T>& y )
{
	vector_4<T> tmp( x );

	tmp[ 0 ] += y[ 0 ];
	tmp[ 1 ] += y[ 1 ];
	tmp[ 2 ] += y[ 2 ];
	tmp[ 3 ] += y[ 3 ];
			
	return tmp;
}

template<class T>
vector_4<T> operator+=( vector_4<T>& x, const vector_4<T>& y )
{			
	return x = x + y;
}
		
template<class T>
vector_4<T> operator-( const vector_4<T>& x, const vector_4<T>& y )
{
	vector_4<T> tmp( x );

	tmp[ 0 ] -= y[ 0 ];
	tmp[ 1 ] -= y[ 1 ];
	tmp[ 2 ] -= y[ 2 ];
	tmp[ 3 ] -= y[ 3 ];
			
	return tmp;
}
		
template<class T>
vector_4<T> operator-( const vector_4<T>& x )
{
	vector_4<T> tmp( x );

	tmp[ 0 ] = -tmp[ 0 ];
	tmp[ 1 ] = -tmp[ 1 ];
	tmp[ 2 ] = -tmp[ 2 ];
	tmp[ 3 ] = -tmp[ 3 ];
			
	return tmp;
}

template<class T>
vector_4<T> operator-=( vector_4<T>& x, const vector_4<T>& y )
{			
	return x = x - y;
}
		
template<class T, class R>
vector_4<T> operator*( const vector_4<T>& x, R s )
{
	vector_4<T> y( x );

	y[ 0 ] *= s;
	y[ 1 ] *= s;
	y[ 2 ] *= s;
	y[ 3 ] *= s;

	return y;
}

template<class T, class R>
vector_4<T> operator*( R s, const vector_4<T>& x )
{
	return x * s;
}

template<class T, class R>
vector_4<T>& operator*=( vector_4<T>& x, R s )
{
	return x = x * s;
}

template<class T, class R>
vector_4<T> operator/( const vector_4<T>& x, R s )
{
	return x * ( T( 1.0f ) / s );
}

template<class T, class R>
vector_4<T>& operator/=( vector_4<T>& x, R s )
{
	return x = x / s;
}

typedef vector_2<double>	vec2d;
typedef vector_2<float>		vec2f;
typedef vector_3<double>	vec3d;
typedef vector_3<float>		vec3f;
typedef vector_4<double>	vec4d;
typedef vector_4<float>		vec4f;

template<class T>
class color
{
public:
	typedef T 		value_type;
	typedef size_t	size_type;
			
	explicit color( T r = T( ), T g = T( ), T b = T( ) )
	{ x_[ 0 ] = r; x_[ 1 ] = g; x_[ 2 ] = b; }

	T operator[ ]( size_type i ) const
	{ return x_[ i ]; }
	T& operator[ ]( size_type i )
	{ return x_[ i ]; }

	T get( size_t i ) const
	{ return x_[ i ]; }
	void set( size_t i, T x )
	{ x_[ i ] = x; }

private:
	detail::vector<T, 3> x_;
};

template<class T, class R>
color<T> operator*( const color<T>& x, R s )
{
	color<T> y( x );

	y[ 0 ] *= s;
	y[ 1 ] *= s;
	y[ 2 ] *= s;

	return y;
}

template<class T, class R>
color<T> operator*( R s, const color<T>& x )
{
	return x * s;
}

template<class T>
class color_rgba
{
public:
	typedef T 		value_type;
	typedef size_t	size_type;
			
	explicit color_rgba( T r = T( ), T g = T( ), T b = T( ), T a = T( ) )
	{ x_[ 0 ] = r; x_[ 1 ] = g; x_[ 2 ] = b; x_[ 3 ] = a; }

	T operator[ ]( size_type i ) const
	{ return x_[ i ]; }

	T& operator[ ]( size_type i )
	{ return x_[ i ]; }

	T get( size_t i ) const
	{ return x_[ i ]; }
	void set( size_t i, T x )
	{ x_[ i ] = x; }
			
private:
	detail::vector<T, 4> x_;
};

typedef color<float> color3f;

template<class T>
class axis_angle
{
public:
	typedef T 		value_type;
	typedef size_t	size_type;
			
	explicit axis_angle( T x = T( ), T y = T( ), T z = T( 1.0f ), T a = T( ) )
		: x_( )
	{ x_[ 0 ] = x; x_[ 1 ] = y; x_[ 2 ] = z; x_[ 3 ] = a; }

	T operator[ ]( size_type i ) const
	{ return x_[ i ]; }

	T& operator[ ]( size_type i )
	{ return x_[ i ]; }
			
private:
	detail::vector<T, 4> x_;
};

template<class T>
class matrix_4x4
{
public:
	enum { N = 16 };
	
	typedef T 			value_type;
	typedef T*			iterator;
	typedef const T*	const_iterator;
	typedef T& 			reference;
	typedef const T&	const_reference;
	typedef size_t		size_type;

	explicit matrix_4x4( T v = T( ) )
	{ std::fill( a, a + N, v ); }
			
	explicit matrix_4x4( T b[ N ] )
	{
#if _MSC_VER >= 1400
		std::copy( b, b + N, stdext::checked_array_iterator<T*>( a, N ) );
#else
		std::copy( b, b + N, a );
#endif // _MSC_VER >= 1400
	}
			
	matrix_4x4( const matrix_4x4<T>& b )
	{ 
#if _MSC_VER >= 1400
		std::copy( b.a, b.a + N, stdext::checked_array_iterator<T*>( a, N ) );
#else
		std::copy( b.a, b.a + N, a );
#endif // _MSC_VER >= 1400
	}
			
	matrix_4x4<T>& operator=( const matrix_4x4<T>& b )
	{
		matrix_4x4<T>( b ).swap( *this );
				
		return *this;
	}
			
	T operator[ ]( size_type i ) const
	{ return a[ i ]; }
	T& operator[ ]( size_type i )
	{ return a[ i ]; }
	
	T operator( )( int i, int j ) const
	{ return a[ i * 4 + j ]; }
	T& operator( )( int i, int j )
	{ return a[ i * 4 + j ]; }

	T get( int i, int j ) const
	{ return a[ i * 4 + j ]; }
	void set( int i, int j, T value )
	{ a[ i * 4 + j ] = value; }
	
	void swap( matrix_4x4<T>& b )
	{ std::swap_ranges( a, a + N, b.a ); }
			
	T a[ N ];
};
		
template<class T>
void swap( matrix_4x4<T>& a, matrix_4x4<T>& b )
{ a.swap( b ); }
		
template<class T>
matrix_4x4<T> make_identity( )
{
	matrix_4x4<T> c;
		
	std::fill( c.a, c.a + c.N, T( 0.0f ) );
	c[ 0 ] = c[ 5 ] = c[ 10 ] = c[ 15 ] = T( 1.0f );
			
	return c;
}

template<class T>
matrix_4x4<T> make_transpose( const matrix_4x4<T>& a )
{
	matrix_4x4<T> c;

	c[ 0  ] = a[ 0 ], c[ 1  ] = a[ 4 ], c[ 2  ] = a[ 8  ], c[ 3  ] = a[ 12 ];
	c[ 4  ] = a[ 1 ], c[ 5  ] = a[ 5 ], c[ 6  ] = a[ 9  ], c[ 7  ] = a[ 13 ];
	c[ 8  ] = a[ 2 ], c[ 9  ] = a[ 6 ], c[ 10 ] = a[ 10 ], c[ 11 ] = a[ 14 ];
	c[ 12 ] = a[ 3 ], c[ 13 ] = a[ 7 ], c[ 14 ] = a[ 11 ], c[ 15 ] = a[ 15 ];
			
	return c;
}
		
template<class T>
matrix_4x4<T> make_scale( const vector_3<T>& s )
{
	matrix_4x4<T> c;
		
	c = make_identity<T>( );
			
	c[ 0 ]	= s[ 0 ];
	c[ 5 ]	= s[ 1 ];
	c[ 10 ] = s[ 2 ];
		
	return c;
}

template<class T>
matrix_4x4<T> make_scale( const vector_4<T>& s )
{
	matrix_4x4<T> c;
		
	c = make_identity<T>( );
			
	c[ 0 ]	= s[ 0 ];
	c[ 5 ]	= s[ 1 ];
	c[ 10 ] = s[ 2 ];
	c[ 15 ] = s[ 3 ];
		
	return c;
}

template<class T>
matrix_4x4<T> make_scale( T a, T b, T c, T d )
{
	matrix_4x4<T> s;
		
	s = make_identity<T>( );
			
	s[ 0 ]	= a;
	s[ 5 ]	= b;
	s[ 10 ] = c;
	s[ 15 ] = d;
		
	return s;
}

template<class T>
matrix_4x4<T> make_translation( const vector_3<T>& t )
{
	matrix_4x4<T> c;
			
	c = make_identity<T>( );
			
	c[ 12 ]	= t[ 0 ];
	c[ 13 ]	= t[ 1 ];
	c[ 14 ] = t[ 2 ];
			
	return c;
}

template<class T>
matrix_4x4<T> make_translation( T a, T b, T c )
{
	matrix_4x4<T> t;
			
	t = make_identity<T>( );
			
	t[ 12 ]	= a;
	t[ 13 ]	= b;
	t[ 14 ] = c;
			
	return t;
}

template<class T>
matrix_4x4<T> from_axis_angle_rotation( const axis_angle<T>& a )
{
	// TODO: use compile time decision to dispatch to the appropriate trigonometric function.
			
	T cos_radians			= T( cos( a[ 3 ] ) );
	T sin_radians			= T( sin( a[ 3 ] ) );
	T one_minus_cos_radians = 1.0f - cos_radians;
			
	T inv_magnitude = T( 1.0f ) / T( sqrt( a[ 0 ] * a[ 0 ] + a[ 1 ] * a[ 1 ] + a[ 2 ] * a[ 2 ] ) );
			
	T x = a[ 0 ] * inv_magnitude;
	T y = a[ 1 ] * inv_magnitude;
	T z = a[ 2 ] * inv_magnitude;
			
	matrix_4x4<T> c;
			
	c[ 0  ] = cos_radians + one_minus_cos_radians * x * x;
	c[ 4  ] = one_minus_cos_radians * x * y - sin_radians * z;
	c[ 8  ] = one_minus_cos_radians * x * z + sin_radians * y;
	c[ 12 ] = 0.0f;
			
	c[ 1  ] = one_minus_cos_radians * x * y + sin_radians * z;
	c[ 5  ] = cos_radians + one_minus_cos_radians * y * y;
	c[ 9  ] = one_minus_cos_radians * y * z - sin_radians * x;
	c[ 13 ] = 0.0f;
			
	c[ 2  ] = one_minus_cos_radians * x * z - sin_radians * y;
	c[ 6  ] = one_minus_cos_radians * y * z + sin_radians * x;
	c[ 10 ] = cos_radians + one_minus_cos_radians * z * z;
	c[ 14 ] = 0.0f;

	c[ 3  ] = 0.0f;
	c[ 7  ] = 0.0f;
	c[ 11 ] = 0.0f;
	c[ 15 ] = 1.0f;
			
	return c;
}

template<class T>
matrix_4x4<T> inverse( const matrix_4x4<T>& a )
{
	T x00, x01, x02, x03;
	T x10, x11, x12, x13;
	T x20, x21, x22, x23;
	T x30, x31, x32, x33;
	T y01, y02, y03;
	T y12, y13, y23;
	T z00, z01, z02, z03;
	T z10, z11, z12, z13;
	T z20, z21, z22, z23;
	T z30, z31, z32, z33;
	
	x00 = a( 0, 0 ), x01 = a( 0, 1 );
	x10 = a( 1, 0 ), x11 = a( 1, 1 );
	x20 = a( 2, 0 ), x21 = a( 2, 1 );
	x30 = a( 3, 0 ), x31 = a( 3, 1 );
	
	y01 = x00 * x11 - x10 * x01;
	y02 = x00 * x21 - x20 * x01;
	y03 = x00 * x31 - x30 * x01;
	y12 = x10 * x21 - x20 * x11;
	y13 = x10 * x31 - x30 * x11;
	y23 = x20 * x31 - x30 * x21;
	
	x02 = a( 0, 2 ), x03 = a( 0, 3 );
	x12 = a( 1, 2 ), x13 = a( 1, 3 );
	x22 = a( 2, 2 ), x23 = a( 2, 3 );
	x32 = a( 3, 2 ), x33 = a( 3, 3 );
	
	z33 = x02 * y12 - x12 * y02 + x22 * y01;
	z23 = x12 * y03 - x32 * y01 - x02 * y13;
	z13 = x02 * y23 - x22 * y03 + x32 * y02;
	z03 = x22 * y13 - x32 * y12 - x12 * y23;
	z32 = x13 * y02 - x23 * y01 - x03 * y12;
	z22 = x03 * y13 - x13 * y03 + x33 * y01;
	z12 = x23 * y03 - x33 * y02 - x03 * y23;
	z02 = x13 * y23 - x23 * y13 + x33 * y12;
	
	y01 = x02 * x13 - x12 * x03;
	y02 = x02 * x23 - x22 * x03;
	y03 = x02 * x33 - x32 * x03;
	y12 = x12 * x23 - x22 * x13;
	y13 = x12 * x33 - x32 * x13;
	y23 = x22 * x33 - x32 * x23;
	
	x00 = a( 0, 0 );
	x01 = a( 0, 1 );
	x10 = a( 1, 0 );
	x11 = a( 1, 1 );
	x20 = a( 2, 0 );
	x21 = a( 2, 1 );
	x30 = a( 3, 0 );
	x31 = a( 3, 1 );

	z30 = x11 * y02 - x21 * y01 - x01 * y12;
	z20 = x01 * y13 - x11 * y03 + x31 * y01;
	z10 = x21 * y03 - x31 * y02 - x01 * y23;
	z00 = x11 * y23 - x21 * y13 + x31 * y12;
	z31 = x00 * y12 - x10 * y02 + x20 * y01;
	z21 = x10 * y03 - x30 * y01 - x00 * y13;
	z11 = x00 * y23 - x20 * y03 + x30 * y02;
	z01 = x20 * y13 - x30 * y12 - x10 * y23;

	T inv_det = T( 1.0f ) / ( x30 * z30 + x20 * z20 + x10 * z10 + x00 * z00 );

	matrix_4x4<T> r;
	
	r( 0, 0 ) = z00 * inv_det, r( 0, 1 ) = z10 * inv_det, r( 0, 2 ) = z20 * inv_det, r( 0, 3 ) = z30 * inv_det;
	r( 1, 0 ) = z01 * inv_det, r( 1, 1 ) = z11 * inv_det, r( 1, 2 ) = z21 * inv_det, r( 1, 3 ) = z31 * inv_det;
	r( 2, 0 ) = z02 * inv_det, r( 2, 1 ) = z12 * inv_det, r( 2, 2 ) = z22 * inv_det, r( 2, 3 ) = z32 * inv_det;
	r( 3, 0 ) = z03 * inv_det, r( 3, 1 ) = z13 * inv_det, r( 3, 2 ) = z23 * inv_det, r( 3, 3 ) = z33 * inv_det;
	
	return r;
}

template<class T>
matrix_4x4<T> operator+( const matrix_4x4<T>& a, const matrix_4x4<T>& b )
{
	matrix_4x4<T> c;

	c[ 0  ] = a[ 0  ] + b[ 0  ], c[ 1  ] = a[ 1  ] + b[ 1  ], c[ 2  ] = a[ 2  ] + b[ 2  ], c[ 3  ] = a[ 3  ] + b[ 3  ];
	c[ 4  ] = a[ 4  ] + b[ 4  ], c[ 5  ] = a[ 5  ] + b[ 5  ], c[ 6  ] = a[ 6  ] + b[ 6  ], c[ 7  ] = a[ 7  ] + b[ 7  ];
	c[ 8  ] = a[ 8  ] + b[ 8  ], c[ 9  ] = a[ 9  ] + b[ 9  ], c[ 10 ] = a[ 10 ] + b[ 10 ], c[ 11 ] = a[ 11 ] + b[ 11 ];
	c[ 12 ] = a[ 12 ] + b[ 12 ], c[ 13 ] = a[ 13 ] + b[ 13 ], c[ 14 ] = a[ 14 ] + b[ 14 ], c[ 15 ] = a[ 15 ] + b[ 15 ];

	return c;
}

template<class T>
matrix_4x4<T>& operator+=( matrix_4x4<T>& a, const matrix_4x4<T>& b )
{
	return a = a + b;
}

template<class T>
matrix_4x4<T> operator-( const matrix_4x4<T>& a, const matrix_4x4<T>& b )
{
	matrix_4x4<T> c;

	c[ 0  ] = a[ 0  ] - b[ 0  ], c[ 1  ] = a[ 1  ] - b[ 1  ], c[ 2  ] = a[ 2  ] - b[ 2  ], c[ 3  ] = a[ 3  ] - b[ 3  ];
	c[ 4  ] = a[ 4  ] - b[ 4  ], c[ 5  ] = a[ 5  ] - b[ 5  ], c[ 6  ] = a[ 6  ] - b[ 6  ], c[ 7  ] = a[ 7  ] - b[ 7  ];
	c[ 8  ] = a[ 8  ] - b[ 8  ], c[ 9  ] = a[ 9  ] - b[ 9  ], c[ 10 ] = a[ 10 ] - b[ 10 ], c[ 11 ] = a[ 11 ] - b[ 11 ];
	c[ 12 ] = a[ 12 ] - b[ 12 ], c[ 13 ] = a[ 13 ] - b[ 13 ], c[ 14 ] = a[ 14 ] - b[ 14 ], c[ 15 ] = a[ 15 ] - b[ 15 ];

	return c;
}

template<class T>
matrix_4x4<T>& operator-=( matrix_4x4<T>& a, const matrix_4x4<T>& b )
{
	return a = a - b;
}

template<class T>
matrix_4x4<T> operator-( const matrix_4x4<T>& a )
{
	matrix_4x4<T> b;
	
	b[ 0  ] = -a[ 0  ], b[ 1  ] = -a[ 1  ], b[ 2  ] = -a[ 2  ], b[ 3  ] = -a[ 3  ];
	b[ 4  ] = -a[ 4  ], b[ 5  ] = -a[ 5  ], b[ 6  ] = -a[ 6  ], b[ 7  ] = -a[ 7  ];
	b[ 8  ] = -a[ 8  ], b[ 9  ] = -a[ 9  ], b[ 10 ] = -a[ 10 ], b[ 11 ] = -a[ 11 ];
	b[ 12 ] = -a[ 12 ], b[ 13 ] = -a[ 13 ], b[ 14 ] = -a[ 14 ], b[ 15 ] = -a[ 15 ];
	
	return b;
}

template<class T>
matrix_4x4<T> operator*( const matrix_4x4<T>& a, const matrix_4x4<T>& b )
{
	matrix_4x4<T> c;

	c[ 0  ] = a[ 0 ] * b[ 0  ] + a[ 4 ] * b[ 1  ] + a[ 8  ] * b[ 2  ] + a[ 12 ] * b[ 3  ];
	c[ 1  ] = a[ 1 ] * b[ 0  ] + a[ 5 ] * b[ 1  ] + a[ 9  ] * b[ 2  ] + a[ 13 ] * b[ 3  ];
	c[ 2  ] = a[ 2 ] * b[ 0  ] + a[ 6 ] * b[ 1  ] + a[ 10 ] * b[ 2  ] + a[ 14 ] * b[ 3  ];
	c[ 3  ] = a[ 3 ] * b[ 0  ] + a[ 7 ] * b[ 1  ] + a[ 11 ] * b[ 2  ] + a[ 15 ] * b[ 3  ];
 
	c[ 4  ] = a[ 0 ] * b[ 4  ] + a[ 4 ] * b[ 5  ] + a[ 8  ] * b[ 6  ] + a[ 12 ] * b[ 7  ];
	c[ 5  ] = a[ 1 ] * b[ 4  ] + a[ 5 ] * b[ 5  ] + a[ 9  ] * b[ 6  ] + a[ 13 ] * b[ 7  ];
	c[ 6  ] = a[ 2 ] * b[ 4  ] + a[ 6 ] * b[ 5  ] + a[ 10 ] * b[ 6  ] + a[ 14 ] * b[ 7  ];
	c[ 7  ] = a[ 3 ] * b[ 4  ] + a[ 7 ] * b[ 5  ] + a[ 11 ] * b[ 6  ] + a[ 15 ] * b[ 7  ];

	c[ 8  ] = a[ 0 ] * b[ 8  ] + a[ 4 ] * b[ 9  ] + a[ 8  ] * b[ 10 ] + a[ 12 ] * b[ 11 ];
	c[ 9  ] = a[ 1 ] * b[ 8  ] + a[ 5 ] * b[ 9  ] + a[ 9  ] * b[ 10 ] + a[ 13 ] * b[ 11 ];
	c[ 10 ] = a[ 2 ] * b[ 8  ] + a[ 6 ] * b[ 9  ] + a[ 10 ] * b[ 10 ] + a[ 14 ] * b[ 11 ];
	c[ 11 ] = a[ 3 ] * b[ 8  ] + a[ 7 ] * b[ 9  ] + a[ 11 ] * b[ 10 ] + a[ 15 ] * b[ 11 ];

	c[ 12 ] = a[ 0 ] * b[ 12 ] + a[ 4 ] * b[ 13 ] + a[ 8  ] * b[ 14 ] + a[ 12 ] * b[ 15 ];
	c[ 13 ] = a[ 1 ] * b[ 12 ] + a[ 5 ] * b[ 13 ] + a[ 9  ] * b[ 14 ] + a[ 13 ] * b[ 15 ];
	c[ 14 ] = a[ 2 ] * b[ 12 ] + a[ 6 ] * b[ 13 ] + a[ 10 ] * b[ 14 ] + a[ 14 ] * b[ 15 ];
	c[ 15 ] = a[ 3 ] * b[ 12 ] + a[ 7 ] * b[ 13 ] + a[ 11 ] * b[ 14 ] + a[ 15 ] * b[ 15 ];

	return c;
}

template<class T>
matrix_4x4<T>& operator*=( matrix_4x4<T>& a, const matrix_4x4<T>& b )
{
	return a = a * b;
}

template<class T, class R>
matrix_4x4<T> operator*( const matrix_4x4<T>& a, R s )
{
	matrix_4x4<T> c;
	
	c[ 0  ] = a[ 0  ] * s, c[ 1  ] = a[ 1  ] * s, c[ 2  ] = a[ 2  ] * s, c[ 3  ] = a[ 3  ] * s;
	c[ 4  ] = a[ 4  ] * s, c[ 5  ] = a[ 5  ] * s, c[ 6  ] = a[ 6  ] * s, c[ 7  ] = a[ 7  ] * s;
	c[ 8  ] = a[ 8  ] * s, c[ 9  ] = a[ 9  ] * s, c[ 10 ] = a[ 10 ] * s, c[ 11 ] = a[ 11 ] * s;
	c[ 12 ] = a[ 12 ] * s, c[ 13 ] = a[ 13 ] * s, c[ 14 ] = a[ 14 ] * s, c[ 15 ] = a[ 15 ] * s;

	return c;
}

template<class T, class R>
matrix_4x4<T> operator*( R s, const matrix_4x4<T>& a )
{
	return a * s;
}

template<class T, class R>
matrix_4x4<T>& operator*=( matrix_4x4<T>& a, R s )
{
	return a = a * s;
}

template<class T, class R>
matrix_4x4<T> operator/( const matrix_4x4<T>& a, R s )
{
	T r = T( 1.0f ) / T( s );

	return a * r;
}

template<class T, class R>
matrix_4x4<T>& operator/=( matrix_4x4<T>& a, R s )
{
	return a = a / s;
}

template<class T>
vector_4<T> operator*( const matrix_4x4<T>& a, const vector_4<T>& b )
{
	vector_4<T> v;

	v[ 0 ] = a[ 0 ] * b[ 0 ] + a[ 4 ] * b[ 1 ] + a[ 8  ] * b[ 2 ] + a[ 12 ] * b[ 3 ];
	v[ 1 ] = a[ 1 ] * b[ 0 ] + a[ 5 ] * b[ 1 ] + a[ 9  ] * b[ 2 ] + a[ 13 ] * b[ 3 ];
	v[ 2 ] = a[ 2 ] * b[ 0 ] + a[ 6 ] * b[ 1 ] + a[ 10 ] * b[ 2 ] + a[ 14 ] * b[ 3 ];
	v[ 3 ] = a[ 3 ] * b[ 0 ] + a[ 7 ] * b[ 1 ] + a[ 11 ] * b[ 2 ] + a[ 15 ] * b[ 3 ];

	return v;
}

typedef matrix_4x4<double>	matrixd;
typedef matrix_4x4<float>	matrixf;

template<class T>
class quaternion
{
public:
	typedef T 		value_type;
	typedef size_t	size_type;
			
	explicit quaternion( T x = T( ), T y = T( ), T z = T( 1.0f ), T w = T( ) )
		: x_( )
	{ x_[ 0 ] = x; x_[ 1 ] = y; x_[ 2 ] = z; x_[ 3 ] = w; }

	T operator[ ]( size_type i ) const
	{ return x_[ i ]; }

	T& operator[ ]( size_type i )
	{ return x_[ i ]; }
			
private:
	detail::vector<T, 4> x_;
};

template<class T>
quaternion<T> operator+( const quaternion<T>& x, const quaternion<T>& y )
{
	quaternion<T> tmp( x );

	tmp[ 0 ] += y[ 0 ];
	tmp[ 1 ] += y[ 1 ];
	tmp[ 2 ] += y[ 2 ];
	tmp[ 3 ] += y[ 3 ];
			
	return tmp;
}

template<class T>
quaternion<T> operator+=( quaternion<T>& x, const quaternion<T>& y )
{			
	return x = x + y;
}
		
template<class T>
quaternion<T> operator-( const quaternion<T>& x, const quaternion<T>& y )
{
	quaternion<T> tmp( x );

	tmp[ 0 ] -= y[ 0 ];
	tmp[ 1 ] -= y[ 1 ];
	tmp[ 2 ] -= y[ 2 ];
	tmp[ 3 ] -= y[ 3 ];
			
	return tmp;
}
		
template<class T>
quaternion<T> operator-( const quaternion<T>& x )
{
	quaternion<T> tmp( x );

	tmp[ 0 ] = -tmp[ 0 ];
	tmp[ 1 ] = -tmp[ 1 ];
	tmp[ 2 ] = -tmp[ 2 ];
	tmp[ 3 ] = -tmp[ 3 ];
			
	return tmp;
}

template<class T>
quaternion<T> operator-=( quaternion<T>& x, const quaternion<T>& y )
{			
	return x = x - y;
}
		
template<class T>
quaternion<T> operator*( const quaternion<T>& x, T s )
{
	quaternion<T> y( x );

	y[ 0 ] *= s;
	y[ 1 ] *= s;
	y[ 2 ] *= s;
	y[ 3 ] *= s;

	return y;
}

template<class T>
quaternion<T> operator*( T s, const quaternion<T>& x )
{
	return x * s;
}

template<class T>
quaternion<T>& operator*=( quaternion<T>& x, T s )
{
	return x = x * s;
}

template<class T>
quaternion<T> operator/( const quaternion<T>& x, T s )
{
	return x * ( T( 1.0f ) / s );
}

template<class T>
quaternion<T> operator/( T s, const quaternion<T>& x )
{
	return x / s;
}

template<class T>
quaternion<T>& operator/=( quaternion<T>& x, T s )
{
	return x = x / s;
}

template<class T>
T dot_product( const quaternion<T>& x, const quaternion<T>& y )
{
	return x[ 0 ] * y[ 0 ] + x[ 1 ] * y[ 1 ] + x[ 2 ] * y[ 2 ] + x[ 3 ] * y[ 3 ];
}

template<class T>
quaternion<T> quaternion_from_rotation( const matrix_4x4<T>& m )
{
	quaternion<T> q;
	
	T trace = m( 0, 0 ) + m( 1, 1 ) + m( 2, 2 );
	
	if( trace > T( 0.0f ) )
	{
		T root = sqrt( trace + T( 1.0f ) );
		q[ 3 ] = T( 0.5f ) * root;
		root = 0.5f / root;
		q[ 0 ] = ( m( 2, 1 ) - m( 1, 2 ) ) * root;
		q[ 1 ] = ( m( 0, 2 ) - m( 2, 0 ) ) * root;
		q[ 2 ] = ( m( 1, 0 ) - m( 0, 1 ) ) * root;
	}
	else
	{
		int next[ ] = { 1, 2, 0 };

		int i = 0;
		if( m( 0, 0 ) > m( 1, 1 ) ) i = 1;
		if( m( 2, 2 ) > m( i, i ) ) i = 2;
		
		int j = next[ i ];
		int k = next[ j ];
		
		T root = sqrtf( m( i, i ) - m( j, j ) - m( k, k ) + T( 1.0f ) );

		q[ i ] = T( 0.5f ) * root;
		root = 0.5f / root;
		q[ j ] = ( m( j, i ) + m( i, j ) ) * root;
		q[ k ] = ( m( k, i ) + m( i, k ) ) * root;
		q[ 3 ] = ( m( k, j ) - m( j, k ) ) * root;
	}

	return q;
}

template<class T>
matrix_4x4<T> quaternion_to_rotation( const quaternion<T>& q )
{
	matrix_4x4<T> m;

	T tx  = T( 2.0f ) * q[ 0 ];
	T ty  = T( 2.0f ) * q[ 1 ];
	T tz  = T( 2.0f ) * q[ 2 ];
	T txx = tx * q[ 0 ];
	T txy = ty * q[ 0 ];
	T txz = tz * q[ 0 ];
	T tyy = ty * q[ 1 ];
	T tyz = tz * q[ 1 ];
	T tzz = tz * q[ 2 ];
	T twx = tx * q[ 3 ];
	T twy = ty * q[ 3 ];
	T twz = tz * q[ 3 ];

	m( 0, 0 ) = T( 1.0f ) - ( tyy + tzz );
	m( 0, 1 ) = txy - twz;
	m( 0, 2 ) = txz + twy;
	m( 0, 3 ) = 0.0f;
	
	m( 1, 0 ) = txy + twz;
	m( 1, 1 ) = T( 1.0f ) - ( txx + tzz );
	m( 1, 2 ) = tyz - twx;
	m( 1, 3 ) = 0.0f;
	
	m( 2, 0 ) = txz - twy;
	m( 2, 1 ) = tyz + twx;
	m( 2, 2 ) = T( 1.0f ) - ( txx + tyy );
	m( 3, 3 ) = 1.0f;
	
	return m;
}

typedef quaternion<double>	quaterniond;
typedef quaternion<float>	quaternionf;

} }

#endif
