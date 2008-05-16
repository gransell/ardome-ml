
// QuickTime - An QuickTime plugin to il.

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#ifdef WIN32
#include <ImageCompression.h>
#include <QuickTimeComponents.h>
#include <QTML.h>
#else
#include <QuickTime/ImageCompression.h>
#include <QuickTime/QuickTimeComponents.h>
#endif

#include <boost/filesystem/fstream.hpp>

#include <openpluginlib/pl/utf8_utils.hpp>

#include <openimagelib/plugins/quicktime/quicktime_plugin.hpp>

namespace il = olib::openimagelib::il;
namespace fs = boost::filesystem;
namespace pl = olib::openpluginlib;

namespace olib { namespace openimagelib { namespace plugins { namespace QT {

namespace
{
	void destroy( il::image_type* im )
	{ delete im; }

	il::image_type_ptr quicktime_image_type_to_image_type( int width, int height )
	{
		typedef il::image<unsigned char, il::a8r8g8b8> a8r8g8b8_image_type;

		return il::image_type_ptr( new il::image_type( a8r8g8b8_image_type( width, height, 1 ) ), destroy );
	}
	
	il::image_type_ptr load_quicktime( const fs::path& path )
	{
		OSStatus err;
		
		FSSpec fsspec;
#ifdef WIN32
		err = NativePathNameToFSSpec( const_cast<char*>( path.native_file_string( ).c_str( ) ), &fsspec, kErrorIfFileNotFound );
#else
		FSRef ref;
		err = FSPathMakeRef( ( const UInt8* ) ( path.native_file_string( ) ).c_str( ), &ref, 0 );
		if( err != noErr )
			return il::image_type_ptr( );

		err = FSGetCatalogInfo( &ref, kFSCatInfoNone, NULL, NULL, &fsspec, NULL );
#endif
		if( err != noErr )
			return il::image_type_ptr( );
		
		GraphicsImportComponent gi_comp = 0;
		err = GetGraphicsImporterForFile( &fsspec, &gi_comp );
		if( err != noErr )
			return il::image_type_ptr( );
		
		Rect rect;
		err = GraphicsImportGetNaturalBounds( gi_comp, &rect );
		if( err != noErr )
			return il::image_type_ptr( );
			
		ImageDescriptionHandle image_desc;
		image_desc = ( ImageDescriptionHandle ) NewHandle( sizeof( ImageDescriptionHandle ) );
		
		HLock( ( Handle ) image_desc );
		
		err = GraphicsImportGetImageDescription( gi_comp, &image_desc );
		if( err != noErr )
			return il::image_type_ptr( );
		
		int width  = rect.right  - rect.left;
		int height = rect.bottom - rect.top;
		
		il::image_type_ptr im = quicktime_image_type_to_image_type( width, height );
		if( !im )
			return il::image_type_ptr( );
			
		GWorldPtr gworld;
		NewGWorldFromPtr( &gworld, k32ARGBPixelFormat, &rect, 0, 0, 0, ( Ptr ) im->data( ), im->pitch( ) );
		if( gworld == NULL )
		{
			CloseComponent( gi_comp );
			return il::image_type_ptr( );
		}
		
		GDHandle device;
		CGrafPtr port;
		GetGWorld( &port, &device );
		
		MatrixRecord matrix;
		SetIdentityMatrix( &matrix );
		err = GraphicsImportSetMatrix( gi_comp, &matrix );
		if( err == noErr )
			err = GraphicsImportSetGWorld( gi_comp, gworld, 0 );
		if( err == noErr )
			err = GraphicsImportSetQuality( gi_comp, codecLosslessQuality );
		if( ( err == noErr ) && GetGWorldPixMap( gworld ) && LockPixels( GetGWorldPixMap( gworld ) ) )
			GraphicsImportDraw( gi_comp );
		else
		{
			DisposeGWorld( gworld );
			CloseComponent( gi_comp );
			
			gworld = 0;
			
			return il::image_type_ptr( );
		}
		
		UnlockPixels( GetGWorldPixMap( gworld ) );
		CloseComponent( gi_comp );
		SetGWorld( port, device );
		DisposeGWorld( gworld );
		
		return il::convert( im, L"a8b8g8r8" );
	}
}

il::image_type_ptr QT_plugin::load( const fs::path& path )
{ return load_quicktime( path ); }

bool QT_plugin::store( const fs::path& /*path*/, const il::image_type_ptr& )
{ return false; }

} } } }
