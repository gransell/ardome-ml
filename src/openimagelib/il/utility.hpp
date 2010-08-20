/* -*- tab-width: 4; indent-tabs-mode: t -*- */ 
// il - A image library representation.

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#ifndef UTILITY_INC_
#define UTILITY_INC_

#include <openimagelib/il/config.hpp>
#include <openpluginlib/pl/string.hpp>

#include <cassert>

namespace opl = olib::openpluginlib;

namespace olib { namespace openimagelib { namespace il {

inline image_type_ptr conform( image_type_ptr image, int flags )
{
	if ( image && !image->matching( flags ) )
		image = image_type_ptr( static_cast<image_type*>( image->clone( flags ) ) );
	return image;
}

inline void yuv444_to_rgb24( int &r, int &g, int &b, unsigned char y, unsigned char u, unsigned char v )
{
	r = ( ( 1192 * ( y - 16 ) + 1634 * ( v - 128 ) ) >> 10 );
	g = ( ( 1192 * ( y - 16 ) - 832 * ( v - 128 ) - 400 * ( u - 128 ) ) >> 10 );
	b = ( ( 1192 * ( y - 16 ) + 2066 * ( u - 128 ) ) >> 10 );
	r = r < 0 ? 0 : r > 255 ? 255 : r; 
	g = g < 0 ? 0 : g > 255 ? 255 : g;
	b = b < 0 ? 0 : b > 255 ? 255 : b;
}

inline void rgb24_to_yuv444( int &y, int &u, int &v, unsigned char r, unsigned char g, unsigned char b )
{
	y = ( ( 263 * r + 516 * g + 100 * b ) >> 10 ) + 16;
	u = ( ( -152 * r - 298 * g + 450 * b ) >> 10 ) + 128;
	v = ( ( 450 * r - 377 * g - 73 * b ) >> 10 ) + 128;
}

IL_DECLSPEC image_type_ptr convert( const image_type_ptr &image, const opl::wstring &pf, int flags = 0 );

IL_DECLSPEC image_type_ptr allocate( const opl::wstring &pf, int width, int height );
IL_DECLSPEC image_type_ptr allocate( const image_type_ptr &src_img, const opl::wstring &format );
IL_DECLSPEC image_type_ptr allocate( const image_type_ptr &src_img );

IL_DECLSPEC image_type_ptr field( const image_type_ptr &im, int field );
IL_DECLSPEC image_type_ptr deinterlace( const image_type_ptr &im );
IL_DECLSPEC image_type_ptr extract_alpha( const image_type_ptr &im );
IL_DECLSPEC image_type_ptr merge_alpha( const image_type_ptr &im, const image_type_ptr &alpha );

enum IL_DECLSPEC rescale_filter { POINT_SAMPLING, BILINEAR_SAMPLING, BICUBIC_SAMPLING };

IL_DECLSPEC image_type_ptr rescale( const image_type_ptr &im, int new_w, int new_h, int new_d = 1, rescale_filter filter = POINT_SAMPLING );

template<typename T, template<class, class> class structure, template<class> class storage>
typename image<T, structure, storage>::size_type calculate_mipmap_size( const boost::shared_ptr<image<T, structure, storage> >& im, typename image<T, structure, storage>::size_type mip )
{
	typedef typename image<T, structure, storage>::size_type size_type;

	size_type width	 = im->width( )	 >> mip;
	size_type height = im->height( ) >> mip;
	size_type depth	 = im->depth( )	 >> mip;

	if( width  == 0 ) width  = 1;
	if( height == 0 ) height = 1;
	if( depth  == 0 ) depth  = 1;

	return im->allocsize( width, height, depth );
}

template<typename T, template<class, class> class structure, template<class> class storage>
typename image<T, structure, storage>::const_pointer cubemap_face( const boost::shared_ptr<image<T, structure, storage> >& im, int F )
{
	typedef typename image<T, structure, storage>::const_pointer const_pointer;
	typedef typename image<T, structure, storage>::size_type	 size_type;

	assert( im->is_cubemap( ) && L"This image doesn't represent a cubemap." );
	assert( F >= 0 && F <= 5 && L"A cubemap only has six faces." );

	if( !im->is_cubemap( ) ) return static_cast<const_pointer>( 0 );
			
	size_type siz = 0;
			
	while( F-- > 0 )
		for( size_type i = 0; i < im->count( ); ++i ) siz += calculate_mipmap_size( im, i );

	return im->data( ) + siz;
}

typedef std::vector<unsigned int> histogram_type;

enum IL_DECLSPEC histogram_filter { LUMINANCE, RED, GREEN, BLUE, ALPHA }; // RED, GREEN, BLUE and ALPHA do relate to channels rather than actual pixel format being rgba.

IL_DECLSPEC void histogram( const image_type_ptr& im, int size, float mask[ ], histogram_type& hist );
IL_DECLSPEC void histogram( const image_type_ptr& im, int size, histogram_filter filter, histogram_type& hist );

IL_DECLSPEC image_type_ptr project( const image_type_ptr& im, const opl::string& channel );
IL_DECLSPEC void min_and_max( const image_type_ptr& im, float& min, float& max );
IL_DECLSPEC void swab( image_type_ptr im, int i );
IL_DECLSPEC void swab( image_type_ptr im );
IL_DECLSPEC image_type_ptr convert_log_to_linear( image_type_ptr im, int refblack = 95, int refwhite = 685, int softclip = 0, float gamma = 1.7f, float density_range = 2.048f );
IL_DECLSPEC image_type_ptr normalise( image_type_ptr im, float output_range );
IL_DECLSPEC image_type_ptr tm_linear( image_type_ptr im, float factor = 100.0f, float gamma = 2.0f, float minval = 0.0f, float maxval = 1e20 );
IL_DECLSPEC image_type_ptr gamma( image_type_ptr im, float gamma );

inline bool is_yuv_planar( const opl::wstring &pf )
{
	return pf.length( ) > 4 && pf.substr( 0, 3 ) == L"yuv" && pf.substr( pf.length( ) - 1, 1 ) == L"p";
}

inline bool is_yuv_planar( const image_type_ptr &img )
{
	return img ? is_yuv_planar( img->pf( ) ) : false;
}

IL_DECLSPEC image_type_ptr load_image( const opl::wstring &resource );
IL_DECLSPEC bool store_image( const opl::wstring &resource, image_type_ptr image );

} } }

#endif
