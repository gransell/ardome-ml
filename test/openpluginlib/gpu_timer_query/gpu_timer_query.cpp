
// openpluginlib - A plugin interface to openlibraries.

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include <iostream>

#ifdef HAVE_CONFIG_H
#include <openlibraries_global_config.hpp>
#endif

#include <GL/glew.h>

#if defined ( WIN32 ) || defined ( HAVE_GL_GLUT_H )
#include <GL/glut.h>
#else
#include <GLUT/glut.h>
#endif

#include <openpluginlib/pl/timer.hpp>

int WIN_WIDTH  = 720;
int WIN_HEIGHT = 576;

namespace opl = olib::openpluginlib;

#ifdef GL_EXT_timer_query

namespace
{
	typedef opl::timer<opl::gpu_>::value_type value_type;
	opl::timer<opl::gpu_> r;
}

#endif

static void display( void )
{
#ifdef GL_EXT_timer_query
	r.start( );

	glDisable( GL_DEPTH_TEST );
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	glEnable( GL_DEPTH_TEST );

	glViewport( 0, 0, WIN_WIDTH, WIN_HEIGHT );

	glMatrixMode( GL_PROJECTION );
	glPushMatrix( );
	glLoadIdentity( );
	gluOrtho2D( 0.0f, WIN_WIDTH, 0.0f, WIN_HEIGHT );

	glMatrixMode( GL_MODELVIEW );
	glPushMatrix( );
	glLoadIdentity( );

	glBegin( GL_QUADS );
		glVertex2f( 0.0f, 0.0f );
		glVertex2f( WIN_WIDTH,  0.0f );
		glVertex2f( WIN_WIDTH, WIN_HEIGHT );
		glVertex2f( 0.0f, WIN_HEIGHT );
	glEnd( );

	glMatrixMode( GL_PROJECTION );
	glPopMatrix( );

	glMatrixMode( GL_MODELVIEW );
	glPopMatrix( );

	glFlush( );

	r.stop( );
	value_type elapsed = r.elapsed( );
	std::cout << "gpu: elapsed time is " << elapsed.tv_sec << " seconds and " << elapsed.tv_usec << " microseconds.\n";

#ifndef NDEBUG
	GLenum error;
	while( ( error = glGetError( ) ) != GL_NO_ERROR )
		fprintf( stderr, "GLerror:%s\n", gluErrorString( error ) );
#endif

	glutSwapBuffers( );
	
#endif
}

static void idle( void )
{
	glutPostRedisplay( );
}

static void reshape( int w, int h )
{
	glViewport( 0, 0, w, h );

	WIN_WIDTH  = w;
	WIN_HEIGHT = h;
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
	glutCreateWindow( "GPU Timer Query" );
	glutDisplayFunc( display );
	glutIdleFunc( idle );
	glutReshapeFunc( reshape );
	glutKeyboardFunc( keyboard );

	glewInit( );

#ifdef GL_EXT_timer_query
	if( r.reset( ) )
	{
		std::cout << "gpu: ticks per microsecond " << r.ticks( ) << ".\n";

		glutMainLoop( );
	}
	else
#endif
	{
		std::cout << "gpu: no timer available.\n";
	}
}
