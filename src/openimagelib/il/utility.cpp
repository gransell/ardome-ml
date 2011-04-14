/* -*- tab-width: 4; indent-tabs-mode: t -*- */ 
// il - A image library representation.

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#include <algorithm>
#include <cmath>
#include <limits>
#include <iostream>
#include <stdio.h>

#include <openimagelib/il/openimagelib_plugin.hpp>

#include <openpluginlib/pl/fast_math.hpp>
#include <openpluginlib/pl/utf8_utils.hpp>
#include <openpluginlib/pl/discovery_traits.hpp>

namespace opl = olib::openpluginlib;
namespace fs  = boost::filesystem;

namespace olib { namespace openimagelib { namespace il {

namespace
{
	// NOTE: anonymous namespace reserved to utility functions.
	
	float P( float x )
	{
		if( x < 0.0f )
			return 0.0f;
		else
			return x;
	}

	float R( float x )
	{
		const float px0 = P( x - 1.0f );
		const float px1 = P( x );
		const float px2 = P( x + 1.0f );
		const float px3 = P( x + 2.0f );
	
		return ( px3 * px3 * px3 - 4.0f * px2 * px2 * px2 + 6.0f * px1 * px1 * px1 - 4.0f * px0 * px0 * px0 ) / 6.0f; 
	}

	int P( int v )
	{
		return v < 0 ? 0 : v;
	}

	int C( int v )
	{
		return ( ( ( v * v ) >> 11 ) * v ) >> 11;
	}

	int R( int x )
	{
		const int px0 = P( x - 0x800 );
		const int px1 = P( x );
		const int px2 = P( x + 0x800 );
		const int px3 = P( x + 0x1000 );
	
		return ( C( px3 ) - 4 * C( px2 ) + 6 * C( px1 ) - 4 * C( px0 ) ) / 6; 
	}

	float L( float val, float quant_step, float disp_gamma, float neg_film_gamma )
	{
		return powf( powf( 10.0f, val * quant_step / neg_film_gamma ), disp_gamma / 1.7f );
	}
	
	void log_to_linear_lut( std::vector<unsigned short>& lut, int refblack, int refwhite, int softclip, float gamma, float neg_film_gamma, float density_range )
	{
		int lut_siz = 1 << 10;
		
		float breakpoint	= static_cast<float>( refwhite - softclip );
		float quant_step	= density_range / lut_siz;
		float gain			= ( lut_siz - 1.0f ) / ( 1.0f - L( static_cast<float>( refblack - refwhite ), quant_step, gamma, neg_film_gamma ) );
		float offset		= gain - lut_siz - 1.0f;
		float knee_offset	= L( breakpoint - refwhite, quant_step, gamma, neg_film_gamma ) * gain - offset;
		float knee_gain		= ( lut_siz - 1.0f - knee_offset ) / powf( 5.0f * softclip, softclip / 100.0f );

		lut.resize( lut_siz );
		
		for( int i = 0; i < lut_siz; ++i )
		{	
			float val = static_cast<float>( i );
			
			if( val < refblack )
				val = 0.0f;
			else if( val > breakpoint )
				val = powf( i - breakpoint, softclip / 100.0f ) * knee_gain + knee_offset;
			else
				val = L( val - refwhite, quant_step, gamma, neg_film_gamma ) * gain - offset;
				
			lut[ i ] = static_cast<unsigned short>( floorf( val + 0.5f ) );
		}
	}
}

// Foward declaration to plane scaler
static void rescale_plane( image_type_ptr &new_im, const image_type_ptr& im, int new_d, int bs, rescale_filter filter, const int &p );

// The following private functions are a bit rough and shouldn't be exposed publicly
// All access to the functions are via the public convert and allocate functions

typedef image< unsigned char, f32 >				f32_image_type;
typedef image< unsigned char, l8 >				l8_image_type;
typedef image< unsigned char, l8a8 >			l8a8_image_type;
typedef image< unsigned char, l8a8p >			l8a8p_image_type;
typedef image< unsigned char, l12a12p >			l12a12p_image_type;
typedef image< unsigned char, l16a16p >			l16a16p_image_type;
typedef image< unsigned char, r8g8b8 >			r8g8b8_image_type;
typedef image< unsigned char, r8g8b8p >			r8g8b8p_image_type;
typedef image< unsigned char, b8g8r8 >			b8g8r8_image_type;
typedef image< unsigned char, b8g8r8a8 >		b8g8r8a8_image_type;
typedef image< unsigned char, r8g8b8a8 >		r8g8b8a8_image_type;
typedef image< unsigned char, r8g8b8a8p >		r8g8b8a8p_image_type;
typedef image< unsigned char, a8r8g8b8 >		a8r8g8b8_image_type;
typedef image< unsigned char, a8b8g8r8 >		a8b8g8r8_image_type;
typedef image< unsigned char, r8g8b8log >		r8g8b8log_image_type;
typedef image< unsigned char, r8g8b8a8log >		r8g8b8a8log_image_type;
typedef image< unsigned char, r10g10b10 >		r10g10b10_image_type;
typedef image< unsigned char, r10g10b10a10 >	r10g10b10a10_image_type;
typedef image< unsigned char, r10g10b10p >		r10g10b10p_image_type;
typedef image< unsigned char, r10g10b10a10p >	r10g10b10a10p_image_type;
typedef image< unsigned char, r10g10b10log >	r10g10b10log_image_type;
typedef image< unsigned char, r10g10b10a10log >	r10g10b10a10log_image_type;
typedef image< unsigned char, r12g12b12 >		r12g12b12_image_type;
typedef image< unsigned char, r12g12b12a12 >	r12g12b12a12_image_type;
typedef image< unsigned char, r12g12b12p >		r12g12b12p_image_type;
typedef image< unsigned char, r12g12b12a12p >	r12g12b12a12p_image_type;
typedef image< unsigned char, r12g12b12log >	r12g12b12log_image_type;
typedef image< unsigned char, r12g12b12a12log >	r12g12b12a12log_image_type;
typedef image< unsigned char, r16g16b16 >		r16g16b16_image_type;
typedef image< unsigned char, r16g16b16a16 >	r16g16b16a16_image_type;
typedef image< unsigned char, r16g16b16p >		r16g16b16p_image_type;
typedef image< unsigned char, r16g16b16a16p >	r16g16b16a16p_image_type;
typedef image< unsigned char, r16g16b16f >		r16g16b16f_image_type;
typedef image< unsigned char, r16g16b16a16f >	r16g16b16a16f_image_type;
typedef image< unsigned char, r16g16b16log >	r16g16b16log_image_type;
typedef image< unsigned char, r16g16b16a16log >	r16g16b16a16log_image_type;
typedef image< unsigned char, r32g32b32f >		r32g32b32f_image_type;
typedef image< unsigned char, r32g32b32a32f >	r32g32b32a32f_image_type;
typedef image< unsigned char, yuv444p >			yuv444p_image_type;
typedef image< unsigned char, yuv444 >			yuv444_image_type;
typedef image< unsigned char, yuv422 >			yuv422_image_type;
typedef image< unsigned char, yuv422p >			yuv422p_image_type;
typedef image< unsigned char, yuv420p >			yuv420p_image_type;
typedef image< unsigned char, yuv411 >			yuv411_image_type;
typedef image< unsigned char, yuv411p >			yuv411p_image_type;
typedef const unsigned char *					const_pointer;
typedef unsigned char *							pointer;
typedef int 									size_type;

// Public allocation method
IL_DECLSPEC image_type_ptr allocate( const opl::wstring &pf, int width, int height )
{
	image_type_ptr dst_img = image_type_ptr( );

	if ( pf == L"r8g8b8" )
		dst_img = image_type_ptr( new image_type( r8g8b8_image_type( width, height, 1 ) ) );
	else if ( pf == L"r8g8b8p" )
		dst_img = image_type_ptr( new image_type( r8g8b8p_image_type( width, height, 1 ) ) );
	else if ( pf == L"b8g8r8" )
		dst_img = image_type_ptr( new image_type( b8g8r8_image_type( width, height, 1 ) ) );
	else if ( pf == L"r8g8b8a8" )
		dst_img = image_type_ptr( new image_type( r8g8b8a8_image_type( width, height, 1 ) ) );
	else if ( pf == L"b8g8r8a8" )
		dst_img = image_type_ptr( new image_type( b8g8r8a8_image_type( width, height, 1 ) ) );
	else if( pf == L"a8r8g8b8" )
		dst_img = image_type_ptr( new image_type( a8r8g8b8_image_type( width, height, 1 ) ) );
	else if( pf == L"a8b8g8r8" )
		dst_img = image_type_ptr( new image_type( a8b8g8r8_image_type( width, height, 1 ) ) );
	else if ( pf == L"r8g8b8log" )
		dst_img = image_type_ptr( new image_type( r8g8b8log_image_type( width, height, 1 ) ) );
	else if ( pf == L"r8g8b8a8log" )
		dst_img = image_type_ptr( new image_type( r8g8b8a8log_image_type( width, height, 1 ) ) );
	else if( pf == L"r10g10b10a10" )
		dst_img = image_type_ptr( new image_type( r10g10b10a10_image_type( width, height, 1 ) ) );
	else if( pf == L"r10g10b10" )
		dst_img = image_type_ptr( new image_type( r10g10b10_image_type( width, height, 1 ) ) );
	else if( pf == L"r10g10b10a10p" )
		dst_img = image_type_ptr( new image_type( r10g10b10a10p_image_type( width, height, 1 ) ) );
	else if( pf == L"r10g10b10p" )
		dst_img = image_type_ptr( new image_type( r10g10b10p_image_type( width, height, 1 ) ) );
	else if( pf == L"r10g10b10log" )
		dst_img = image_type_ptr( new image_type( r10g10b10log_image_type( width, height, 1 ) ) );
	else if( pf == L"r10g10b10a10log" )
		dst_img = image_type_ptr( new image_type( r10g10b10a10log_image_type( width, height, 1 ) ) );
	else if( pf == L"r12g12b12a12" )
		dst_img = image_type_ptr( new image_type( r12g12b12a12_image_type( width, height, 1 ) ) );
	else if( pf == L"r12g12b12" )
		dst_img = image_type_ptr( new image_type( r12g12b12_image_type( width, height, 1 ) ) );
	else if( pf == L"r12g12b12a12p" )
		dst_img = image_type_ptr( new image_type( r12g12b12a12p_image_type( width, height, 1 ) ) );
	else if( pf == L"r12g12b12p" )
		dst_img = image_type_ptr( new image_type( r12g12b12p_image_type( width, height, 1 ) ) );
	else if( pf == L"r12g12b12log" )
		dst_img = image_type_ptr( new image_type( r12g12b12log_image_type( width, height, 1 ) ) );
	else if( pf == L"r12g12b12a12log" )
		dst_img = image_type_ptr( new image_type( r12g12b12a12log_image_type( width, height, 1 ) ) );
	else if( pf == L"r16g16b16a16f" )
		dst_img = image_type_ptr( new image_type( r16g16b16a16f_image_type( width, height, 1 ) ) );
	else if( pf == L"r16g16b16a16" )
		dst_img = image_type_ptr( new image_type( r16g16b16a16_image_type( width, height, 1 ) ) );
	else if( pf == L"r16g16b16" )
		dst_img = image_type_ptr( new image_type( r16g16b16_image_type( width, height, 1 ) ) );
	else if( pf == L"r16g16b16f" )
		dst_img = image_type_ptr( new image_type( r16g16b16f_image_type( width, height, 1 ) ) );
	else if( pf == L"r16g16b16a16f" )
		dst_img = image_type_ptr( new image_type( r16g16b16a16f_image_type( width, height, 1 ) ) );
	else if( pf == L"r16g16b16a16p" )
		dst_img = image_type_ptr( new image_type( r16g16b16a16p_image_type( width, height, 1 ) ) );
	else if( pf == L"r16g16b16p" )
		dst_img = image_type_ptr( new image_type( r16g16b16p_image_type( width, height, 1 ) ) );
	else if( pf == L"r16g16b16log" )
		dst_img = image_type_ptr( new image_type( r16g16b16log_image_type( width, height, 1 ) ) );
	else if( pf == L"r16g16b16a16log" )
		dst_img = image_type_ptr( new image_type( r16g16b16a16log_image_type( width, height, 1 ) ) );
	else if( pf == L"r32g32b32f" )
		dst_img = image_type_ptr( new image_type( r32g32b32f_image_type( width, height, 1 ) ) );
	else if( pf == L"r32g32b32a32f" )
		dst_img = image_type_ptr( new image_type( r32g32b32a32f_image_type( width, height, 1 ) ) );
	else if ( pf == L"yuv444p" )
		dst_img = image_type_ptr( new image_type( yuv444p_image_type( width, height, 1 ) ) );
	else if ( pf == L"yuv444" )
		dst_img = image_type_ptr( new image_type( yuv444_image_type( width, height, 1 ) ) );
	else if ( pf == L"yuv422" )
		dst_img = image_type_ptr( new image_type( yuv422_image_type( width, height, 1 ) ) );
	else if ( pf == L"yuv422p" )
		dst_img = image_type_ptr( new image_type( yuv422p_image_type( width, height, 1 ) ) );
	else if ( pf == L"yuv420p" )
		dst_img = image_type_ptr( new image_type( yuv420p_image_type( width, height, 1 ) ) );
	else if ( pf == L"yuv411" )
		dst_img = image_type_ptr( new image_type( yuv411_image_type( width, height, 1 ) ) );
	else if ( pf == L"yuv411p" )
		dst_img = image_type_ptr( new image_type( yuv411p_image_type( width, height, 1 ) ) );
	else if ( pf == L"l8" )
		dst_img = image_type_ptr( new image_type( l8_image_type( width, height, 1 ) ) );
	else if ( pf == L"f32" )
		dst_img = image_type_ptr( new image_type( f32_image_type( width, height, 1 ) ) );
	else if ( pf == L"l8a8" )
		dst_img = image_type_ptr( new image_type( l8a8_image_type( width, height, 1 ) ) );
	else if ( pf == L"l8a8p" )
		dst_img = image_type_ptr( new image_type( l8a8p_image_type( width, height, 1 ) ) );
	else if ( pf == L"l12a12p" )
		dst_img = image_type_ptr( new image_type( l12a12p_image_type( width, height, 1 ) ) );
	else if ( pf == L"l16a16p" )
		dst_img = image_type_ptr( new image_type( l16a16p_image_type( width, height, 1 ) ) );
	
	return dst_img;
}

// Convenience method to allocate an image of the output format, ensuring all the required
// information from the source is inherited
IL_DECLSPEC image_type_ptr allocate( const image_type_ptr &src_img, const opl::wstring &format )
{
	image_type_ptr dst_img = il::allocate( format, src_img->width( ), src_img->height( ) );

	if ( dst_img )
	{
		dst_img->set_flipped( src_img->is_flipped( ) );
		dst_img->set_flopped( src_img->is_flopped( ) );
		dst_img->set_field_order( src_img->field_order( ) );
	}

	return dst_img;
}

IL_DECLSPEC image_type_ptr allocate( const image_type_ptr &src_img ) 
{
	return il::allocate( src_img, src_img->pf( ) ); 
}

// Obtain the fields in presentation order
IL_DECLSPEC image_type_ptr field( const image_type_ptr &im, int field )
{
	// Default our return to the source
	image_type_ptr new_im = im;

	// Only do something if we have an image and it isn't progressive
	if ( im && im->field_order( ) != progressive )
	{
		// Allocate an image which is half the size of the source
		new_im = allocate( im->pf( ), im->width( ), im->height( ) / 2 );

		// The apps field 0 depends on the field order of the image
		if ( im->field_order( ) == top_field_first )
			field = field == 0 ? 0 : 1;
		else
			field = field == 1 ? 0 : 1;

		// Copy every second scan line from each plane to extract the field
		for ( int i = 0; i < im->plane_count( ); i ++ )
		{
			unsigned char *src = im->data( i ) + field * im->pitch( i );
			unsigned char *dst = new_im->data( i );
			int src_pitch = 2 * im->pitch( i );
			int dst_pitch = new_im->pitch( i );
			int linesize = new_im->linesize( i );
			int height = new_im->height( i );
			while( height -- )
			{
				memcpy( dst, src, linesize );
				dst += dst_pitch;
				src += src_pitch;
			}
		}
	}

	return new_im;
}

IL_DECLSPEC image_type_ptr deinterlace( const image_type_ptr &im )
{
	if ( im && im->field_order( ) != progressive )
	{
		im->set_field_order( progressive );
		for ( int i = 0; i < im->plane_count( ); i ++ )
		{
			unsigned char *dst = im->data( i );
			unsigned char *src = im->data( i ) + im->pitch( i );
			int linesize = im->linesize( i );
			int src_pitch = im->pitch( i ) - linesize;
			int height = im->height( i ) - 1;
			while( height -- )
			{
				for ( int w = 0; w < linesize; w ++ )
				{
					*dst = ( *src ++ + *dst ) >> 1;
					dst ++;
				}
				dst += src_pitch;
				src += src_pitch;
			}
		}
	}

	return im;
}

static int locate_alpha_offset( const opl::wstring &pf )
{
	int result = -1;

	if ( pf == L"r8g8b8a8" )
		result = 3;
	else if ( pf == L"b8g8r8a8" )
		result = 3;
	else if( pf == L"a8r8g8b8" )
		result = 0;
	else if( pf == L"a8b8g8r8" )
		result = 0;

	return result;
}

IL_DECLSPEC image_type_ptr extract_alpha( const image_type_ptr &im )
{
	image_type_ptr result;

	if ( im )
	{
		int offset = locate_alpha_offset( im->pf( ) );
		if ( offset >= 0 )
		{
			result = allocate( L"l8", im->width( ), im->height( ) );

			const_pointer src = im->data( );
			pointer dst = result->data( );

			size_type h = im->height( );

			size_type src_rem = im->pitch( ) - im->linesize( );
			size_type dst_rem = result->pitch( ) - result->linesize( );

			while( h -- )
			{
				size_type w = im->width( );

				while ( w -- )
				{
					*dst ++ = src[ offset ];
					src += 4;
				}

				src += src_rem;
				dst += dst_rem;
			}
		}
	}

	return result;
}

IL_DECLSPEC image_type_ptr merge_alpha( const image_type_ptr &im, const image_type_ptr &alpha )
{
	if ( im && alpha )
	{
		int offset = locate_alpha_offset( im->pf( ) );
		if ( offset != -1 && alpha->pf( ) == L"l8" )
		{
			pointer s = im->data( );
			const_pointer a = alpha->data( );

			size_type h = im->height( );

			size_type s_rem = im->pitch( ) - im->linesize( );
			size_type a_rem = alpha->pitch( ) - alpha->linesize( );

			while( h -- )
			{
				size_type w = im->width( );

				while ( w -- )
				{
					s[ offset ] = *a ++;
					s += 4;
				}

				s += s_rem;
				a += a_rem;
			}
		}
	}
	return im;
}

static image_type_ptr yuvp_to_yuvp( const image_type_ptr &src_img, const opl::wstring &format )
{
	image_type_ptr dst_img = allocate( src_img, format );

	if ( dst_img != 0 )
	{
		for ( int plane = 0; plane < 3; plane ++ )
		{
			rescale_plane( dst_img, src_img, 1, 1, BILINEAR_SAMPLING, plane );
		}
	}

	return dst_img;
}

static int lookup_u1[ 255 ];
static int lookup_u2[ 255 ];
static int lookup_v1[ 255 ];
static int lookup_v2[ 255 ];

static bool lookup_init = false;

inline void init_table( )
{
	if ( !lookup_init )
	{
		for ( int i = 0; i < 255; i ++ )
		{
			lookup_u1[ i ] = ( i - 128 ) * 400;
			lookup_u2[ i ] = ( i - 128 ) * 2066;
			lookup_v1[ i ] = ( i - 128 ) * 1634;
			lookup_v2[ i ] = ( i - 128 ) * 832;
		}
		lookup_init = true;
	}
}

inline unsigned char clamp_sample( const int v ) { return ( unsigned char )( v < 0 ? 0 : v > 255 ? 255 : v ); }

inline void yuv444_to_rgb( unsigned char *&dst, const int y, const int rc, const int gc, const int bc )
{
	*dst ++ = clamp_sample( ( y + rc ) >> 10 );
	*dst ++ = clamp_sample( ( y - gc ) >> 10 );
	*dst ++ = clamp_sample( ( y + bc ) >> 10 );
}

inline void yuv444_to_bgr( unsigned char *&dst, const int y, const int rc, const int gc, const int bc )
{
	*dst ++ = clamp_sample( ( y + bc ) >> 10 );
	*dst ++ = clamp_sample( ( y - gc ) >> 10 );
	*dst ++ = clamp_sample( ( y + rc ) >> 10 );
}

static image_type_ptr yuv420p_to_bgr( const image_type_ptr &src_img, const opl::wstring &format )
{
	image_type_ptr dst_img = allocate( src_img, format );

	init_table( );

	if ( dst_img != 0 )
	{
		size_type dst_width = dst_img->width( ) / 2;
		size_type dst_height = dst_img->height( ) / 2;
		
		pointer dst0 = dst_img->data( );
		pointer dst1 = dst_img->data( ) + dst_img->pitch( );
		size_type dst_rem = 2 * dst_img->pitch( ) - dst_img->linesize( );
		size_type Y_rem = 2 * src_img->pitch( ) - src_img->linesize( );
		size_type UV_rem = src_img->pitch( 1 ) - src_img->linesize( 1 );

		const_pointer Y0 = src_img->data( 0 );
		const_pointer Y1 = src_img->data( 0 ) + src_img->pitch( 0 );
		const_pointer U = src_img->data( 1 );
		const_pointer V = src_img->data( 2 );

		int rc, gc, bc;
		int x;

		while( dst_height -- )
		{
			x = dst_width;
			while( x -- )
			{
				rc = lookup_v1[ *V ];
				gc = lookup_v2[ *V ++ ] + lookup_u1[ *U ];
				bc = lookup_u2[ *U ++ ];
				yuv444_to_bgr( dst0, 1192 * ( int( *Y0 ++ ) - 16 ), rc, gc, bc );
				yuv444_to_bgr( dst0, 1192 * ( int( *Y0 ++ ) - 16 ), rc, gc, bc );
				yuv444_to_bgr( dst1, 1192 * ( int( *Y1 ++ ) - 16 ), rc, gc, bc );
				yuv444_to_bgr( dst1, 1192 * ( int( *Y1 ++ ) - 16 ), rc, gc, bc );
			}

			dst0 += dst_rem;
			dst1 += dst_rem;
			Y0 += Y_rem;
			Y1 += Y_rem;
			U += UV_rem;
			V += UV_rem;
		}
	}

	return dst_img;
}

static image_type_ptr yuv420p_to_rgb( const image_type_ptr &src_img, const opl::wstring &format )
{
	image_type_ptr dst_img = allocate( src_img, format );

	init_table( );

	if ( dst_img != 0 )
	{
		size_type dst_width = dst_img->width( ) / 2;
		size_type dst_height = dst_img->height( ) / 2;
		
		pointer dst0 = dst_img->data( );
		pointer dst1 = dst_img->data( ) + dst_img->pitch( );
		size_type dst_rem = 2 * dst_img->pitch( ) - dst_img->linesize( );
		size_type Y_rem = 2 * src_img->pitch( ) - src_img->linesize( );
		size_type UV_rem = src_img->pitch( 1 ) - src_img->linesize( 1 );

		const_pointer Y0 = src_img->data( 0 );
		const_pointer Y1 = src_img->data( 0 ) + src_img->pitch( 0 );
		const_pointer U = src_img->data( 1 );
		const_pointer V = src_img->data( 2 );

		register int rc, gc, bc;
		register int x;

		while( dst_height -- )
		{
			x = dst_width;
			while( x -- )
			{
				rc = lookup_v1[ *V ];
				gc = lookup_v2[ *V ++ ] + lookup_u1[ *U ];
				bc = lookup_u2[ *U ++ ];
				yuv444_to_rgb( dst0, 1192 * ( int( *Y0 ++ ) - 16 ), rc, gc, bc );
				yuv444_to_rgb( dst0, 1192 * ( int( *Y0 ++ ) - 16 ), rc, gc, bc );
				yuv444_to_rgb( dst1, 1192 * ( int( *Y1 ++ ) - 16 ), rc, gc, bc );
				yuv444_to_rgb( dst1, 1192 * ( int( *Y1 ++ ) - 16 ), rc, gc, bc );
			}

			dst0 += dst_rem;
			dst1 += dst_rem;
			Y0 += Y_rem;
			Y1 += Y_rem;
			U += UV_rem;
			V += UV_rem;
		}
	}

	return dst_img;
}

static image_type_ptr yuvp_to_rgb( const image_type_ptr &src_img, const opl::wstring &format, int r, int g, int b, int a )
{
	image_type_ptr dst_img = allocate( src_img, format );

	if ( dst_img != 0 )
	{
		size_type dst_width = dst_img->width( );
		size_type dst_height = dst_img->height( );
		
		size_type chroma_width = src_img->width( 1 );
		size_type chroma_height = src_img->height( 1 );

		size_type x_factor = ( chroma_width << 8 ) / dst_width;
		size_type y_factor = ( chroma_height << 8 ) / dst_height;

		pointer dst = dst_img->data( );
		size_type dst_rem = dst_img->pitch( ) - dst_img->linesize( );

		int rgb[ 4 ] = { 255, 255, 255, 255 };
		int uv_offset;

		for ( int y = 0; y < dst_height; y ++ )
		{
			const_pointer Y = src_img->data( 0 ) + ( y * src_img->pitch( 0 ) );
			const_pointer U = src_img->data( 1 ) + ( ( y * y_factor ) >> 8 ) * src_img->pitch( 1 );
			const_pointer V = src_img->data( 2 ) + ( ( y * y_factor ) >> 8 ) * src_img->pitch( 2 );

			if ( a == -1 )
			{
				for ( int x = 0; x < dst_width; x ++ )
				{
					uv_offset = ( x * x_factor ) >> 8;
					yuv444_to_rgb24( rgb[ r ], rgb[ g ], rgb[ b ], *Y ++, U[ uv_offset ], V[ uv_offset ] );
					*dst ++ = ( unsigned char )( rgb[ 0 ] );
					*dst ++ = ( unsigned char )( rgb[ 1 ] );
					*dst ++ = ( unsigned char )( rgb[ 2 ] );
				}
			}
			else
			{
				for ( int x = 0; x < dst_width; x ++ )
				{
					uv_offset = ( x * x_factor ) >> 8;
					yuv444_to_rgb24( rgb[ r ], rgb[ g ], rgb[ b ], *Y ++, U[ uv_offset ], V[ uv_offset ] );
					*dst ++ = ( unsigned char )( rgb[ 0 ] );
					*dst ++ = ( unsigned char )( rgb[ 1 ] );
					*dst ++ = ( unsigned char )( rgb[ 2 ] );
					*dst ++ = ( unsigned char )( rgb[ 3 ] );
				}
			}

			dst += dst_rem;
		}
	}

	return dst_img;
}

static image_type_ptr rgb_to_yuvp( const image_type_ptr &src_img, const opl::wstring &format, int r, int g, int b, int a )
{
	image_type_ptr dst_img = allocate( src_img, format );

	if ( dst_img != 0 )
	{
		size_type dst_width = dst_img->width( );
		size_type dst_height = dst_img->height( );
		
		size_type chroma_width = dst_img->width( 1 );
		size_type chroma_height = dst_img->height( 1 );

		size_type src_width = src_img->width( );
		size_type src_height = src_img->height( );

		size_type x_factor = ( chroma_width << 8 ) / src_width;
		size_type y_factor = ( chroma_height << 8 ) / src_height;

		const_pointer src = src_img->data( );
		size_type src_rem = src_img->pitch( ) - src_img->linesize( );

		int uv_offset;

		int yuv[ 3 ];
		int components = a == -1 ? 3 : 4;

		for ( int y = 0; y < dst_height; y ++ )
		{
			pointer Y = dst_img->data( 0 ) + ( y * dst_img->pitch( 0 ) );
			pointer U = dst_img->data( 1 ) + ( ( y * y_factor ) >> 8 ) * dst_img->pitch( 1 );
			pointer V = dst_img->data( 2 ) + ( ( y * y_factor ) >> 8 ) * dst_img->pitch( 2 );

			for ( int x = 0; x < dst_width; x ++ )
			{
				uv_offset = ( x * x_factor ) >> 8;
				rgb24_to_yuv444( yuv[ 0 ], yuv[ 1 ], yuv[ 2 ], src[ r ], src[ g ], src[ b ] );
				*Y ++ = ( unsigned char )yuv[ 0 ];
				U[ uv_offset ] = ( unsigned char )yuv[ 1 ];
				V[ uv_offset ] = ( unsigned char )yuv[ 2 ];
				src += components;
			}

			src += src_rem;
		}
	}

	return dst_img;
}

#ifdef HAVE_MMX
static image_type_ptr yuv420p_to_yuv422( const image_type_ptr &src_img, const opl::wstring &format )
{
	image_type_ptr dst_img = allocate( src_img, format );

	if ( dst_img != 0 )
	{
		size_type dst_width = dst_img->width( ) / 8;
		size_type dst_height = dst_img->height( ) / 2;
		
		pointer dst0 = dst_img->data( );
		pointer dst1 = dst_img->data( ) + dst_img->pitch( );

		const size_type dst_rem = 2 * dst_img->pitch( ) - dst_img->linesize( );
		const size_type y_src_rem = 2 * src_img->pitch( ) - src_img->linesize( );
		const size_type uv_src_rem = src_img->pitch( 1 ) - src_img->linesize( 1 );

		const_pointer Y0 = src_img->data( 0 );
		const_pointer Y1 = src_img->data( 0 ) + src_img->pitch( 0 );
		const_pointer U = src_img->data( 1 );
		const_pointer V = src_img->data( 2 );

		int x;

		while( dst_height -- )
		{
			x = dst_width;
			while( x -- )
			{
    			__asm__ __volatile__ 
				(
       				".align 8\n"
					"movq       (%2), %%mm0\n"
					"movd       (%4), %%mm1\n"
					"movd       (%5), %%mm2\n"
					"punpcklbw %%mm2, %%mm1\n"
					"movq      %%mm0, %%mm2\n"
					"punpcklbw %%mm1, %%mm2\n"
					"movq      %%mm2, (%0)\n"
					"punpckhbw %%mm1, %%mm0\n"
					"movq      %%mm0, 8(%0)\n"
					"movq       (%3), %%mm0\n"
					"movq      %%mm0, %%mm2\n"
					"punpcklbw %%mm1, %%mm2\n"
					"movq      %%mm2, (%1)\n"
					"punpckhbw %%mm1, %%mm0\n"
					"movq      %%mm0, 8(%1)\n"
					"emms\n"
       				:
       				: "r" (dst0),  "r" (dst1),  "r" (Y0),  "r" (Y1), "r" (U), "r" (V) 
				);
    			dst0 += 16; dst1 += 16; Y0 += 8; Y1 += 8; U += 4; V += 4;
 			}

			dst0 += dst_rem;
			dst1 += dst_rem;
			Y0 += y_src_rem;
			Y1 += y_src_rem;
			U += uv_src_rem;
			V += uv_src_rem;
		}
	}

	return dst_img;
}

#endif

inline void yuv420p_to_yuv444( unsigned char *&dst0, unsigned char *&dst1, const unsigned char *&Y0, const unsigned char *&Y1, const unsigned char u, const unsigned char v )
{
	*dst0 ++ = *Y0 ++;
	*dst0 ++ = u;
	*dst0 ++ = v;

	*dst0 ++ = *Y0 ++;
	*dst0 ++ = u;
	*dst0 ++ = v;

	*dst1 ++ = *Y1 ++;
	*dst1 ++ = u;
	*dst1 ++ = v;

	*dst1 ++ = *Y1 ++;
	*dst1 ++ = u;
	*dst1 ++ = v;
}

static image_type_ptr yuv420p_to_yuv444( const image_type_ptr &src_img, const opl::wstring &format )
{
	image_type_ptr dst_img = allocate( src_img, format );

	if ( dst_img != 0 )
	{
		size_type dst_width = dst_img->width( ) / 4;
		size_type dst_height = dst_img->height( ) / 2;
		
		pointer dst0 = dst_img->data( );
		pointer dst1 = dst_img->data( ) + dst_img->pitch( );

		const size_type dst_rem = 2 * dst_img->pitch( ) - dst_img->linesize( );
		const size_type y_src_rem = 2 * src_img->pitch( ) - src_img->linesize( );
		const size_type uv_src_rem = src_img->pitch( 1 ) - src_img->linesize( 1 );

		const_pointer Y0 = src_img->data( 0 );
		const_pointer Y1 = src_img->data( 0 ) + src_img->pitch( 0 );
		const_pointer U = src_img->data( 1 );
		const_pointer V = src_img->data( 2 );

		int x;

		while( dst_height -- )
		{
			x = dst_width;
			while( x -- )
			{
				yuv420p_to_yuv444( dst0, dst1, Y0, Y1, *U ++, *V ++ );
				yuv420p_to_yuv444( dst0, dst1, Y0, Y1, *U ++, *V ++ );
 			}

			dst0 += dst_rem;
			dst1 += dst_rem;
			Y0 += y_src_rem;
			Y1 += y_src_rem;
			U += uv_src_rem;
			V += uv_src_rem;
		}
	}

	return dst_img;
}

static image_type_ptr yuvp_to_yuv422( const image_type_ptr &src_img, const opl::wstring &format )
{
	image_type_ptr dst_img = allocate( src_img, format );

	if ( dst_img != 0 )
	{
		size_type dst_width = dst_img->width( ) / 2;
		size_type dst_height = dst_img->height( );
		
		size_type chroma_width = src_img->width( 1 );
		size_type chroma_height = src_img->height( 1 );

		size_type x_factor = ( chroma_width << 8 ) / dst_width;
		size_type y_factor = ( chroma_height << 8 ) / dst_height;

		pointer dst = dst_img->data( );
		size_type dst_rem = dst_img->pitch( ) - dst_img->linesize( );
		size_type y_src_rem = src_img->pitch( ) - src_img->linesize( );

		int uv_offset;

		const_pointer pu = src_img->data( 1 );
		int ppu = src_img->pitch( 1 );
		const_pointer pv = src_img->data( 2 );
		int ppv = src_img->pitch( 2 );

		const_pointer Y = src_img->data( 0 );
		const_pointer U, V;

		int mx = dst_width * x_factor;
		int x = 0;

		for ( int y = 0; y < dst_height; y ++ )
		{
			U = pu + ( ( y * y_factor ) >> 8 ) * ppu;
			V = pv + ( ( y * y_factor ) >> 8 ) * ppv;

			for( x = 0; x < mx; x += x_factor )
			{
   				uv_offset = x >> 8;
				*dst ++ = *Y ++;
				*dst ++ = U[ uv_offset ];
				*dst ++ = *Y ++;
				*dst ++ = V[ uv_offset ];
 			}

			dst += dst_rem;
			Y += y_src_rem;
		}
	}

	return dst_img;
}

static image_type_ptr yuv422_to_yuvp( const image_type_ptr &src_img, const opl::wstring &format )
{
	image_type_ptr dst_img = allocate( src_img, format );

	if ( dst_img != 0 )
	{
		for ( int p = 0; p < 3; p ++ )
		{
			size_type dst_width = dst_img->width( p );
			size_type dst_height = dst_img->height( p );

			pointer dst = dst_img->data( p );
			size_type dst_rem = dst_img->pitch( p ) - dst_img->linesize( p );

			size_type src_height = src_img->height( );

			size_type y_factor = ( src_height << 8 ) / dst_height;

			if ( p == 0 )
			{
				for ( int y = 0; y < dst_height; y ++ )
				{
					const_pointer line = src_img->data( ) + ( y * src_img->pitch( ) );
	
					for ( int x = 0; x < dst_width; x ++ )
					{
						*dst ++ = ( unsigned char )( *line );
						line += 2;
					}

					dst += dst_rem;
				}
			}
			else
			{
				int uv_offset = 1 + ( p - 1 ) * 2;

				size_type src_width = src_img->width( );
				size_type x_factor = ( src_width << 8 ) / dst_width;

				for ( int y = 0; y < dst_height; y ++ )
				{
					const_pointer line = src_img->data( ) + ( ( ( y * y_factor ) >> 8 ) * src_img->pitch( ) );

					for ( int x = 0; x < dst_width; x ++ )
						*dst ++ = ( unsigned char )( line[ 4 * ( ( x * x_factor ) >> 9 ) + uv_offset ] );

					dst += dst_rem;
				}
			}
		}
	}

	return dst_img;
}

static image_type_ptr yuv422_to_rgb( const image_type_ptr &src_img, const opl::wstring &format, int r, int g, int b, int a )
{
	size_type width = src_img->width( );
	size_type height = src_img->height( );

	image_type_ptr dst_img = allocate( src_img, format );

	if ( dst_img != 0 )
	{
		int rgb[ 4 ] = { 255, 255, 255, 255 };

		const_pointer src = src_img->data( );
		size_type src_pitch = src_img->pitch( ) - src_img->linesize( );
		pointer dst = dst_img->data( );
		size_type dst_pitch = dst_img->pitch( ) - dst_img->linesize( );

		size_type orig_width = width;

		while( height -- )
		{
			while( width > 1 )
			{
				yuv444_to_rgb24( rgb[ 0 ], rgb[ 1 ], rgb[ 2 ], *src, *( src + 1 ), *( src + 3 ) );
				*dst ++ = ( unsigned char )rgb[ r ];
				*dst ++ = ( unsigned char )rgb[ g ];
				*dst ++ = ( unsigned char )rgb[ b ];
				if ( a != -1 ) *dst ++ = ( unsigned char )rgb[ a ];
				src += 2;
				yuv444_to_rgb24( rgb[ 0 ], rgb[ 1 ], rgb[ 2 ], *src, *( src - 1 ), *( src + 1 ) );
				*dst ++ = ( unsigned char )rgb[ r ];
				*dst ++ = ( unsigned char )rgb[ g ];
				*dst ++ = ( unsigned char )rgb[ b ];
				if ( a != -1 ) *dst ++ = ( unsigned char )rgb[ a ];
				src += 2;
				width -= 2;
			}

			dst += dst_pitch;
			src += src_pitch;
			width = orig_width;
		}
	}

	return dst_img;
}

static image_type_ptr rgb_to_yuv422( const image_type_ptr &src_img, const opl::wstring &format, int bytes, int r, int g, int b )
{
	size_type width = src_img->width( );
	size_type height = src_img->height( );

	image_type_ptr dst_img = allocate( src_img, format );

	if ( dst_img != 0 )
	{
		int y[ 2 ], u[ 2 ], v[ 2 ];

		const_pointer src = src_img->data( );
		size_type src_pitch = src_img->pitch( ) - src_img->linesize( );
		pointer dst = dst_img->data( );
		size_type dst_pitch = dst_img->pitch( ) - dst_img->linesize( );

		size_type orig_width = width;

		while( height -- )
		{
			width = orig_width;

			while( width > 1 )
			{
				rgb24_to_yuv444( y[ 0 ], u[ 0 ], v[ 0 ], *( src + r ), *( src + g ), *( src + b ) );
				src += bytes;
				rgb24_to_yuv444( y[ 1 ], u[ 1 ], v[ 1 ], *( src + r ), *( src + g ), *( src + b ) );
				src += bytes;
				*dst ++ = ( unsigned char )y[ 0 ];
				*dst ++ = ( unsigned char )( ( u[ 0 ] + u[ 1 ] ) >> 1 );
				*dst ++ = ( unsigned char )y[ 1 ];
				*dst ++ = ( unsigned char )( ( v[ 0 ] + v[ 1 ] ) >> 1 );
				width -= 2;
			}

			dst += dst_pitch;
			src += src_pitch;
		}
	}

	return dst_img;
}

static image_type_ptr yuvp_to_yuv444( const image_type_ptr &src_img, const opl::wstring &format )
{
	image_type_ptr dst_img = allocate( src_img, format );

	if ( dst_img != 0 )
	{
		size_type dst_width = dst_img->width( );
		size_type dst_height = dst_img->height( );
		
		size_type chroma_width = src_img->width( 1 );
		size_type chroma_height = src_img->height( 1 );

		size_type x_factor = ( chroma_width << 8 ) / dst_width;
		size_type y_factor = ( chroma_height << 8 ) / dst_height;

		pointer dst = dst_img->data( );
		size_type dst_rem = dst_img->pitch( ) - dst_img->linesize( );

		int uv_offset;

		for ( int y = 0; y < dst_height; y ++ )
		{
			const_pointer Y = src_img->data( 0 ) + ( y * src_img->pitch( 0 ) );
			const_pointer U = src_img->data( 1 ) + ( ( y * y_factor ) >> 8 ) * src_img->pitch( 1 );
			const_pointer V = src_img->data( 2 ) + ( ( y * y_factor ) >> 8 ) * src_img->pitch( 2 );

			for ( int x = 0; x < dst_width; x ++ )
			{
				uv_offset = ( x * x_factor ) >> 8;
				*dst ++ = ( unsigned char )( *Y ++ );
				*dst ++ = ( unsigned char )( U[ uv_offset ] );
				*dst ++ = ( unsigned char )( V[ uv_offset ] );
			}

			dst += dst_rem;
		}
	}

	return dst_img;
}

static image_type_ptr yuv444_to_yuvp( const image_type_ptr &src_img, const opl::wstring &format )
{
	image_type_ptr dst_img = allocate( src_img, format );

	if ( dst_img != 0 )
	{
		for ( int p = 0; p < 3; p ++ )
		{
			size_type dst_width = dst_img->width( p );
			size_type dst_height = dst_img->height( p );

			pointer dst = dst_img->data( p );
			size_type dst_rem = dst_img->pitch( p ) - dst_img->linesize( p );

			size_type src_height = src_img->height( );

			size_type y_factor = ( src_height << 8 ) / dst_height;

			if ( p == 0 )
			{
				for ( int y = 0; y < dst_height; y ++ )
				{
					const_pointer line = src_img->data( ) + ( y * src_img->pitch( ) );
	
					for ( int x = 0; x < dst_width; x ++ )
					{
						*dst ++ = ( unsigned char )( *line );
						line += 3;
					}

					dst += dst_rem;
				}
			}
			else
			{
				size_type src_width = src_img->width( );
				size_type x_factor = ( src_width << 8 ) / dst_width;

				for ( int y = 0; y < dst_height; y ++ )
				{
					const_pointer line = src_img->data( ) + ( ( ( y * y_factor ) >> 8 ) * src_img->pitch( ) );

					for ( int x = 0; x < dst_width; x ++ )
						*dst ++ = ( unsigned char )( line[ 3 * ( ( x * x_factor ) >> 8 ) + p ] );

					dst += dst_rem;
				}
			}
		}
	}

	return dst_img;
}

static image_type_ptr rgb_to_yuv444( const image_type_ptr &src_img, const opl::wstring &format, int bytes, int r, int g, int b )
{
	size_type width = src_img->width( );
	size_type height = src_img->height( );

	image_type_ptr dst_img = allocate( src_img, format );

	if ( dst_img != 0 )
	{
		int y, u, v;

		const_pointer src = src_img->data( );
		size_type src_pitch = src_img->pitch( ) - src_img->linesize( );
		pointer dst = dst_img->data( );
		size_type dst_pitch = dst_img->pitch( ) - dst_img->linesize( );

		size_type orig_width = width;

		while( height -- )
		{
			width = orig_width;

			while( width -- )
			{
				rgb24_to_yuv444( y, u, v, *( src + r ), *( src + g ), *( src + b ) );
				src += bytes;
				*dst ++ = ( unsigned char )y;
				*dst ++ = ( unsigned char )u;
				*dst ++ = ( unsigned char )v;
			}

			dst += dst_pitch;
			src += src_pitch;
		}
	}

	return dst_img;
}

static image_type_ptr yuv444_to_rgb( const image_type_ptr &src_img, const opl::wstring &format, int r, int g, int b, int a )
{
	size_type width = src_img->width( );
	size_type height = src_img->height( );

	image_type_ptr dst_img = allocate( src_img, format );

	if ( dst_img != 0 )
	{
		int rgb[ 4 ] = { 255, 255, 255, 255 };

		const_pointer src = src_img->data( );
		size_type src_pitch = src_img->pitch( ) - src_img->linesize( );
		pointer dst = dst_img->data( );
		size_type dst_pitch = dst_img->pitch( ) - dst_img->linesize( );

		size_type orig_width = width;

		while( height -- )
		{
			while( width -- )
			{
				yuv444_to_rgb24( rgb[ 0 ], rgb[ 1 ], rgb[ 2 ], *src, *( src + 1 ), *( src + 2 ) );
				src += 3;
				*dst ++ = ( unsigned char )rgb[ r ];
				*dst ++ = ( unsigned char )rgb[ g ];
				*dst ++ = ( unsigned char )rgb[ b ];
				if ( a != -1 ) *dst ++ = ( unsigned char )rgb[ a ];
			}

			dst += dst_pitch;
			src += src_pitch;
			width = orig_width;
		}
	}

	return dst_img;
}

static image_type_ptr yuv444_to_yuv422( const image_type_ptr &src_img, const opl::wstring &format )
{
	size_type width = src_img->width( );
	size_type height = src_img->height( );

	image_type_ptr dst_img = allocate( src_img, format );

	if ( dst_img != 0 )
	{
		const_pointer src = src_img->data( );
		size_type src_pitch = src_img->pitch( ) - src_img->linesize( );

		pointer dst = dst_img->data( );
		size_type dst_pitch = dst_img->pitch( ) - dst_img->linesize( );

		size_type orig_width = width / 2;

		while( height -- )
		{
			width = orig_width;

			while( width -- )
			{
				*dst ++ = src[ 0 ];
				*dst ++ = ( src[ 1 ] + src[ 4 ] ) >> 1;
				*dst ++ = src[ 3 ];
				*dst ++ = ( src[ 2 ] + src[ 5 ] ) >> 1;
				src += 6;
			}

			dst += dst_pitch;
			src += src_pitch;
		}
	}

	return dst_img;
}

static image_type_ptr rgb_to_yuv411( const image_type_ptr &src_img, const opl::wstring &format, int bytes, int r, int g, int b )
{
	size_type width = src_img->width( );
	size_type height = src_img->height( );

	image_type_ptr dst_img = allocate( src_img, format );

	if ( dst_img != 0 )
	{
		int y[ 4 ], u[ 4 ], v[ 4 ];

		const_pointer src = src_img->data( );
		size_type src_pitch = src_img->pitch( ) - src_img->linesize( );
		pointer dst = dst_img->data( );
		size_type dst_pitch = dst_img->pitch( ) - dst_img->linesize( );

		size_type orig_width = width;

		while( height -- )
		{
			width = orig_width;

			while( width > 3 )
			{
				rgb24_to_yuv444( y[ 0 ], u[ 0 ], v[ 0 ], *( src + r ), *( src + g ), *( src + b ) );
				src += bytes;
				rgb24_to_yuv444( y[ 1 ], u[ 1 ], v[ 1 ], *( src + r ), *( src + g ), *( src + b ) );
				src += bytes;
				rgb24_to_yuv444( y[ 2 ], u[ 2 ], v[ 2 ], *( src + r ), *( src + g ), *( src + b ) );
				src += bytes;
				rgb24_to_yuv444( y[ 3 ], u[ 3 ], v[ 3 ], *( src + r ), *( src + g ), *( src + b ) );
				src += bytes;
				*dst ++ = ( unsigned char )y[ 0 ];
				*dst ++ = ( unsigned char )y[ 1 ];
				*dst ++ = ( unsigned char )( ( u[ 0 ] + u[ 1 ] + u[ 2 ] + u[ 3 ] ) >> 2 );
				*dst ++ = ( unsigned char )y[ 2 ];
				*dst ++ = ( unsigned char )y[ 3 ];
				*dst ++ = ( unsigned char )( ( v[ 0 ] + v[ 1 ] + v[ 2 ] + v[ 3 ] ) >> 2 );
				width -= 4;
			}

			dst += dst_pitch;
			src += src_pitch;
		}
	}

	return dst_img;
}

static image_type_ptr yuv411_to_rgb( const image_type_ptr &src_img, const opl::wstring &format, int r, int g, int b, int a )
{
	size_type width = src_img->width( );
	size_type height = src_img->height( );

	image_type_ptr dst_img = allocate( src_img, format );

	if ( dst_img != 0 )
	{
		int rgb[ 4 ] = { 255, 255, 255, 255 };

		const_pointer src = src_img->data( );
		size_type src_pitch = src_img->pitch( ) - src_img->linesize( );
		pointer dst = dst_img->data( );
		size_type dst_pitch = dst_img->pitch( ) - dst_img->linesize( );

		size_type orig_width = width;

		while( height -- )
		{
			while( width > 3 )
			{
				yuv444_to_rgb24( rgb[ 0 ], rgb[ 1 ], rgb[ 2 ], *src, *( src + 2 ), *( src + 5 ) );
				*dst ++ = ( unsigned char )rgb[ r ];
				*dst ++ = ( unsigned char )rgb[ g ];
				*dst ++ = ( unsigned char )rgb[ b ];
				if ( a != -1 ) *dst ++ = ( unsigned char )rgb[ a ];
				yuv444_to_rgb24( rgb[ 0 ], rgb[ 1 ], rgb[ 2 ], *( src + 1 ), *( src + 2 ), *( src + 5 ) );
				*dst ++ = ( unsigned char )rgb[ r ];
				*dst ++ = ( unsigned char )rgb[ g ];
				*dst ++ = ( unsigned char )rgb[ b ];
				if ( a != -1 ) *dst ++ = ( unsigned char )rgb[ a ];
				yuv444_to_rgb24( rgb[ 0 ], rgb[ 1 ], rgb[ 2 ], *( src + 3 ), *( src + 2 ), *( src + 5 ) );
				*dst ++ = ( unsigned char )rgb[ r ];
				*dst ++ = ( unsigned char )rgb[ g ];
				*dst ++ = ( unsigned char )rgb[ b ];
				if ( a != -1 ) *dst ++ = ( unsigned char )rgb[ a ];
				yuv444_to_rgb24( rgb[ 0 ], rgb[ 1 ], rgb[ 2 ], *( src + 4 ), *( src + 2 ), *( src + 5 ) );
				*dst ++ = ( unsigned char )rgb[ r ];
				*dst ++ = ( unsigned char )rgb[ g ];
				*dst ++ = ( unsigned char )rgb[ b ];
				if ( a != -1 ) *dst ++ = ( unsigned char )rgb[ a ];
				src += 6;
				width -= 4;
			}

			dst += dst_pitch;
			src += src_pitch;
			width = orig_width;
		}
	}

	return dst_img;
}

struct rgb_map
{
	opl::wstring from;
	opl::wstring to;
	int bytes_in;
	int in[ 4 ];
	int bytes_out;
	int out[ 4 ];
};

struct rgb_map rgb_mapping[ ] =
{
	{ L"r8g8b8", L"b8g8r8", 3, { 0, 1, 2, 0 }, 3, { 2, 1, 0, 0 } },
	{ L"r8g8b8", L"r8g8b8a8", 3, { 0, 1, 2, 0 }, 4, { 0, 1, 2, 3 } },
	{ L"r8g8b8", L"b8g8r8a8", 3, { 0, 1, 2, 0 }, 4, { 2, 1, 0, 3 } },
	{ L"r8g8b8log", L"b8g8r8a8", 3, { 0, 1, 2, 0 }, 4, { 2, 1, 0, 3 } },
	{ L"r8g8b8", L"a8b8g8r8", 3, { 0, 1, 2, 0 }, 4, { 3, 2, 1, 0 } },
	{ L"b8g8r8", L"r8g8b8", 3, { 2, 1, 0, 0 }, 3, { 0, 1, 2, 0 } },
	{ L"b8g8r8", L"r8g8b8a8", 3, { 2, 1, 0, 0 }, 4, { 0, 1, 2, 3 } },
	{ L"b8g8r8", L"a8b8g8r8", 3, { 2, 1, 0, 0 }, 4, { 3, 2, 1, 0 } },
	{ L"b8g8r8", L"b8g8r8a8", 3, { 2, 1, 0, 0 }, 4, { 2, 1, 0, 3 } },
	{ L"r8g8b8a8", L"r8g8b8", 4, { 0, 1, 2, 0 }, 3, { 0, 1, 2, 0 } },
	{ L"r8g8b8a8", L"b8g8r8", 4, { 0, 1, 2, 0 }, 3, { 2, 1, 0, 0 } },
	{ L"r8g8b8a8", L"b8g8r8a8", 4, { 0, 1, 2, 3 }, 4, { 2, 1, 0, 3 } },
	{ L"b8g8r8a8", L"r8g8b8", 4, { 2, 1, 0, 0 }, 3, { 0, 1, 2, 0 } },
	{ L"b8g8r8a8", L"b8g8r8", 4, { 2, 1, 0, 0 }, 3, { 2, 1, 0, 0 } },
	{ L"b8g8r8a8", L"r8g8b8a8", 4, { 2, 1, 0, 3 }, 4, { 0, 1, 2, 3 } },
	{ L"b8g8r8a8", L"a8b8g8r8", 4, { 2, 1, 0, 3 }, 4, { 3, 2, 1, 0 } },
	{ L"a8r8g8b8", L"a8b8g8r8", 4, { 1, 2, 3, 0 }, 4, { 3, 2, 1, 0 } },
	{ L"a8r8g8b8", L"b8g8r8", 4, { 1, 2, 3, 0 }, 3, { 2, 1, 0, 0 } },
	{ L"a8r8g8b8", L"b8g8r8a8", 4, { 1, 2, 3, 0 }, 4, { 2, 1, 0, 3 } },
	{ L"a8b8g8r8", L"b8g8r8a8", 4, { 3, 2, 1, 0 }, 4, { 2, 1, 0, 3 } },
	{ L"a8b8g8r8", L"r8g8b8a8", 4, { 3, 2, 1, 0 }, 4, { 0, 1, 2, 3 } },
	{ L"l8a8", L"b8g8r8a8", 2, { 0, 0, 0, 1 }, 4, { 2, 1, 0, 3 } },
	{ L"l8a8", L"b8g8r8", 2, { 0, 0, 0, 1 }, 3, { 2, 1, 0, 0 } },
	{ L"", L"", 0, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 } }
};

static image_type_ptr rgb_to_rgb( const image_type_ptr &src_img, const opl::wstring &format )
{
	size_type width = src_img->width( );
	size_type height = src_img->height( );

	image_type_ptr dst_img = allocate( src_img, format );

	if ( dst_img != 0 )
	{
		const_pointer src = src_img->data( );
		size_type src_pitch = src_img->pitch( );
		pointer dst = dst_img->data( );
		size_type dst_pitch = dst_img->pitch( );

		const_pointer sptr = src;
		pointer dptr = dst;

		size_type orig_width = width;

		int i = 0;
		while ( rgb_mapping[ i ].from != L"" )
		{
			if ( rgb_mapping[ i ].from == src_img->pf( ) && rgb_mapping[ i ].to == format )
				break;
			i ++;
		}

		rgb_map &map = rgb_mapping[ i ];
		size_type oc = map.bytes_out;
		size_type ic = map.bytes_in;

		bool has_alpha = ic == 4 || src_img->pf( ) == L"l8a8";

		while( height -- )
		{
			while( width -- )
			{
				switch( oc )
				{
					case 4:		*( dst + map.out[ 3 ] ) = has_alpha  ? *( src + map.in[ 3 ] ) : 255;
					case 3:		*( dst + map.out[ 2 ] ) = *( src + map.in[ 2 ] );
					case 2:		*( dst + map.out[ 1 ] ) = *( src + map.in[ 1 ] );
					case 1:		*( dst + map.out[ 0 ] ) = *( src + map.in[ 0 ] );
				};

				src += ic;
				dst += oc;
			}

			dst = dptr += dst_pitch;
			src = sptr += src_pitch;
			width = orig_width;
		}
	}

	return dst_img;
}

static image_type_ptr r8g8b8p_to_b8g8r8a8( const image_type_ptr &src_img, const opl::wstring &format )
{
	size_type width = src_img->width( );
	size_type height = src_img->height( );

	image_type_ptr dst_img = allocate( src_img, format );
	if( dst_img != 0 )
	{
		const_pointer src0 = src_img->data( 0 );
		size_type src_pitch0 = src_img->pitch( 0 );
		const_pointer src1 = src_img->data( 1 );
		size_type src_pitch1 = src_img->pitch( 1 );
		const_pointer src2 = src_img->data( 2 );
		size_type src_pitch2 = src_img->pitch( 2 );
		pointer dst = dst_img->data( );
		size_type dst_pitch = dst_img->pitch( );

		const_pointer sptr0 = src0;
		const_pointer sptr1 = src1;
		const_pointer sptr2 = src2;
		pointer dptr = dst;

		size_type orig_width = width;

		while( height-- )
		{
			while( width-- )
			{
				*( dst + 0 ) = *src2++;
				*( dst + 1 ) = *src1++;
				*( dst + 2 ) = *src0++;
				*( dst + 3 ) = 255;
				
				dst += 4;
			}
			
			dst = dptr += dst_pitch;
			src0 = sptr0 += src_pitch0;
			src1 = sptr1 += src_pitch1;
			src2 = sptr2 += src_pitch2;
			width = orig_width;
		}
	}
	
	return dst_img;
}

static image_type_ptr r8g8b8a8p_to_b8g8r8a8( const image_type_ptr &src_img, const opl::wstring &format )
{
	size_type width = src_img->width( );
	size_type height = src_img->height( );

	image_type_ptr dst_img = allocate( src_img, format );
	if( dst_img != 0 )
	{
		const_pointer src0 = src_img->data( 0 );
		size_type src_pitch0 = src_img->pitch( 0 );
		const_pointer src1 = src_img->data( 1 );
		size_type src_pitch1 = src_img->pitch( 1 );
		const_pointer src2 = src_img->data( 2 );
		size_type src_pitch2 = src_img->pitch( 2 );
		const_pointer src3 = src_img->data( 3 );
		size_type src_pitch3 = src_img->pitch( 3 );
		pointer dst = dst_img->data( );
		size_type dst_pitch = dst_img->pitch( );

		const_pointer sptr0 = src0;
		const_pointer sptr1 = src1;
		const_pointer sptr2 = src2;
		const_pointer sptr3 = src3;
		pointer dptr = dst;

		size_type orig_width = width;

		while( height-- )
		{
			while( width-- )
			{
				*( dst + 0 ) = *src2++;
				*( dst + 1 ) = *src1++;
				*( dst + 2 ) = *src0++;
				*( dst + 3 ) = *src3++;
				
				dst += 4;
			}
			
			dst = dptr += dst_pitch;
			src0 = sptr0 += src_pitch0;
			src1 = sptr1 += src_pitch1;
			src2 = sptr2 += src_pitch2;
			src3 = sptr3 += src_pitch3;
			width = orig_width;
		}
	}
	
	return dst_img;
}

static image_type_ptr rgb10_rgb12_rgb16_ushort_to_b8g8r8a8( const image_type_ptr &src_img, const opl::wstring &format )
{
	size_type width = src_img->width( );
	size_type height = src_img->height( );

	image_type_ptr dst_img = allocate( src_img, format );
	if( dst_img != 0 )
	{
		const unsigned short* src = ( const unsigned short* ) src_img->data( );
		size_type src_pitch = src_img->pitch( );
		pointer dst = dst_img->data( );
		size_type dst_pitch = dst_img->pitch( );

		const unsigned short* sptr = src;
		pointer dptr = dst;

		size_type orig_width = width;

		while( height-- )
		{
			while( width-- )
			{
				*( dst + 0 ) = static_cast<unsigned char>( *( src + 2 ) >> 8 );
				*( dst + 1 ) = static_cast<unsigned char>( *( src + 1 ) >> 8 );
				*( dst + 2 ) = static_cast<unsigned char>( *( src + 0 ) >> 8 );
				*( dst + 3 ) = 255;
				
				src += 3;
				dst += 4;
			}
			
			dst = dptr += dst_pitch;
			src = sptr += src_pitch;
			width = orig_width;
		}
	}
	
	return dst_img;
}

static image_type_ptr rgb10_rgb12_rgb16_ushort_with_alpha_to_b8g8r8a8( const image_type_ptr &src_img, const opl::wstring &format )
{
	size_type width = src_img->width( );
	size_type height = src_img->height( );

	image_type_ptr dst_img = allocate( src_img, format );
	if( dst_img != 0 )
	{
		const unsigned short* src = ( const unsigned short* ) src_img->data( );
		size_type src_pitch = src_img->pitch( );
		pointer dst = dst_img->data( );
		size_type dst_pitch = dst_img->pitch( );

		const unsigned short* sptr = src;
		pointer dptr = dst;

		size_type orig_width = width;

		while( height-- )
		{
			while( width-- )
			{
				*( dst + 0 ) = static_cast<unsigned char>( *( src + 2 ) >> 8 );
				*( dst + 1 ) = static_cast<unsigned char>( *( src + 1 ) >> 8 );
				*( dst + 2 ) = static_cast<unsigned char>( *( src + 0 ) >> 8 );
				*( dst + 3 ) = static_cast<unsigned char>( *( src + 3 ) >> 8 );
				
				src += 4;
				dst += 4;
			}
			
			dst = dptr += dst_pitch;
			src = sptr += src_pitch;
			width = orig_width;
		}
	}
	
	return dst_img;
}

static image_type_ptr rgb10_rgb12_rgb16p_ushort_to_b8g8r8a8( const image_type_ptr &src_img, const opl::wstring &format )
{
	size_type width = src_img->width( );
	size_type height = src_img->height( );

	image_type_ptr dst_img = allocate( src_img, format );
	if( dst_img != 0 )
	{
		const unsigned short* src0 = ( const unsigned short* ) src_img->data( 0 );
		size_type src_pitch0 = src_img->pitch( 0 );
		const unsigned short* src1 = ( const unsigned short* ) src_img->data( 1 );
		size_type src_pitch1 = src_img->pitch( 1 );
		const unsigned short* src2 = ( const unsigned short* ) src_img->data( 2 );
		size_type src_pitch2 = src_img->pitch( 2 );

		pointer dst = dst_img->data( );
		size_type dst_pitch = dst_img->pitch( );

		const unsigned short* sptr0 = src0;
		const unsigned short* sptr1 = src1;
		const unsigned short* sptr2 = src2;
		pointer dptr = dst;

		size_type orig_width = width;

		while( height-- )
		{
			while( width-- )
			{
				*( dst + 0 ) = static_cast<unsigned char>( *src2++ >> 8 );
				*( dst + 1 ) = static_cast<unsigned char>( *src1++ >> 8 );
				*( dst + 2 ) = static_cast<unsigned char>( *src0++ >> 8 );
				*( dst + 3 ) = 255;
				
				dst += 4;
			}
			
			dst = dptr += dst_pitch;
			src0 = sptr0 += src_pitch0;
			src1 = sptr1 += src_pitch1;
			src2 = sptr2 += src_pitch2;
			width = orig_width;
		}
	}
	
	return dst_img;
}

static image_type_ptr rgb10_rgb12_rgb16p_ushort_with_alpha_to_b8g8r8a8( const image_type_ptr &src_img, const opl::wstring &format )
{
	size_type width = src_img->width( );
	size_type height = src_img->height( );

	image_type_ptr dst_img = allocate( src_img, format );
	if( dst_img != 0 )
	{
		const unsigned short* src0 = ( const unsigned short* ) src_img->data( 0 );
		size_type src_pitch0 = src_img->pitch( 0 );
		const unsigned short* src1 = ( const unsigned short* ) src_img->data( 1 );
		size_type src_pitch1 = src_img->pitch( 1 );
		const unsigned short* src2 = ( const unsigned short* ) src_img->data( 2 );
		size_type src_pitch2 = src_img->pitch( 2 );
		const unsigned short* src3 = ( const unsigned short* ) src_img->data( 3 );
		size_type src_pitch3 = src_img->pitch( 3 );

		pointer dst = dst_img->data( );
		size_type dst_pitch = dst_img->pitch( );

		const unsigned short* sptr0 = src0;
		const unsigned short* sptr1 = src1;
		const unsigned short* sptr2 = src2;
		const unsigned short* sptr3 = src3;
		pointer dptr = dst;

		size_type orig_width = width;

		while( height-- )
		{
			while( width-- )
			{
				*( dst + 0 ) = static_cast<unsigned char>( *src2++ >> 8 );
				*( dst + 1 ) = static_cast<unsigned char>( *src1++ >> 8 );
				*( dst + 2 ) = static_cast<unsigned char>( *src0++ >> 8 );
				*( dst + 3 ) = static_cast<unsigned char>( *src3++ >> 8 );
				
				dst += 4;
			}
			
			dst = dptr += dst_pitch;
			src0 = sptr0 += src_pitch0;
			src1 = sptr1 += src_pitch1;
			src2 = sptr2 += src_pitch2;
			src3 = sptr3 += src_pitch3;
			width = orig_width;
		}
	}
	
	return dst_img;
}

static image_type_ptr l8a8p_to_b8g8r8a8( const image_type_ptr &src_img, const opl::wstring &format )
{
	size_type width = src_img->width( );
	size_type height = src_img->height( );

	image_type_ptr dst_img = allocate( src_img, format );
	if( dst_img != 0 )
	{
		const_pointer src0 = src_img->data( 0 );
		size_type src_pitch0 = src_img->pitch( 0 );
		const_pointer src1 = src_img->data( 1 );
		size_type src_pitch1 = src_img->pitch( 1 );
		pointer dst = dst_img->data( );
		size_type dst_pitch = dst_img->pitch( );

		const_pointer sptr0 = src0;
		const_pointer sptr1 = src1;
		pointer dptr = dst;

		size_type orig_width = width;

		while( height-- )
		{
			while( width-- )
			{
				*( dst + 0 ) = *src0;
				*( dst + 1 ) = *src0;
				*( dst + 2 ) = *src0++;
				*( dst + 3 ) = *src1++;
				
				dst += 4;
			}
			
			dst = dptr += dst_pitch;
			src0 = sptr0 += src_pitch0;
			src1 = sptr1 += src_pitch1;
			width = orig_width;
		}
	}
	
	return dst_img;
}

static image_type_ptr l10_l12_l16p_ushort_with_alpha_to_b8g8r8a8( const image_type_ptr &src_img, const opl::wstring &format )
{
	size_type width = src_img->width( );
	size_type height = src_img->height( );

	image_type_ptr dst_img = allocate( src_img, format );
	if( dst_img != 0 )
	{
		const unsigned short* src0 = reinterpret_cast<const unsigned short*>( src_img->data( 0 ) );
		size_type src_pitch0 = src_img->pitch( 0 );
		const unsigned short* src1 = reinterpret_cast<const unsigned short*>( src_img->data( 1 ) );
		size_type src_pitch1 = src_img->pitch( 1 );
		pointer dst = dst_img->data( );
		size_type dst_pitch = dst_img->pitch( );

		const unsigned short* sptr0 = src0;
		const unsigned short* sptr1 = src1;
		pointer dptr = dst;

		size_type orig_width = width;

		while( height-- )
		{
			while( width-- )
			{
				*( dst + 0 ) = static_cast<unsigned char>( *src0 >> 8 );
				*( dst + 1 ) = static_cast<unsigned char>( *src0 >> 8 );
				*( dst + 2 ) = static_cast<unsigned char>( *src0 >> 8 );
				*( dst + 3 ) = static_cast<unsigned char>( *src1 >> 8 );
				
				++src0;
				++src1;
				dst += 4;
			}
			
			dst = dptr += dst_pitch;
			src0 = sptr0 += src_pitch0;
			src1 = sptr1 += src_pitch1;
			width = orig_width;
		}
	}
	
	return dst_img;
}

// Truncates 32-bit float to b8g8r8a8. The intention is to perform all tonemaps at the float level and convert to 8-bit only when needed.
// Assumes normalise( im, 255.0f ) has already been called.
static image_type_ptr r32g32b32f_to_b8g8r8a8( image_type_ptr src_img, const opl::wstring &format )
{
	size_type width = src_img->width( );
	size_type height = src_img->height( );

	image_type_ptr dst_img = allocate( src_img, format );
	if( dst_img != 0 )
	{
		const float* src = ( const float* ) src_img->data( );
		size_type src_pitch = src_img->pitch( );
		pointer dst = dst_img->data( );
		size_type dst_pitch = dst_img->pitch( );

		const float* sptr = src;
		pointer dptr = dst;

		size_type orig_width = width;

		while( height-- )
		{
			while( width-- )
			{
				*( dst + 2 ) = static_cast<unsigned char>( opl::fast_floorf( *src++ ) );
				*( dst + 1 ) = static_cast<unsigned char>( opl::fast_floorf( *src++ ) );
				*( dst + 0 ) = static_cast<unsigned char>( opl::fast_floorf( *src++ ) );

				*( dst + 3 ) = 255;

				dst += 4;
			}

			dst = dptr += dst_pitch;
			src = sptr += src_pitch;
			width = orig_width;
		}
	}
	
	return dst_img;
}

static void fill( image_type_ptr img, int plane, boost::uint8_t value )
{
	boost::uint8_t *dst = img->data( plane );
	size_type width = img->width( plane );
	size_type height = img->height( plane );
	size_type pitch = img->pitch( plane );

	while( height -- )
	{
		memset( dst, value, width );
		dst += pitch;
	}
}

static image_type_ptr l8_to_yuv_planar( image_type_ptr src_img, const opl::wstring &format )
{
	size_type width = src_img->width( );
	size_type height = src_img->height( );

	image_type_ptr dst_img = allocate( src_img, format );
	if( dst_img != 0 )
	{
		boost::uint8_t *src = src_img->data( );
		boost::uint8_t *dst = dst_img->data( );

		size_type src_pitch = src_img->pitch( );
		size_type dst_pitch = dst_img->pitch( );

		while( height -- )
		{
			memcpy( dst, src, width );
			dst += dst_pitch;
			src += src_pitch;
		}

		fill( dst_img, 1, 128 );
		fill( dst_img, 2, 128 );
	}

	return dst_img;
}

static image_type_ptr f32_to_yuv_planar( image_type_ptr src_img, const opl::wstring &format )
{
	size_type height = src_img->height( );

	image_type_ptr dst_img = allocate( src_img, format );
	if( dst_img != 0 )
	{
		float *src = reinterpret_cast< float * >( src_img->data( ) );
		boost::uint8_t *dst = dst_img->data( );

		size_type src_pitch = src_img->pitch( ) - src_img->linesize( );
		size_type dst_pitch = dst_img->pitch( ) - dst_img->linesize( );
		int width;

		while( height -- )
		{
			width = dst_img->width( );
			while( width -- )
				*dst ++ = boost::uint8_t( 255 * *src ++ );
			dst += dst_pitch;
			src += src_pitch;
		}

		fill( dst_img, 1, 128 );
		fill( dst_img, 2, 128 );
	}

	return dst_img;
}

static image_type_ptr yuv_planar_to_l8( image_type_ptr src_img )
{
	size_type width = src_img->width( );
	size_type height = src_img->height( );

	image_type_ptr dst_img = allocate( src_img, L"l8" );
	if( dst_img != 0 )
	{
		boost::uint8_t *src = src_img->data( );
		boost::uint8_t *dst = dst_img->data( );

		size_type src_pitch = src_img->pitch( );
		size_type dst_pitch = dst_img->pitch( );

		while( height -- )
		{
			memcpy( dst, src, width );
			dst += dst_pitch;
			src += src_pitch;
		}
	}

	return dst_img;
}

inline bool is_rgb_packed( const opl::wstring &pf )
{
	return pf == L"a8b8g8r8" || pf == L"a8r8g8b8" || pf == L"b8g8r8" || pf == L"b8g8r8a8" || pf == L"r8g8b8" || pf == L"r8g8b8a8";
}

IL_DECLSPEC image_type_ptr convert( const image_type_ptr &src, const opl::wstring &dst_pf, int )
{
	// Sanity check
	if ( src == 0 )
		return src;

	// Get the format of the source
	opl::wstring src_pf = src->pf( );

	// Convert
	if ( dst_pf == src_pf )
	{
		return src;
	}
	else if ( is_yuv_planar( src ) && is_yuv_planar( dst_pf ) )
	{
		return yuvp_to_yuvp( src, dst_pf );
	}
	else if ( is_yuv_planar( src ) && dst_pf == L"yuv422" )
	{
#ifdef HAVE_MMX
		if ( src_pf == L"yuv420p" && dst_pf == L"yuv422" )
			return yuv420p_to_yuv422( src, dst_pf );
#endif
		return yuvp_to_yuv422( src, dst_pf );
	}
	else if ( is_yuv_planar( src ) && dst_pf == L"yuv444" )
	{
		if ( src_pf == L"yuv420p" && dst_pf == L"yuv444" )
			return yuv420p_to_yuv444( src, dst_pf );
		return yuvp_to_yuv444( src, dst_pf );
	}
	else if ( is_yuv_planar( src ) && is_rgb_packed( dst_pf ) )
	{
		if ( src_pf == L"yuv420p" && dst_pf == L"r8g8b8" )
			return yuv420p_to_rgb( src, dst_pf );
		if ( src_pf == L"yuv420p" && dst_pf == L"b8g8r8" )
			return yuv420p_to_bgr( src, dst_pf );
		if ( dst_pf == L"a8b8g8r8" )
			return yuvp_to_rgb( src, dst_pf, 3, 2, 1, 0 );
		if ( dst_pf == L"a8r8g8b8" )
			return yuvp_to_rgb( src, dst_pf, 1, 2, 3, 0 );
		if ( dst_pf == L"b8g8r8" )
			return yuvp_to_rgb( src, dst_pf, 2, 1, 0, -1 );
		if ( dst_pf == L"b8g8r8a8" )
			return yuvp_to_rgb( src, dst_pf, 2, 1, 0, 3 );
		if ( dst_pf == L"r8g8b8" )
			return yuvp_to_rgb( src, dst_pf, 0, 1, 2, -1 );
		if ( dst_pf == L"r8g8b8a8" )
			return yuvp_to_rgb( src, dst_pf, 0, 1, 2, 3 );
	}
	else if ( is_rgb_packed( src_pf ) && is_yuv_planar( dst_pf ) )
	{
		if ( src_pf == L"a8b8g8r8" )
			return rgb_to_yuvp( src, dst_pf, 3, 2, 1, 0 );
		if ( src_pf == L"a8r8g8b8" )
			return rgb_to_yuvp( src, dst_pf, 1, 2, 3, 0 );
		if ( src_pf == L"b8g8r8" )
			return rgb_to_yuvp( src, dst_pf, 2, 1, 0, -1 );
		if ( src_pf == L"b8g8r8a8" )
			return rgb_to_yuvp( src, dst_pf, 2, 1, 0, 3 );
		if ( src_pf == L"r8g8b8" )
			return rgb_to_yuvp( src, dst_pf, 0, 1, 2, -1 );
		if ( src_pf == L"r8g8b8a8" )
			return rgb_to_yuvp( src, dst_pf, 0, 1, 2, 3 );
	}
	else if ( src_pf == L"yuv422" && is_yuv_planar( dst_pf ) )
	{
		return yuv422_to_yuvp( src, dst_pf );
	}
	else if ( src_pf == L"yuv444" && is_yuv_planar( dst_pf ) )
	{
		return yuv444_to_yuvp( src, dst_pf );
	}
	else if ( src_pf == L"r8g8b8" )
	{
		if ( dst_pf == L"b8g8r8" )
			return rgb_to_rgb( src, dst_pf );
		else if ( dst_pf == L"r8g8b8a8" )
			return rgb_to_rgb( src, dst_pf );
		else if ( dst_pf == L"b8g8r8a8" )
			return rgb_to_rgb( src, dst_pf );
		else if ( dst_pf == L"yuv444" )
			return rgb_to_yuv444( src, dst_pf, 3, 0, 1, 2 );
		else if ( dst_pf == L"yuv422" )
			return rgb_to_yuv422( src, dst_pf, 3, 0, 1, 2 );
		else if ( dst_pf == L"yuv411" )
			return rgb_to_yuv411( src, dst_pf, 3, 0, 1, 2 );
	}
	else if ( src_pf == L"r8g8b8p" )
	{
		if( dst_pf == L"b8g8r8a8" )
			return r8g8b8p_to_b8g8r8a8( src, dst_pf );
	}
	else if ( src_pf == L"b8g8r8" )
	{
		if ( dst_pf == L"r8g8b8" )
			return rgb_to_rgb( src, dst_pf );
		else if ( dst_pf == L"r8g8b8a8" )
			return rgb_to_rgb( src, dst_pf );
		else if ( dst_pf == L"b8g8r8a8" )
			return rgb_to_rgb( src, dst_pf );
		else if ( dst_pf == L"yuv444" )
			return rgb_to_yuv444( src, dst_pf, 3, 2, 1, 0 );
		else if ( dst_pf == L"yuv422" )
			return rgb_to_yuv422( src, dst_pf, 3, 2, 1, 0 );
		else if ( dst_pf == L"yuv411" )
			return rgb_to_yuv411( src, dst_pf, 3, 2, 1, 0 );
	}
	else if ( src_pf == L"r8g8b8a8" )
	{
		if ( dst_pf == L"r8g8b8" )
			return rgb_to_rgb( src, dst_pf );
		else if ( dst_pf == L"b8g8r8" )
			return rgb_to_rgb( src, dst_pf );
		else if ( dst_pf == L"b8g8r8a8" )
			return rgb_to_rgb( src, dst_pf );
		else if ( dst_pf == L"yuv444" )
			return rgb_to_yuv444( src, dst_pf, 4, 0, 1, 2 );
		else if ( dst_pf == L"yuv422" )
			return rgb_to_yuv422( src, dst_pf, 4, 0, 1, 2 );
		else if ( dst_pf == L"yuv411" )
			return rgb_to_yuv411( src, dst_pf, 4, 0, 1, 2 );
	}
	else if ( src_pf == L"r8g8b8a8p" )
	{
		if( dst_pf == L"b8g8r8a8" )
			return r8g8b8a8p_to_b8g8r8a8( src, dst_pf );
	}
	else if ( src_pf == L"b8g8r8a8" )
	{
		if ( dst_pf == L"r8g8b8" )
			return rgb_to_rgb( src, dst_pf );
		else if ( dst_pf == L"r8g8b8a8" )
			return rgb_to_rgb( src, dst_pf );
		else if ( dst_pf == L"b8g8r8" )
			return rgb_to_rgb( src, dst_pf );
		else if ( dst_pf == L"yuv444" )
			return rgb_to_yuv444( src, dst_pf, 4, 2, 1, 0 );
		else if ( dst_pf == L"yuv422" )
			return rgb_to_yuv422( src, dst_pf, 4, 2, 1, 0 );
		else if ( dst_pf == L"yuv411" )
			return rgb_to_yuv411( src, dst_pf, 4, 2, 1, 0 );
	}
	else if ( src_pf == L"a8r8g8b8" )
	{
		if( dst_pf == L"a8b8g8r8" )
			return rgb_to_rgb( src, dst_pf );
		else if( dst_pf == L"b8g8r8")
			return rgb_to_rgb( src, dst_pf );
		else if( dst_pf == L"b8g8r8a8")
			return rgb_to_rgb( src, dst_pf );
		else if ( dst_pf == L"yuv444" )
			return rgb_to_yuv444( src, dst_pf, 4, 1, 2, 3 );
		else if ( dst_pf == L"yuv422" )
			return rgb_to_yuv422( src, dst_pf, 4, 1, 2, 3 );
		else if ( dst_pf == L"yuv411" )
			return rgb_to_yuv411( src, dst_pf, 4, 1, 2, 3 );
	}
	else if( src_pf == L"a8b8g8r8" )
	{
		if( dst_pf == L"b8g8r8a8" )
			return rgb_to_rgb( src, dst_pf );
		else if( dst_pf == L"r8g8b8a8" )
			return rgb_to_rgb( src, dst_pf );
		else if ( dst_pf == L"yuv444" )
			return rgb_to_yuv444( src, dst_pf, 4, 3, 2, 1 );
		else if ( dst_pf == L"yuv422" )
			return rgb_to_yuv422( src, dst_pf, 4, 3, 2, 1 );
		else if ( dst_pf == L"yuv411" )
			return rgb_to_yuv411( src, dst_pf, 4, 3, 2, 1 );
	}
	else if( src_pf == L"r8g8b8log" )
	{
		if( dst_pf == L"b8g8r8a8" )
			return convert_log_to_linear( rgb_to_rgb( src, dst_pf ) );
	}
	else if( src_pf == L"r8g8b8a8log" )
	{
		if( dst_pf == L"b8g8r8a8" )
			return convert_log_to_linear( rgb_to_rgb( src, dst_pf ) );
	}
	else if( src_pf == L"r10g10b10" )
	{
		if( dst_pf == L"b8g8r8a8" )
			return rgb10_rgb12_rgb16_ushort_to_b8g8r8a8( src, dst_pf );
	}
	else if( src_pf == L"r10g10b10a10" )
	{
		if( dst_pf == L"b8g8r8a8" )
			return rgb10_rgb12_rgb16_ushort_with_alpha_to_b8g8r8a8( src, dst_pf );
	}
	else if( src_pf == L"r10g10b10p" )
	{
		if( dst_pf == L"b8g8r8a8" )
			return rgb10_rgb12_rgb16p_ushort_to_b8g8r8a8( src, dst_pf );
	}
	else if( src_pf == L"r10g10b10a10p" )
	{
		if( dst_pf == L"b8g8r8a8" )
			return rgb10_rgb12_rgb16p_ushort_with_alpha_to_b8g8r8a8( src, dst_pf );
	}
	else if( src_pf == L"r10g10b10log" )
	{
		if( dst_pf == L"b8g8r8a8" )
			return convert_log_to_linear( rgb10_rgb12_rgb16_ushort_to_b8g8r8a8( src, dst_pf ) );
	}
	else if( src_pf == L"r10g10b10a10log" )
	{
		if( dst_pf == L"b8g8r8a8" )
			return convert_log_to_linear( rgb10_rgb12_rgb16_ushort_with_alpha_to_b8g8r8a8( src, dst_pf ) );
	}
	else if( src_pf == L"r12g12b12" )
	{
		if( dst_pf == L"b8g8r8a8" )
			return rgb10_rgb12_rgb16_ushort_to_b8g8r8a8( src, dst_pf );
	}
	else if( src_pf == L"r12g12b12a12" )
	{
		if( dst_pf == L"b8g8r8a8" )
			return rgb10_rgb12_rgb16_ushort_with_alpha_to_b8g8r8a8( src, dst_pf );
	}
	else if( src_pf == L"r12g12b12p" )
	{
		if( dst_pf == L"b8g8r8a8" )
			return rgb10_rgb12_rgb16p_ushort_to_b8g8r8a8( src, dst_pf );
	}
	else if( src_pf == L"r12g12b12a12p" )
	{
		if( dst_pf == L"b8g8r8a8" )
			return rgb10_rgb12_rgb16p_ushort_with_alpha_to_b8g8r8a8( src, dst_pf );
	}
	else if( src_pf == L"r12g12b12log" )
	{
		if( dst_pf == L"b8g8r8a8" )
			return convert_log_to_linear( rgb10_rgb12_rgb16_ushort_to_b8g8r8a8( src, dst_pf ) );
	}
	else if( src_pf == L"r12g12b12a12log" )
	{
		if( dst_pf == L"b8g8r8a8" )
			return convert_log_to_linear( rgb10_rgb12_rgb16_ushort_with_alpha_to_b8g8r8a8( src, dst_pf ) );
	}
	else if( src_pf == L"r16g16b16" )
	{
		if( dst_pf == L"b8g8r8a8" )
			return rgb10_rgb12_rgb16_ushort_to_b8g8r8a8( src, dst_pf );
	}
	else if( src_pf == L"r16g16b16a16" )
	{
		if( dst_pf == L"b8g8r8a8" )
			return rgb10_rgb12_rgb16_ushort_with_alpha_to_b8g8r8a8( src, dst_pf );
	}
	else if( src_pf == L"r16g16b16p" )
	{
		if( dst_pf == L"b8g8r8a8" )
			return rgb10_rgb12_rgb16p_ushort_to_b8g8r8a8( src, dst_pf );
	}
	else if( src_pf == L"r16g16b16a16p" )
	{
		if( dst_pf == L"b8g8r8a8" )
			return rgb10_rgb12_rgb16p_ushort_with_alpha_to_b8g8r8a8( src, dst_pf );
	}
	else if( src_pf == L"r16g16b16log" )
	{
		if( dst_pf == L"b8g8r8a8" )
			return convert_log_to_linear( rgb10_rgb12_rgb16_ushort_to_b8g8r8a8( src, dst_pf ) );
	}
	else if( src_pf == L"r16g16b16a16log" )
	{
		if( dst_pf == L"b8g8r8a8" )
			return convert_log_to_linear( rgb10_rgb12_rgb16_ushort_with_alpha_to_b8g8r8a8( src, dst_pf ) );
	}
	else if( src_pf == L"r32g32b32f" )
	{
		if( dst_pf == L"b8g8r8a8" )
			return tm_linear( src );
		else if( dst_pf == L"ldr8" ) // stub colour space for straight truncation to LDR data.
			return r32g32b32f_to_b8g8r8a8( src, L"b8g8r8a8" );
	}
	else if ( src_pf == L"yuv444" )
	{
		if ( dst_pf == L"r8g8b8" )
			return yuv444_to_rgb( src, dst_pf, 0, 1, 2, -1 );
		else if ( dst_pf == L"b8g8r8" )
			return yuv444_to_rgb( src, dst_pf, 2, 1, 0, -1 );
		else if ( dst_pf == L"r8g8b8a8" )
			return yuv444_to_rgb( src, dst_pf, 0, 1, 2, 3 );
		else if ( dst_pf == L"b8g8r8a8" )
			return yuv444_to_rgb( src, dst_pf, 2, 1, 0, 3 );
		else if ( dst_pf == L"yuv422" )
			return yuv444_to_yuv422( src, dst_pf );
	}
	else if ( src_pf == L"yuv422" )
	{
		if ( dst_pf == L"r8g8b8" )
			return yuv422_to_rgb( src, dst_pf, 0, 1, 2, -1 );
		else if ( dst_pf == L"b8g8r8" )
			return yuv422_to_rgb( src, dst_pf, 2, 1, 0, -1 );
		else if ( dst_pf == L"r8g8b8a8" )
			return yuv422_to_rgb( src, dst_pf, 0, 1, 2, 3 );
		else if ( dst_pf == L"b8g8r8a8" )
			return yuv422_to_rgb( src, dst_pf, 2, 1, 0, 3 );
	}
	else if ( src_pf == L"yuv411" )
	{
		if ( dst_pf == L"r8g8b8" )
			return yuv411_to_rgb( src, dst_pf, 0, 1, 2, -1 );
		else if ( dst_pf == L"b8g8r8" )
			return yuv411_to_rgb( src, dst_pf, 2, 1, 0, -1 );
		else if ( dst_pf == L"r8g8b8a8" )
			return yuv411_to_rgb( src, dst_pf, 0, 1, 2, 3 );
		else if ( dst_pf == L"b8g8r8a8" )
			return yuv411_to_rgb( src, dst_pf, 2, 1, 0, 3 );
	}
	else if( src_pf == L"l8a8" )
	{
		if( dst_pf == L"b8g8r8a8" )
			return rgb_to_rgb( src, dst_pf );
		else if( dst_pf == L"b8g8r8" )
			return rgb_to_rgb( src, dst_pf );
		else if( dst_pf == L"r8g8b8a8" )
			return rgb_to_rgb( src, dst_pf );
	}
	else if( src_pf == L"l8a8p" )
	{
		if( dst_pf == L"b8g8r8a8" )
			return l8a8p_to_b8g8r8a8( src, dst_pf );
	}
	else if( src_pf == L"l10a10p" )
	{
		if( dst_pf == L"b8g8r8a8" )
			return l10_l12_l16p_ushort_with_alpha_to_b8g8r8a8( src, dst_pf );
	}
	else if( src_pf == L"l12a12p" )
	{
		if( dst_pf == L"b8g8r8a8" )
			return l10_l12_l16p_ushort_with_alpha_to_b8g8r8a8( src, dst_pf );
	}
	else if( src_pf == L"l16a16p" )
	{
		if( dst_pf == L"b8g8r8a8" )
			return l10_l12_l16p_ushort_with_alpha_to_b8g8r8a8( src, dst_pf );
	}
  	else if ( src_pf == L"l8" && is_yuv_planar( dst_pf ) )
  	{
  		return l8_to_yuv_planar( src, dst_pf );
  	}
  	else if ( src_pf == L"f32" && is_yuv_planar( dst_pf ) )
  	{
  		return f32_to_yuv_planar( src, dst_pf );
  	}
  	else if ( is_yuv_planar( src_pf ) && dst_pf == L"l8" )
  	{
  		return yuv_planar_to_l8( src );
  	}

#ifndef WIN32
	//fprintf( stderr, "Unknown %s to %s\n", opl::to_string( src_pf ).c_str( ), opl::to_string( dst_pf ).c_str( ) );
#endif

	return image_type_ptr( );
}

static void rescale_plane( image_type_ptr &new_im, const image_type_ptr& im, int new_d, int bs, rescale_filter filter, const int &p )
{
	int src_w = im->width( p );
	int src_h = im->height( p );
	int src_d = im->depth( );
	int src_p = im->pitch( p );

	int new_w = new_im->width( p );
	int new_h = new_im->height( p );
  		
	image_type::const_pointer	src = im->data( p );
	image_type::pointer			dst = new_im->data( p );

	if ( src_w == new_w && src_h == new_h && src_d == new_d && bs == 1 )
	{
		int linesize = new_im->linesize( p );
		int dst_p = new_im->pitch( p );
		while( src_h -- )
		{
			memcpy( dst, src, linesize );
			dst += dst_p;
			src += src_p;
		}
	}
	else
	{
		int diff = new_im->pitch( p ) - new_im->linesize( p );

		image_type::const_pointer	src = im->data( p );
		image_type::pointer			dst = new_im->data( p );

		if( filter == POINT_SAMPLING )
		{
			image_type::const_pointer	tmp;
			image_type::const_pointer	row;
			int k, dk;
			int ho = ( ( src_w - 1 ) << 16 ) / new_w;
			int hm = ho * new_w;
			int vo = ( ( src_h - 1 ) << 16 ) / new_h;
			int vm = vo * new_h;
			int ht = 0;
			int vt = 0;

			src_d --;

			if ( new_d != 1 || bs != 1 )
			{
				for( k = 0; k < new_d; ++k )
				{
					dk = ( k * src_d / new_d ) * src_h;
					vt = 0;
	
					while( vt < vm )
					{
						row = src + dk + ( vt >> 16 ) * src_p;
						ht = 0;
	
						while( ht < hm )
						{
							tmp = row + bs * ( ht >> 16 );
							switch( bs )
							{
								case 4: *dst ++ = *tmp ++;
								case 3: *dst ++ = *tmp ++;
								case 2: *dst ++ = *tmp ++;
								case 1: *dst ++ = *tmp ++;
							}
							ht += ho;
						}
						
						dst += diff;
						vt += vo;
					}
				}
			}
			else
			{
				while( vt < vm )
				{
					row = src + ( vt >> 16 ) * src_p;
					ht = 0;

					while( ht < hm )
					{
						*dst ++ = row[ ht >> 16 ];
						ht += ho;
					}
					
					dst += diff;
					vt += vo;
				}
			}
		}
		else if( filter == BILINEAR_SAMPLING )
		{
			if ( new_d != 1 || bs != 1 )
			{
				float ddi = float( src_w - 1 ) / (std::max)( new_w - 1, 1 );
				float ddj = float( src_h - 1 ) / (std::max)( new_h - 1, 1 );
				float ddk = float( src_d - 1 ) / (std::max)( new_d - 1, 1 );

				for( int k = 0; k < new_d; ++k )
				{
					float sk = k * ddk;
					int dk = ( int ) sk;
					sk -= dk;
					int dk0 = (std::min)( dk + 1, src_d - 1 );
					for( int j = 0; j < new_h; ++j )
					{
						float sj = j * ddj;
						int dj = ( int ) sj;
						sj -= dj;
						int dj0 = (std::min)( dj + 1, src_h - 1 );
						for( int i = 0; i < new_w; ++i )
						{
							float si = i * ddi;
							int di = ( int ) si;
							si -= di;
							int di0 = (std::min)( di + 1, src_w - 1 );
							for( int c = 0; c < bs; ++c )
							{
								*dst++ = static_cast<unsigned char>( (
									( ( src[ ( dk  * src_h + dj  ) * src_p + di  * bs + c ] * ( 1.0f - si ) +
									  	src[ ( dk  * src_h + dj  ) * src_p + di0 * bs + c ] * (        si ) ) * ( 1.0f - sj ) +
									  ( src[ ( dk  * src_h + dj0 ) * src_p + di  * bs + c ] * ( 1.0f - si ) +
									  	src[ ( dk  * src_h + dj0 ) * src_p + di0 * bs + c ] * (        si ) ) *          sj ) * ( 1.0f - sk ) +
									( ( src[ ( dk0 * src_h + dj  ) * src_p + di  * bs + c ] * ( 1.0f - si ) +
									  	src[ ( dk0 * src_h + dj  ) * src_p + di0 * bs + c ] * (        si ) ) * ( 1.0f - sj ) +
									  ( src[ ( dk0 * src_h + dj0 ) * src_p + di  * bs + c ] * ( 1.0f - si ) +
									  	src[ ( dk0 * src_h + dj0 ) * src_p + di0 * bs + c ] * (        si ) ) *          sj ) *          sk ) );
							}					
						}

						dst += diff;
					}
				}
			}
			else
			{
				int j, sj, dj, dj0, i, si, di, di0;
				int ddi = ( src_w << 8 ) / (std::max)( new_w, 1 );
				int ddj = ( src_h << 8 ) / (std::max)( new_h, 1 );
				int mj = ddj * new_h;
				int mi = ddi * new_w;
				image_type::const_pointer row_upper;
				image_type::const_pointer row_lower;

				for( j = 0; j < mj; j += ddj )
				{
					dj = j >> 8;
					sj = j & 0xff;
					dj0 = (std::min)( dj + 1, src_h - 1 );
					row_upper = src + dj  * src_p;
					row_lower = src + dj0 * src_p;

					for( i = 0; i < mi; i += ddi )
					{
						di = i >> 8;
						si = i & 0xff;
						di0 = (std::min)( di + 1, src_w );
						di = std::min<int>( std::max<int>( di, 0 ), src_w - 1 );
						di0 = std::min<int>( std::max<int>( di0, 0 ), src_w - 1 );
						*dst ++ = static_cast<unsigned char>( (
									( row_upper[ di ] * ( 0x100 - si ) + row_upper[ di0 ] * si ) * ( 0x100 - sj ) + 
								    ( row_lower[ di ] * ( 0x100 - si ) + row_lower[ di0 ] * si ) * (         sj ) +
									( row_upper[ di ] * ( 0x100 - si ) + row_upper[ di0 ] * si ) * ( 0x100 - sj ) + 
									( row_lower[ di ] * ( 0x100 - si ) + row_lower[ di0 ] * si ) * (         sj ) ) >> 17 );
					}					

					dst += diff;
				}
			}
		}
		else if( filter == BICUBIC_SAMPLING )
		{
			if ( new_d != 1 || bs != 1 )
			{
				float ddi = float( src_w ) / (std::max)( new_w , 1 );
				float ddj = float( src_h ) / (std::max)( new_h , 1 );
				float ddk = float( src_d ) / (std::max)( new_d , 1 );
	
				for( int k = 0; k < new_d; ++k )
				{
					float sk = k * ddk;
					int dk = ( int ) sk;
					sk -= dk;
					int dk0 = (std::max)( dk - 1, 0 );
					int dk1 = (std::min)( dk + 1, src_d - 1 );
					int dk2 = (std::min)( dk + 2, src_d - 1 );
					float rk0 = R( -1.0f - sk );
					float rk1 = R( -sk );
					float rk2 = R( 1.0f - sk );
					float rk3 = R( 2.0f - sk );
					for( int j = 0; j < new_h; ++j )
					{
						float sj = j * ddj;
						int dj = ( int ) sj;
						sj -= dj;
						int dj0 = (std::max)( dj - 1, 0 );
						int dj1 = (std::min)( dj + 1, src_h );
						int dj2 = (std::min)( dj + 2, src_h );
						float rj0 = R( -1.0f - sj );
						float rj1 = R( -sj );
						float rj2 = R( 1.0f - sj );
						float rj3 = R( 2.0f - sj );
						for( int i = 0; i < new_w; ++i )
						{
							float si = i * ddi;
							int di = ( int ) si;
							si -= di;
							int di0 = (std::max)( di - 1, 0 );
							int di1 = (std::min)( di + 1, src_w );
							int di2 = (std::min)( di + 2, src_w );
							float ri0 = R( -1.0f - si );
							float ri1 = R( -si );
							float ri2 = R( 1.0f - si );
							float ri3 = R( 2.0f - si );
							for( int c = 0; c < bs; ++c )
							{
								*dst++ = ( unsigned char ) (
									( ( src[ ( dk0 * src_h + dj0 ) * src_p + di0  * bs + c ] * ri0 +
										src[ ( dk0 * src_h + dj0 ) * src_p + di   * bs + c ] * ri1 +
										src[ ( dk0 * src_h + dj0 ) * src_p + di1  * bs + c ] * ri2 +
										src[ ( dk0 * src_h + dj0 ) * src_p + di2  * bs + c ] * ri3 ) * rj0 +
									  ( src[ ( dk0 * src_h + dj  ) * src_p + di0  * bs + c ] * ri0 +
										src[ ( dk0 * src_h + dj  ) * src_p + di   * bs + c ] * ri1 +
										src[ ( dk0 * src_h + dj  ) * src_p + di1  * bs + c ] * ri2 +
										src[ ( dk0 * src_h + dj  ) * src_p + di2  * bs + c ] * ri3 ) * rj1 +
									  ( src[ ( dk0 * src_h + dj1 ) * src_p + di0  * bs + c ] * ri0 +
										src[ ( dk0 * src_h + dj1 ) * src_p + di   * bs + c ] * ri1 +
										src[ ( dk0 * src_h + dj1 ) * src_p + di1  * bs + c ] * ri2 +
										src[ ( dk0 * src_h + dj1 ) * src_p + di2  * bs + c ] * ri3 ) * rj2 +
									  ( src[ ( dk0 * src_h + dj2 ) * src_p + di0  * bs + c ] * ri0 +
										src[ ( dk0 * src_h + dj2 ) * src_p + di   * bs + c ] * ri1 +
										src[ ( dk0 * src_h + dj2 ) * src_p + di1  * bs + c ] * ri2 +
										src[ ( dk0 * src_h + dj2 ) * src_p + di2  * bs + c ] * ri3 ) * rj3 ) * rk0 +
	
									( ( src[ ( dk  * src_h + dj0 ) * src_p + di0  * bs + c ] * ri0 +
										src[ ( dk  * src_h + dj0 ) * src_p + di   * bs + c ] * ri1 +
										src[ ( dk  * src_h + dj0 ) * src_p + di1  * bs + c ] * ri2 +
										src[ ( dk  * src_h + dj0 ) * src_p + di2  * bs + c ] * ri3 ) * rj0 +
									  ( src[ ( dk  * src_h + dj  ) * src_p + di0  * bs + c ] * ri0 +
										src[ ( dk  * src_h + dj  ) * src_p + di   * bs + c ] * ri1 +
										src[ ( dk  * src_h + dj  ) * src_p + di1  * bs + c ] * ri2 +
										src[ ( dk  * src_h + dj  ) * src_p + di2  * bs + c ] * ri3 ) * rj1 +
									  ( src[ ( dk  * src_h + dj1 ) * src_p + di0  * bs + c ] * ri0 +
										src[ ( dk  * src_h + dj1 ) * src_p + di   * bs + c ] * ri1 +
										src[ ( dk  * src_h + dj1 ) * src_p + di1  * bs + c ] * ri2 +
										src[ ( dk  * src_h + dj1 ) * src_p + di2  * bs + c ] * ri3 ) * rj2 +
									  ( src[ ( dk  * src_h + dj2 ) * src_p + di0  * bs + c ] * ri0 +
										src[ ( dk  * src_h + dj2 ) * src_p + di   * bs + c ] * ri1 +
										src[ ( dk  * src_h + dj2 ) * src_p + di1  * bs + c ] * ri2 +
										src[ ( dk  * src_h + dj2 ) * src_p + di2  * bs + c ] * ri3 ) * rj3 ) * rk1 +
	
									( ( src[ ( dk1 * src_h + dj0 ) * src_p + di0  * bs + c ] * ri0 +
										src[ ( dk1 * src_h + dj0 ) * src_p + di   * bs + c ] * ri1 +
										src[ ( dk1 * src_h + dj0 ) * src_p + di1  * bs + c ] * ri2 +
										src[ ( dk1 * src_h + dj0 ) * src_p + di2  * bs + c ] * ri3 ) * rj0 +
									  ( src[ ( dk1 * src_h + dj  ) * src_p + di0  * bs + c ] * ri0 +
										src[ ( dk1 * src_h + dj  ) * src_p + di   * bs + c ] * ri1 +
										src[ ( dk1 * src_h + dj  ) * src_p + di1  * bs + c ] * ri2 +
										src[ ( dk1 * src_h + dj  ) * src_p + di2  * bs + c ] * ri3 ) * rj1 +
									  ( src[ ( dk1 * src_h + dj1 ) * src_p + di0  * bs + c ] * ri0 +
										src[ ( dk1 * src_h + dj1 ) * src_p + di   * bs + c ] * ri1 +
										src[ ( dk1 * src_h + dj1 ) * src_p + di1  * bs + c ] * ri2 +
										src[ ( dk1 * src_h + dj1 ) * src_p + di2  * bs + c ] * ri3 ) * rj2 +
									  ( src[ ( dk1 * src_h + dj2 ) * src_p + di0  * bs + c ] * ri0 +
										src[ ( dk1 * src_h + dj2 ) * src_p + di   * bs + c ] * ri1 +
										src[ ( dk1 * src_h + dj2 ) * src_p + di1  * bs + c ] * ri2 +
										src[ ( dk1 * src_h + dj2 ) * src_p + di2  * bs + c ] * ri3 ) * rj3 ) * rk2 +
	
									( ( src[ ( dk2 * src_h + dj0 ) * src_p + di0  * bs + c ] * ri0 +
										src[ ( dk2 * src_h + dj0 ) * src_p + di   * bs + c ] * ri1 +
										src[ ( dk2 * src_h + dj0 ) * src_p + di1  * bs + c ] * ri2 +
										src[ ( dk2 * src_h + dj0 ) * src_p + di2  * bs + c ] * ri3 ) * rj0 +
									  ( src[ ( dk2 * src_h + dj  ) * src_p + di0  * bs + c ] * ri0 +
										src[ ( dk2 * src_h + dj  ) * src_p + di   * bs + c ] * ri1 +
										src[ ( dk2 * src_h + dj  ) * src_p + di1  * bs + c ] * ri2 +
										src[ ( dk2 * src_h + dj  ) * src_p + di2  * bs + c ] * ri3 ) * rj1 +
									  ( src[ ( dk2 * src_h + dj1 ) * src_p + di0  * bs + c ] * ri0 +
										src[ ( dk2 * src_h + dj1 ) * src_p + di   * bs + c ] * ri1 +
										src[ ( dk2 * src_h + dj1 ) * src_p + di1  * bs + c ] * ri2 +
										src[ ( dk2 * src_h + dj1 ) * src_p + di2  * bs + c ] * ri3 ) * rj2 +
									  ( src[ ( dk2 * src_h + dj2 ) * src_p + di0  * bs + c ] * ri0 +
										src[ ( dk2 * src_h + dj2 ) * src_p + di   * bs + c ] * ri1 +
										src[ ( dk2 * src_h + dj2 ) * src_p + di1  * bs + c ] * ri2 +
										src[ ( dk2 * src_h + dj2 ) * src_p + di2  * bs + c ] * ri3 ) * rj3 ) * rk3 );
							}					
						}
	
						dst += diff;
					}
				}
			}
			else
			{
				// Compute pixel ratios on first use
				//
				// Ratios are computed based on an 11 bit fixed point computation.
				// Note that sums of the ratios should be 2^11, but due to rounding
				// errors, we need to enforce this - in this case, we simply add
				// the discrepancy to the highest ratio (being anal, I suppose n
				// should be distributed over the n highest values).
				//
				// TODO: Move this out of here - oil should construct any constant
				// tables of this nature on initialisation.

				static std::vector< std::vector< int > >ratio;

				if ( ratio.size( ) == 0 )
				{
					for ( int t = 0; t < 0x800; t ++ )
					{
						std::vector< int > v;
						v.push_back( R( -0x800 - t ) );
						v.push_back( R( -t ) );
						v.push_back( R( 0x800 - t ) );
						v.push_back( R( 0x1000 - t ) );
						int sum = v[0] + v[1] + v[2] + v[3];
						if ( sum != 0x800 )
						{
							int m = 0;
							for ( int i = 0; i < 4; i ++ )
								m = v[m] > v[i] ? m : i;
							v[m] += 0x800 - sum;
						}
						ratio.push_back( v );
					}
				}

				// Scale the image - this variant requires that the d and bs values are 1
				// hence it's optimised for planar formats.

				const int ddi = ( src_w << 11 ) / (std::max)( new_w, 1 );
				const int ddj = ( src_h << 11 ) / (std::max)( new_h, 1 );
	
				const int mj = ddj * new_h;
				const int mi = ddi * new_w;

				for( int j = 0; j < mj; j += ddj )
				{
					const int ij = j >> 11;
					const unsigned char *dj = src + ij * src_p;
					const unsigned char *dj0 = ij > 0 ? dj - src_p : src;
					const unsigned char *dj1 = ij < src_h - 1 ? dj + src_p : dj;
					const unsigned char *dj2 = ij < src_h - 2 ? dj + 2 * src_p : dj;

					const std::vector< int > &rj = ratio[ j & 0x7ff ];

					for( int i = 0; i < mi; i += ddi )
					{
						int di = i >> 11;
						int di0 = di > 0 ? di - 1 : 0;
						int di1 = di < src_w - 1 ? di + 1 : src_w - 1;
						int di2 = di < src_w - 2 ? di + 2 : src_w - 1;

						const std::vector< int > &ri = ratio[ i & 0x7ff ];

						di = std::min<int>( std::max<int>( di, 0 ), src_w - 1 );
						di0 = std::min<int>( std::max<int>( di0, 0 ), src_w - 1 );
						di1 = std::min<int>( std::max<int>( di1, 0 ), src_w - 1 );
						di2 = std::min<int>( std::max<int>( di2, 0 ), src_w - 1 );

						*dst++ = ( unsigned char ) ( (
							  ( dj0[ di0 ] * ri[0] +
								dj0[ di  ] * ri[1] +
								dj0[ di1 ] * ri[2] +
								dj0[ di2 ] * ri[3] ) * rj[0] +
							  ( dj[ di0 ] * ri[0] +
								dj[ di  ] * ri[1] +
								dj[ di1 ] * ri[2] +
								dj[ di2 ] * ri[3] ) * rj[1] +
							  ( dj1[ di0 ] * ri[0] +
								dj1[ di  ] * ri[1] +
								dj1[ di1 ] * ri[2] +
								dj1[ di2 ] * ri[3] ) * rj[2] +
							  ( dj2[ di0 ] * ri[0] +
								dj2[ di  ] * ri[1] +
								dj2[ di1 ] * ri[2] +
								dj2[ di2 ] * ri[3] ) * rj[3] ) >> 22 );
					}

					dst += diff;
				}
			}
		}
	}
}

image_type_ptr rescale_image( const image_type_ptr& im, int new_w, int new_h, int new_d, int bs, rescale_filter filter )
{
	image_type_ptr new_im = allocate( im->pf( ), new_w, new_h );
	if( !new_im )
		return im;

	for ( int p = 0; p < im->plane_count( ); p ++ )
	{
		rescale_plane( new_im, im, new_d, bs, filter, p );
	}
	
	return new_im;
}

image_type_ptr rescale( const image_type_ptr &im, int new_w, int new_h, int new_d, rescale_filter filter )
{
	if( im->width( ) == new_w && im->height( ) == new_h && im->depth( ) == new_d )
		return im;
	
	// Goncalo - we need some enums for the pixel formats really quick!
	if( im->pf( ) == L"l8" )
		return rescale_image( im, new_w, new_h, new_d, 1, filter );
	else if( im->pf( ) == L"l8a8p" )
		return rescale_image( im, new_w, new_h, new_d, 2, filter );
	else if( ( im->pf( ) == L"r8g8b8" ) || ( im->pf( ) == L"b8g8r8" ) || ( im->pf( ) == L"yuv444" ) )
		return rescale_image( im, new_w, new_h, new_d, 3, filter );
	else if( ( im->pf( ) == L"r8g8b8a8" ) || ( im->pf( ) == L"b8g8r8a8" ) )
		return rescale_image( im, new_w, new_h, new_d, 4, filter );
	else if( is_yuv_planar( im ) )
		return rescale_image( im, new_w, new_h, new_d, 1, filter );
	
	return im;
}

void histogram( const image_type_ptr& im, int size, float mask[ ], histogram_type& hist )
{	
	int w = im->width( );
	int h = im->height( );
	int d = im->depth( );
	int p = im->pitch( );

	// luminance or channels - luminance as a name is just a simplification.
	std::vector<float> luminance( w * h * d, 0.0f );
	typedef std::vector<float>::pointer pointer;

	hist.clear( );
	hist.resize( size, 0 );

	const float one_over_256 = 1.0f / 256.0f;

	image_type_ptr new_im = convert( im, L"r8g8b8a8" );
	image_type::const_pointer src = new_im->data( );

	for( int k = 0; k < d; ++k )
	{
		for( int j = 0; j < h; ++j )
		{
			for( int i = 0; i < w; ++i )
			{
				int src_idx = ( k * h + j  ) * p + i * 4;
				int dst_idx = ( k * h + j  ) * w + i;
				luminance[ dst_idx ] =	src[ src_idx + 0 ] * mask[ 0 ] * one_over_256
									  + src[ src_idx + 1 ] * mask[ 1 ] * one_over_256
									  + src[ src_idx + 2 ] * mask[ 2 ] * one_over_256
									  + src[ src_idx + 3 ] * mask[ 3 ] * one_over_256;
				
				if( luminance[ dst_idx ] < 0.0f )
					luminance[ dst_idx ] = 0.0f;
				else if( luminance[ dst_idx ] > 1.0f )
					luminance[ dst_idx ] = 1.0f;
						
				hist[ static_cast<unsigned int>( size * luminance[ dst_idx ] ) ]++;
			}
		}
	}
}

void histogram( const image_type_ptr& im, int size, histogram_filter filter, histogram_type& hist )
{
	float mask[ 4 ];

	if( filter == LUMINANCE )
	{
		mask[ 0 ] = 0.299f;
		mask[ 1 ] = 0.587f;
		mask[ 2 ] = 0.114f;
		mask[ 3 ] = 0.0f;
	}
	else if( filter == RED )
	{
		mask[ 0 ] = 1.0f;
		mask[ 1 ] = 0.0f;
		mask[ 2 ] = 0.0f;
		mask[ 3 ] = 0.0f;
	}
	else if( filter == GREEN )
	{
		mask[ 0 ] = 0.0f;
		mask[ 1 ] = 1.0f;
		mask[ 2 ] = 0.0f;
		mask[ 3 ] = 0.0f;
	}
	else if( filter == BLUE )
	{
		mask[ 0 ] = 0.0f;
		mask[ 1 ] = 0.0f;
		mask[ 2 ] = 1.0f;
		mask[ 3 ] = 0.0f;
	}
	else if( filter == ALPHA )
	{
		mask[ 0 ] = 0.0f;
		mask[ 1 ] = 0.0f;
		mask[ 2 ] = 0.0f;
		mask[ 3 ] = 1.0f;
	}
	
	histogram( im, size, mask, hist );
}

image_type_ptr project( const image_type_ptr& im, const opl::string& channel )
{
	// Goncalo: project to a single (for now) channel. This function should be generic enough to accept
	// any of the supported traits. Just do RGBA (and swizzling equivalents) for now - most pertinent 
	// for the player.
	// It also assumes BGRA as input - again most pertinent for the player.
	
	unsigned char mask[ 3 ] = { 255, 255, 255 };
	
	if( channel == "red" )
		mask[ 0 ] = mask[ 1 ] = 0;
	else if( channel == "green" )
		mask[ 0 ] = mask[ 2 ] = 0;
	else if( channel == "blue" )
		mask[ 1 ] = mask[ 2 ] = 0;

	image_type_ptr new_im = allocate( im->pf( ), im->width( ), im->height( ) );

	image_type::const_pointer src = im->data( );
	image_type::pointer		  dst = new_im->data( );
	
	int diff = new_im->pitch( ) - new_im->linesize( );

	int w = im->width( );
	int h = im->height( );
	int d = im->depth( );

	if( channel == "red" || channel == "green" || channel == "blue" )
	{
		for( int k = 0; k < d; ++k )
		{
			for( int j = 0; j < h; ++j )
			{
				for( int i = 0; i < w; ++i )
				{
					*dst++ = *src++ & mask[ 0 ];
					*dst++ = *src++ & mask[ 1 ];
					*dst++ = *src++ & mask[ 2 ];
					*dst++ = *src++;
				}
				
				src += diff;
				dst += diff;
			}
		}
		
		return new_im;
	}
	else if( channel == "alpha" )
	{
		for( int k = 0; k < d; ++k )
		{
			for( int j = 0; j < h; ++j )
			{
				for( int i = 0; i < w; ++i )
				{
					*dst++ = *( src + 3 );
					*dst++ = *( src + 3 );
					*dst++ = *( src + 3 );
					*dst++ = 255;
					
					src += 4;
				}
				
				src += diff;
				dst += diff;
			}
		}

		return new_im;
	}

	return im;
}

// For use with the tonemapping functions only! Generalisation will be for later.
void min_and_max( const image_type_ptr& im, float& min_, float& max_ )
{
	int w = im->width( );
	int h = im->height( );
	int d = im->depth( );

	min_ = (std::numeric_limits<float>::max)( );
	max_ = (std::numeric_limits<float>::min)( );

	if( im->pf( ) == L"r32g32b32f" )
	{
		const float* src = reinterpret_cast<const float*>( im->data( ) );

		for( int k = 0; k < d; ++k )
		{
			for( int j = 0; j < h; ++j )
			{
				for( int i = 0; i < w; ++i )
				{
					float r = *src++;
					float g = *src++;
					float b = *src++;
					
					min_ = (std::min)( min_, (std::min)( r, (std::min)( g, b ) ) );
					max_ = (std::max)( max_, (std::max)( r, (std::max)( g, b ) ) );
				}
				
				src += im->pitch( ) - im->linesize( );
			}
		}
	}
}

// CAVEAT EMPTOR: assumes unsigned shorts so it will only work with
// 10-, 12-, and 16-bit images (oil specific).
// For HDR it will have to be generalised (!)
void swab( image_type_ptr im, int p )
{
	size_type width = im->linesize( p );
	size_type height = im->height( p );

	const_pointer src = im->data( p );
	size_type src_pitch = im->pitch( p );

	unsigned short* dst = reinterpret_cast<unsigned short*>( im->data( p ) );
	size_type dst_pitch = im->pitch( p );

	const unsigned short* sptr = reinterpret_cast<const unsigned short*>( src );
	unsigned short* dptr = dst;

	size_type orig_width = width;

	while( height-- )
	{
		while( width-- )
		{
			unsigned short t1 = *src++;
			unsigned short t2 = *src++;
				
			*dst++ = ( t1 << 8 ) | t2;
		}
			
		dst = dptr += dst_pitch;
		sptr += src_pitch;
		src = reinterpret_cast<const_pointer>( sptr );
		width = orig_width;
	}
}

void swab( image_type_ptr im )
{
	for( int p = 0; p < im->plane_count( ); ++p )
		swab( im, p );
}

image_type_ptr convert_log_image_to_linear_b8g8r8a8( image_type_ptr im, int refblack, int refwhite, int softclip, float gamma, float density_range )
{
	image_type_ptr dst_img = allocate( im, L"b8g8r8a8" );

	std::vector<unsigned short> lut;
	log_to_linear_lut( lut, refblack, refwhite, softclip, gamma, 0.6f, density_range );

	size_type width		 = im->width( );
	size_type height	 = im->height( );
	size_type orig_width = width;

	const_pointer src = im->data( );
	size_type src_pitch = im->pitch( );

	pointer dst = dst_img->data( );
	size_type dst_pitch = dst_img->pitch( );
				
	const_pointer sptr = src;
	pointer dptr = dst;
		
	while( height-- )
	{
		while( width-- )
		{
			*dst++ = static_cast<unsigned char>( lut[ static_cast<unsigned short>( *src++ ) << 2 ] >> 2 );
			*dst++ = static_cast<unsigned char>( lut[ static_cast<unsigned short>( *src++ ) << 2 ] >> 2 );
			*dst++ = static_cast<unsigned char>( lut[ static_cast<unsigned short>( *src++ ) << 2 ] >> 2 );
			*dst++ = *src++;
		}

		dst = dptr += dst_pitch;
		src = sptr += src_pitch;
		width = orig_width;
	}
	
	return dst_img;
}

// NOTE: This is a big hack. Basically we convert all log data to b8g8r8a8 (as it was linear) and then apply the 10-bit lut to this data.
// NOTE: Generalisation is left as an exercise to the reader.
image_type_ptr convert_log_to_linear( image_type_ptr im, int refblack, int refwhite, int softclip, float gamma, float density_range )
{
	image_type_ptr dst_img;
	
	if( im->pf( ) == L"b8g8r8a8" )
		dst_img = convert_log_image_to_linear_b8g8r8a8( im, refblack, refwhite, softclip, gamma, density_range );
	
	return dst_img;
}

image_type_ptr normalise( image_type_ptr im, float output_range )
{
	image_type_ptr dst_img;
	
	if( im->pf( ) == L"r32g32b32f" )
	{
		dst_img = allocate( im, im->pf( ) );
		
		size_type width	 = im->width( );
		size_type height = im->height( );

		const float* src = ( const float* ) im->data( );
		size_type src_pitch = im->pitch( );

		float* dst = ( float* ) dst_img->data( );
		size_type dst_pitch = dst_img->pitch( );
				
		const float* sptr = src;
		float* dptr = ( float* ) dst;

		float minval = (std::numeric_limits<float>::max)( );
		float maxval = (std::numeric_limits<float>::min)( );
		
		// calculate the maximum and minimum luminance values.
		for( size_type i = 0; i < height; ++i )
		{
			for( size_type j = 0; j < width; ++j )
			{
				float r = *src++;
				float g = *src++;
				float b = *src++;
			
				float value = r * 0.2125f + g * 0.7154f + b * 0.0721f;
				
				if( value < minval )
					minval = value;
				if( value > maxval )
					maxval = value;
			}

			src = sptr += src_pitch;
		}
		
		// range is the difference the maximum and minimum luminance values.
		float range = maxval - minval;
		if( range < 1e-6f )
			return dst_img;
			
		float one_over_range = 1.0f / range;

		// reset the src image pointer.
		src  = ( const float* ) im->data( );
		sptr = src;
		
		// normalise the image to output_range.
		for( size_type i = 0; i < height; ++i )
		{
			for( size_type j = 0; j < width; ++j )
			{
				float r = output_range * ( ( *src++ - minval ) * one_over_range );
				float g = output_range * ( ( *src++ - minval ) * one_over_range );
				float b = output_range * ( ( *src++ - minval ) * one_over_range );
				
				r < 0.0f ? *dst++ = 0.0f : r > output_range ? *dst++ = output_range : *dst++ = r;
				g < 0.0f ? *dst++ = 0.0f : g > output_range ? *dst++ = output_range : *dst++ = g;
				b < 0.0f ? *dst++ = 0.0f : b > output_range ? *dst++ = output_range : *dst++ = b;
			}

			dst = dptr += dst_pitch;
			src = sptr += src_pitch;
		}
	}
	
	return dst_img;
}

static image_type_ptr tm_scale( image_type_ptr im, float factor, float minval, float maxval )
{
	image_type_ptr dst_img = im;

	if( im->pf( ) == L"r32g32b32f" )
	{
		dst_img = il::allocate( im->pf( ), im->width( ), im->height( ) );

		size_type width		 = im->width( );
		size_type height	 = im->height( );
		size_type orig_width = width;

		const float* src = ( const float* ) im->data( );
		size_type src_pitch = im->pitch( );

		float* dst = ( float* ) dst_img->data( );
		size_type dst_pitch = dst_img->pitch( );

		const float* sptr = src;
		float* dptr = dst;

		while( height-- )
		{
			while( width-- )
			{
				float r = *src++ * factor;
				float g = *src++ * factor;
				float b = *src++ * factor;

				r = r < minval ? minval : r > maxval ? maxval : r;
				g = g < minval ? minval : g > maxval ? maxval : g;
				b = b < minval ? minval : b > maxval ? maxval : b;
				
				r -= minval;
				g -= minval;
				b -= minval;
				
				*dst++ = r;
				*dst++ = g;
				*dst++ = b;
			}

			dst = dptr += dst_pitch;
			src = sptr += src_pitch;
			width = orig_width;
		}
	}

	return dst_img;
}

image_type_ptr tm_linear( image_type_ptr im, float factor, float gamma_factor, float minval, float maxval )
{
	image_type_ptr dst = im;

	if( im->pf( ) == L"r32g32b32f" )
	{
		dst = tm_scale( dst, factor, minval, maxval );
		dst = normalise( dst, 1.0f );
		dst = gamma( dst, gamma_factor );
		dst = normalise( dst, 255.0f );
		dst = convert( dst, L"ldr8" );
	}

	return dst;
}

// NOTE: This function (and similar ones) need to be generalised!
image_type_ptr gamma( image_type_ptr im, float gamma )
{
	image_type_ptr dst_img = im;

	if( im->pf( ) == L"r32g32b32f" )
	{
		dst_img = il::allocate( im->pf( ), im->width( ), im->height( ) );

		size_type width		 = im->width( );
		size_type height	 = im->height( );
		size_type orig_width = width;

		const float* src = ( const float* ) im->data( );
		size_type src_pitch = im->pitch( );

		float* dst = ( float* ) dst_img->data( );
		size_type dst_pitch = dst_img->pitch( );

		const float* sptr = src;
		float* dptr = dst;

		const float one_over_gamma = 1.0f / gamma;

		while( height-- )
		{
			while( width-- )
			{
				*dst++ = opl::fast_powf( *src++, one_over_gamma );
				*dst++ = opl::fast_powf( *src++, one_over_gamma );
				*dst++ = opl::fast_powf( *src++, one_over_gamma );
			}

			dst = dptr += dst_pitch;
			src = sptr += src_pitch;
			width = orig_width;
		}
	}

	return dst_img;
}

namespace {
	// Query structure used
	struct il_query_traits : public opl::default_query_traits
	{
		il_query_traits( const opl::wstring& filename, const opl::wstring &type )
			: filename_( filename )
			, type_( type )
		{ }
			
		opl::wstring to_match( ) const
		{ return filename_; }

		opl::wstring libname( ) const
		{ return opl::wstring( L"openimagelib" ); }

		opl::wstring type( ) const
		{ return opl::wstring( type_ ); }

		const opl::wstring filename_;
		const opl::wstring type_;

	private:
		il_query_traits& operator=( const il_query_traits& );
	};

	static openimagelib_plugin_ptr get_plug( const opl::wstring &resource, const opl::wstring type )
	{
		typedef opl::discovery< il_query_traits > discovery;
		openimagelib_plugin_ptr result = openimagelib_plugin_ptr( );
		il_query_traits query( resource, type );
		discovery plugins( query );
		if ( plugins.size( ) == 0 ) return result;
		discovery::const_iterator i = plugins.begin( );
		// fprintf( stderr, "get_plug: using: %s\n", opl::to_string( i->name() ).c_str() );
		return boost::shared_dynamic_cast<openimagelib_plugin>( i->create_plugin( "" ) );
	}
}

image_type_ptr load_image( const opl::wstring &resource )
{
	openimagelib_plugin_ptr plug = get_plug( resource, L"" );
	if ( !plug )
		return image_type_ptr( );
	
	return plug->load( fs::path( opl::to_string( resource ).c_str(), fs::native ) );
}

bool store_image( const opl::wstring &resource, image_type_ptr image )
{
	openimagelib_plugin_ptr plug = get_plug( resource, L"" );
	if ( !plug )
	{
		//fprintf( stderr, "store_image: failed to find a plugin\n" );
		return false;
	}
	
	return plug->store( fs::path( opl::to_string( resource ).c_str(), fs::native ), image );	
}

} } }
