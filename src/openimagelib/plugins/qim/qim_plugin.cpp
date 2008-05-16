
// QIM - A QIM plugin to il.

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#include <cstdlib>
#include <memory>
#include <vector>
#include <string>

#include <qimage.h>

#include <openimagelib/plugins/qim/qim_plugin.hpp>

namespace il = olib::openimagelib::il;
namespace fs = boost::filesystem;

namespace olib { namespace openimagelib { namespace plugins { namespace QIM {

typedef il::image<unsigned char, il::surface_format> image_type;

namespace
{
	void destroy( image_type* im )
	{ delete im; }
				
	il::image_type_ptr load_qim( const fs::path& path )
	{
		typedef il::image< unsigned char, il::b8g8r8a8 > b8g8r8a8_image_type;
		QImage qimage( QString( path.native_directory_string( ).c_str( ) ) );
		int w = qimage.width( );
		int h = qimage.height( );
		uchar *bits = qimage.bits( );

		if ( !qimage.isNull( ) && qimage.depth( ) == 32 )
		{
			// Optimal memcpy when applicable
			il::image_type_ptr image( new il::image_type( b8g8r8a8_image_type( w, h, 1 ) ) );

			w *= 4;
			int stride = image->pitch( );
			uchar *ptr = image->data( );
			while ( h -- )
			{
				memcpy( ptr, bits, w );
				ptr += stride;
				bits += w;
			}

			return image;
		}
		else if ( !qimage.isNull( ) )
		{
			// Suboptimal fallback into other colour formats supported by QImage
			il::image_type_ptr image( new il::image_type( b8g8r8a8_image_type( w, h, 1 ) ) );

			uchar *ptr = image->data( );
			int stride = w * 4 - image->pitch( );
			QRgb rgb;
			int x, y;

			for( y = 0; y < h; y ++ )
			{
				for( x = 0; x < w; x ++ )
				{
					rgb = qimage.pixel( x, y );
					*ptr ++ = rgb & 0xff;
					*ptr ++ = ( rgb >> 8 ) & 0xff;
					*ptr ++ = ( rgb >> 16 ) & 0xff;
					*ptr ++ = ( rgb >> 24 ) & 0xff;
				}
				ptr += stride;
			}

			return image;
		}
		else
		{
			if ( qimage.isNull( ) )
				fprintf( stderr, "qimage is null\n" );
			else
				fprintf( stderr, "qimage has a depth of %d\n", qimage.depth( ) );
		}

		return il::image_type_ptr( );
	}
}

il::image_type_ptr QIM_plugin::load( const fs::path& path )
{ return il::image_type_ptr( load_qim( path ) ); }

bool QIM_plugin::store( const fs::path& /*path*/, const il::image_type_ptr& /*image*/ )
{ return false; }

} } } }
