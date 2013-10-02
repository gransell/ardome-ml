// Ardendo libx264 Wrapper for AML
//
// (C) 2012-2013 VizRT
//
// The sole purpose of this plugin is to register x264 as an
// FFmpeg encoder. It has no inputs/filters/stores of its own.

#include <boost/cstdint.hpp>
#include <openmedialib/ml/openmedialib_plugin.hpp>

namespace ml = olib::openmedialib::ml;
namespace pl = olib::openpluginlib;

extern "C" {
#include <libavcodec/avcodec.h>

//These symbols are defined in the ffmpeg_x264_adapter shared library
#ifdef _WIN32
__declspec( dllimport ) AVCodec ff_libx264_encoder;
__declspec( dllimport ) AVCodec ff_libx264rgb_encoder;
#else
extern AVCodec ff_libx264_encoder;
extern AVCodec ff_libx264rgb_encoder;
#endif
}

namespace amf {

static int init( void ) 
{
	static int initted = 0;

	if ( !initted )
	{
		// Load something from the avformat plugin to force it to load and 
		// initialise the libavformat state before we do anything else
		ml::filter_type_ptr force_load = ml::create_filter( L"resampler" );

		if ( force_load )
		{
			// Register the protocol and note that we're now initialised
			avcodec_register( &ff_libx264_encoder);
			avcodec_register( &ff_libx264rgb_encoder);

		  	initted = 1;
		}
		else
		{
			fprintf( stderr, "Failed to find the avformat plugin." );
		}
	}

	return initted;
}
}

//
// Access methods for openpluginlib
//

extern "C"
{
	ML_PLUGIN_DECLSPEC bool openplugin_init( void )
	{
		amf::init( );
		return true;
	}

	ML_PLUGIN_DECLSPEC bool openplugin_uninit( void )
	{
		return true;
	}
	
	ML_PLUGIN_DECLSPEC bool openplugin_create_plugin( const char*, pl::openplugin** plug )
	{
		*plug = 0;
		return true;
	}
	
	ML_PLUGIN_DECLSPEC void openplugin_destroy_plugin( pl::openplugin* plug )
	{ 
	}
}

