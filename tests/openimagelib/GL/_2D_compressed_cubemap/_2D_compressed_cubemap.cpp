
// il - A image library representation.

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

// simple example to exercise the load features of il (cubemaps).

#include <cstdio>
#include <cstdlib>

#ifdef HAVE_CONFIG_H
#include <openlibraries_global_config.hpp>
#endif

#include <GL/glew.h>

#if defined ( WIN32 ) || defined ( HAVE_GL_GLUT_H )
#include <GL/glut.h>
#else
#include <GLUT/glut.h>
#endif

#include <openpluginlib/pl/openpluginlib.hpp>
#include <openpluginlib/pl/utf8_utils.hpp>

#include <openimagelib/il/openimagelib_plugin.hpp>

const int WIN_WIDTH   = 640;
const int WIN_HEIGHT  = 480;

namespace il  = olib::openimagelib::il;
namespace opl = olib::openpluginlib;

static GLuint texid_cube_pos_x;
static GLuint texid_cube_neg_x;
static GLuint texid_cube_pos_y;
static GLuint texid_cube_neg_y;
static GLuint texid_cube_pos_z;
static GLuint texid_cube_neg_z;

namespace
{
#ifdef WIN32
	const opl::string plugin_path( "./plugins" );
#else
	const opl::string plugin_path( OPENIMAGELIB_PLUGINS );
#endif

	struct query_traits
	{
		query_traits( const opl::wstring& filename )
			: filename_( filename )
		{ }
		
		opl::wstring libname( ) const
		{ return L"openimagelib"; }
		
		opl::wstring to_match( ) const
		{ return filename_; }

		opl::wstring type( ) const
		{ return L""; }
		
		int merit( ) const
		{ return 0; }
		
		opl::wstring filename_;
	};
}

static bool download_cubemap_texture( const boost::filesystem::path& path )
{
	typedef opl::discovery<query_traits> oil_discovery;
	
	oil_discovery plugins( query_traits( opl::to_wstring( path.native_file_string( ).c_str( ) ) ) );
	if( plugins.empty( ) ) return 0; 
	
	oil_discovery::const_iterator i = plugins.begin( );
	boost::shared_ptr<il::openimagelib_plugin> plug = boost::shared_static_cast<il::openimagelib_plugin>( i->create_plugin( "" ) );
	if( !plug ) return 0;
	
	ml::image_type_ptr image = plug->load( path );
	if( !image ) return 0;
	
	glActiveTexture( GL_TEXTURE0 );

	glGenTextures( 1, &texid_cube_pos_x );
	glBindTexture( GL_TEXTURE_2D, texid_cube_pos_x );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
	
	typedef il::image_type::size_type size_type;
	
	il::image_type::const_pointer texels;
	size_type width, height;
	
	texels	= cubemap_face( image, 0 );
	width	= image->width( );
	height	= image->height( );

	for( il::image_type::size_type i = 0; i < image->count( ); ++i )
	{
		size_type mipsize = calculate_mipmap_size( image, i );
		
		glCompressedTexImage2DARB( GL_TEXTURE_2D, i, GL_COMPRESSED_RGB_S3TC_DXT1_EXT, width, height, 0, mipsize, texels );

		texels += mipsize;
		
		width  /= 2;
		height /= 2;
	}

	glGenTextures( 1, &texid_cube_neg_x );
	glBindTexture( GL_TEXTURE_2D, texid_cube_neg_x );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );

	texels	= cubemap_face( image, 1 );
	width	= image->width( );
	height	= image->height( );

	for( size_type i = 0; i < image->count( ); ++i )
	{
		size_type mipsize = calculate_mipmap_size( image, i );
		
		glCompressedTexImage2DARB( GL_TEXTURE_2D, i, GL_COMPRESSED_RGB_S3TC_DXT1_EXT, width, height, 0, mipsize, texels );

		texels += mipsize;
		
		width  /= 2;
		height /= 2;
	}

	glGenTextures( 1, &texid_cube_pos_y );
	glBindTexture( GL_TEXTURE_2D, texid_cube_pos_y );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );

	texels	= cubemap_face( image, 2 );
	width	= image->width( );
	height	= image->height( );

	for( size_type i = 0; i < image->count( ); ++i )
	{
		size_type mipsize = calculate_mipmap_size( image, i );
		
		glCompressedTexImage2DARB( GL_TEXTURE_2D, i, GL_COMPRESSED_RGB_S3TC_DXT1_EXT, width, height, 0, mipsize, texels );

		texels += mipsize;
		
		width  /= 2;
		height /= 2;
	}

	glGenTextures( 1, &texid_cube_neg_y );
	glBindTexture( GL_TEXTURE_2D, texid_cube_neg_y );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );

	texels	= cubemap_face( image, 3 );
	width	= image->width( );
	height	= image->height( );

	for( size_type i = 0; i < image->count( ); ++i )
	{
		size_type mipsize = calculate_mipmap_size( image, i );
		
		glCompressedTexImage2DARB( GL_TEXTURE_2D, i, GL_COMPRESSED_RGB_S3TC_DXT1_EXT, width, height, 0, mipsize, texels );

		texels += mipsize;
		
		width  /= 2;
		height /= 2;
	}

	glGenTextures( 1, &texid_cube_pos_z );
	glBindTexture( GL_TEXTURE_2D, texid_cube_pos_z );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );

	texels	= cubemap_face( image, 4 );
	width	= image->width( );
	height	= image->height( );

	for( size_type i = 0; i < image->count( ); ++i )
	{
		size_type mipsize = calculate_mipmap_size( image, i );
		
		glCompressedTexImage2DARB( GL_TEXTURE_2D, i, GL_COMPRESSED_RGB_S3TC_DXT1_EXT, width, height, 0, mipsize, texels );

		texels += mipsize;

		width  /= 2;
		height /= 2;
	}

	glGenTextures( 1, &texid_cube_neg_z );
	glBindTexture( GL_TEXTURE_2D, texid_cube_neg_z );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );

	texels	= cubemap_face( image, 5 );
	width	= image->width( );
	height	= image->height( );

	for( size_type i = 0; i < image->count( ); ++i )
	{
		size_type mipsize = calculate_mipmap_size( image, i );
		
		glCompressedTexImage2DARB( GL_TEXTURE_2D, i, GL_COMPRESSED_RGB_S3TC_DXT1_EXT, width, height, 0, mipsize, texels );

		texels += mipsize;

		width  /= 2;
		height /= 2;
	}
	
	return true;
}

static void draw_quad( )
{
	glBegin( GL_QUADS );
		glMultiTexCoord2f( GL_TEXTURE0, 0.0f, 0.0f );
		glVertex3f( -1.0f, -1.0f, -1.0f );
		glMultiTexCoord2f( GL_TEXTURE0, 1.0f, 0.0f );
		glVertex3f(  1.0f, -1.0f, -1.0f );
		glMultiTexCoord2f( GL_TEXTURE0, 1.0f, 1.0f );
		glVertex3f(  1.0f,  1.0f, -1.0f );
		glMultiTexCoord2f( GL_TEXTURE0, 0.0f, 1.0f );
		glVertex3f( -1.0f,  1.0f, -1.0f );
	glEnd( );
}

static void display( void )
{
	glDisable( GL_DEPTH_TEST );
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	glEnable( GL_DEPTH_TEST );

	glEnable( GL_TEXTURE_2D );

	// Neg X.
	glBindTexture( GL_TEXTURE_2D, texid_cube_neg_x );
	glPushMatrix( );
	glTranslatef( 2.0f, 0.0f, 0.0f );
		draw_quad( );
	glPopMatrix( );

	// Neg Y.
	glBindTexture( GL_TEXTURE_2D, texid_cube_neg_y );
	glPushMatrix( );
		draw_quad( );
	glPopMatrix( );
	
	// Pos Z.
	glBindTexture( GL_TEXTURE_2D, texid_cube_pos_z );
	glPushMatrix( );
	glTranslatef( 0.0f, -2.0f, 0.0f );
		draw_quad( );
	glPopMatrix( );

	// Neg Z.
	glBindTexture( GL_TEXTURE_2D, texid_cube_neg_z );
	glPushMatrix( );
	glTranslatef( 0.0f, 2.0f, 0.0f );
		draw_quad( );
	glPopMatrix( );
	
	// Pos X.
	glBindTexture( GL_TEXTURE_2D, texid_cube_pos_x );
	glPushMatrix( );
	glTranslatef( -2.0f, 0.0f, 0.0f );
		draw_quad( );
	glPopMatrix( );

	// Pos Y.
	glBindTexture( GL_TEXTURE_2D, texid_cube_pos_y );
	glPushMatrix( );
	glTranslatef( -4.0f, 0.0f, 0.0f );
		draw_quad( );
	glPopMatrix( );
	
	glDisable( GL_TEXTURE_2D );

#ifndef NDEBUG
	GLenum error;
	while( ( error = glGetError( ) ) != GL_NO_ERROR )
		fprintf( stderr, "GLerror:%s\n", gluErrorString( error ) );
#endif
	glutSwapBuffers( );
}

static void idle( void )
{
	glutPostRedisplay( );
}

static void reshape( int w, int h )
{
	glViewport( 0, 0, w, h );
	
	glMatrixMode( GL_PROJECTION );
	glLoadIdentity( );
	gluPerspective( 45.0f, ( float ) w / h, 0.01f, 1000.0f );
	
	glMatrixMode( GL_MODELVIEW );
	glLoadIdentity( );
	gluLookAt( -1.0f, 0.0f, -10.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f );
}

static void keyboard( unsigned char key, int /*x*/, int /*y*/ )
{
	switch( key )
	{
		case 27:
			exit( 0 );
			break;
		
		default:
			break;
	}
}

int main( int argc, char* argv[ ] )
{
	glutInit( &argc, argv );
	glutInitWindowSize( WIN_WIDTH, WIN_HEIGHT );
	glutInitDisplayMode( GLUT_DOUBLE | GLUT_RGBA );
	glutCreateWindow( "GLTextureDraw" );
	glutDisplayFunc( display );
	glutIdleFunc( idle );
	glutReshapeFunc( reshape );
	glutKeyboardFunc( keyboard );

	glewInit( );
	
	opl::init( plugin_path );
	
	download_cubemap_texture( "test.dds" );

	glEnable( GL_TEXTURE_2D );

	glutMainLoop( );
}
