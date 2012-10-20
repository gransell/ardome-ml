
// GDI+ - A GDI+ plugin to il.

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#ifdef WIN32
#include <windows.h>
#include <gdiplus.h>
#endif // WIN32

#include <cstdlib>
#include <vector>
#include <string>

#include <openimagelib/plugins/gdi+/gdi+_plugin.hpp>

namespace il = olib::openimagelib::il;
namespace fs = boost::filesystem;

namespace olib { namespace openimagelib { namespace plugins { namespace GDI {

typedef il::image<unsigned char, il::surface_format> image_type;
			
namespace
{
	void destroy( image_type* im )
	{ delete im; }
				
	std::wstring to_wstring( const std::string& str )
	{
		std::vector<wchar_t> ws;
		ws.resize( str.size( ) + 1 );

#if _MSC_VER >= 1400
		size_t size;
		mbstowcs_s( &size, &ws[ 0 ], ws.size( ), str.c_str( ), str.size( ) );
#else
		mbstowcs( &ws[ 0 ], str.c_str( ), str.size( ) );
#endif // _MSC_VER >= 0x1400
		
		return std::wstring( ws.begin( ), ws.end( ) );
	}

	struct mime_type_equals_png 
		: public std::unary_function<bool, Gdiplus::ImageCodecInfo>
	{
		bool operator( )( const Gdiplus::ImageCodecInfo& info )
		{
			return wcscmp( info.MimeType, L"image/png" ) == 0;
		}
	};

	bool has_png_encoder_installed( CLSID* clsid )
	{
		using namespace Gdiplus;

		UINT num, size;
		GetImageEncodersSize( &num, &size );

		ImageCodecInfo* encoders = reinterpret_cast<ImageCodecInfo*>( malloc( size ) );
		GetImageEncoders( num, size, encoders );

		bool found = false;

		typedef const ImageCodecInfo* const_iterator;
		const_iterator I = std::find_if( encoders, encoders + num, mime_type_equals_png( ) );
		if( I != encoders + num )
		{
			*clsid = I->Clsid;
			found = true;
		}
	
		free( encoders );
	
		return found;
	}

	il::image_type_ptr gdiplus_pixelformat_to_image_type( Gdiplus::PixelFormat pixelformat, int width, int height )
	{
		using namespace olib::openimagelib::il;

		typedef image<unsigned char, b8g8r8>	b8g8r8_image_type;
		typedef image<unsigned char, b8g8r8a8>	b8g8r8a8_image_type;
		typedef image<unsigned char, r8g8b8a8>	r8g8b8a8_image_type;

		switch( pixelformat )
		{
			case PixelFormat24bppRGB:
				return il::image_type_ptr( new image_type( b8g8r8_image_type( width, height, 1 ) ), destroy );
				
			case PixelFormat32bppARGB:
				return il::image_type_ptr( new image_type( b8g8r8a8_image_type( width, height, 1 ) ), destroy );

			case PixelFormat32bppPARGB:
			default:
				return il::image_type_ptr( static_cast<image_type*>( 0 ) );
		}
	}
				
	il::image_type_ptr load_image( const fs::wpath& path )
	{
		Gdiplus::Bitmap bitmap( ( path.native_directory_string( ) ).c_str( ) );

		Gdiplus::BitmapData bitmapData;
		Gdiplus::Rect rect( 0, 0, bitmap.GetWidth( ), bitmap.GetHeight( ) );

		bitmap.LockBits( &rect, Gdiplus::ImageLockModeRead, bitmap.GetPixelFormat( ), &bitmapData );

		il::image_type_ptr image = gdiplus_pixelformat_to_image_type( 
										bitmap.GetPixelFormat( ),
										bitmap.GetWidth( ),
										bitmap.GetHeight( ) );

		if( !image ) return il::image_type_ptr( );

		unsigned char* scan0	= static_cast<unsigned char*>( bitmapData.Scan0 );
		unsigned char* pixels	= image->data( );

		int stride = bitmapData.Stride;
		image->set_flipped( stride < 0 );

		size_t linesize = image->linesize( );
		for( image_type::size_type i = 0; i < image->height( ); ++i )
		{
			memcpy( pixels, scan0, linesize );

			scan0	+= stride;
			pixels	+= image->pitch( );
		}

		bitmap.UnlockBits( &bitmapData );

		return il::image_type_ptr( image );
	}
/*
	bool store_png( const fs::path& path, const image_type_ptr& image )
	{
		using namespace Gdiplus;

		EncoderParameters encoder_params;
		ULONG			  param_value;

		encoder_params.Count = 1;
		encoder_params.Parameter[ 0 ].Guid			 = EncoderSaveFlag;
		encoder_params.Parameter[ 0 ].Type			 = EncoderParameterValueTypeLong;
		encoder_params.Parameter[ 0 ].NumberOfValues = 1;
		encoder_params.Parameter[ 0 ].Value			 = &param_value;

		CLSID encoder_clsid;
		if( !has_png_encoder_installed( &encoder_clsid ) )
			return false;

		PixelFormat pf = image_pf_to_gdiplus_pixelformat( image->pf( ) );

		std::auto_ptr<Bitmap> im( new Bitmap( static_cast<INT>( image->width( ) ), static_cast<INT>( image->height( ) ), static_cast<INT>( image->pitch( ) ), pf, image->data( ) ) );

		Status status = im->Save( to_wstring( path.native_file_string( ) ).c_str( ), &encoder_clsid, &encoder_params );
		if( status != Ok ) return false;

		return true;
	}
*/
}

il::image_type_ptr GDI_plugin::load( const fs::wpath& path )
{ return load_image( path ); }

bool GDI_plugin::store( const fs::path& path, const il::image_type_ptr& )
{ return false; }

} } } }
