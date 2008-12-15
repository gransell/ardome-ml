
// 3D_lightmap - A lightmap generator plugin to il.

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#include <cstdlib>

#include <boost/tokenizer.hpp>

#include <openimagelib/plugins/3D_lightmap/3D_lightmap_plugin.hpp>

namespace fs = boost::filesystem;
namespace il = olib::openimagelib::il;

namespace olib { namespace openimagelib { namespace plugins { namespace lightmap {

typedef il::image<unsigned char, il::surface_format> image_type;

namespace
{
	void destroy( image_type* im )
	{ delete im; }

	bool discover_lightmap_size( const boost::filesystem::path& path, int& radius )
	{
		typedef boost::tokenizer< boost::char_separator<char> > tokenizer;
		
		tokenizer tok( path.leaf( ).begin( ), path.leaf( ).end( ) );
		
		radius = 1;
		
		tokenizer::const_iterator I = tok.begin( );
		if( I != tok.end( ) ) 
			radius = static_cast<int>( strtol( I->c_str( ), 0, 10 ) );

		return true;
	}
	
	il::image_type_ptr generator_image_type_to_image_type( int radius )
	{
		using namespace olib::openimagelib::il;

		typedef boost::shared_ptr<image_type>	image_type_ptr;
		typedef image<unsigned char, l8>		l8_image_type;

		return image_type_ptr( new image_type( l8_image_type( radius, radius, radius ) ), destroy );
	}

	il::image_type_ptr generate_lightmap( const boost::filesystem::path& path )
	{
		int radius;
		
		discover_lightmap_size( path, radius );
		il::image_type_ptr im = generator_image_type_to_image_type( radius );
		
		unsigned char* texels = im->data( );
		
		float radius_sq = static_cast<float>( radius * radius );
		for( int i = 0; i < radius; ++i )
		{
			for( int j = 0; j < radius; ++j )
			{
				for( int k = 0; k < radius; ++k )
				{
					float dist_sq = static_cast<float>( i * i + j * j + k * k );

					if( dist_sq < radius_sq )
					{
						float falloff = ( radius_sq - dist_sq ) / radius_sq;
						falloff *= falloff;
						
						texels[ i * radius * radius + j * radius + k ] = static_cast<unsigned char>( 255.0f * falloff );
					}
					else
					{
						texels[ i * radius * radius + j * radius + k ] = 0;
					}
				}
			}
		}
	
		return im;
	}
}

il::image_type_ptr lightmap3D_plugin::load( const fs::path& path )
{ return il::image_type_ptr( generate_lightmap( path ) ); }

bool lightmap3D_plugin::store( const fs::path& path, const il::image_type_ptr& )
{ return false; }

} } } }
