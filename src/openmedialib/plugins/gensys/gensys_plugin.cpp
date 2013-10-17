// gensys - Generic plugin functionality
//
// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.
//
// #input:colour:
//
// A frame generator which provides a fixed image.
//
// The colour: input is used to provide the first input to a filter:compositor
// and doing so ensures that all subsequently connected inputs are scaled and
// composited correctly. 
//
// For example, say we wish to have a graph which generates an output of 640x480
// square pixels at an NTSC frame rate, we would construct our graph as follows:
//
// colour: width=640 height=480 sar_num=1 sar_den=1 fps_num=30000 fps_den=1001
// file.mpg 
// filter:frame_rate fps_num=30000 fps_den=1001
// filter:compositor slots=2
//
// Note that this will provide a correct output, regardless of the resolution
// and frame rate of file.mpg.
//
// #input:pusher:
//
// A generic frame pusher input
//
// Doesn't do much - frames that are pushed in are retrieved on the fetch. 
// See filter:tee for an example of use.
//
// #filter:conform
//
// Ensures all frames which pass through have image and/or audio components
// as requested.
//
// #filter:crop
//
// Requires a single input - crops an image to the specified relative 
// coordinates.
//
// #filter:composite
//
// Requires 2 connected inputs - by default, it composites the second input
// on to the first - this is mainly seen as a mechanism to provide letter/pillar
// boxing type of functionality.
//
// It provides a number of modes - these are mainly for the purposes of simplifying
// the usage from the applications point of view. 
//
// When the application provides control over the geometry of the overlay, it can 
// specify the physical pixel geometry (which needs to be background aware) or it can
// specify relative position (which typically gives more flexibility in terms of the
// background resolution and aspect ratio).
//
// Coordinates of 0,0 specify the top left hand corner.
//
// #filter:correction
//
// Requires a single connected input.
//
// Carries out typical colour correction operations (hue, contrast, brightness
// and saturation).
//
// Works in the YCbCr colour space only.
//
// #filter:frame_rate
//
// Changes the frame rate to the requested value.
//
// #filter:clip
//
// Clip the connected input to the specified in/out points.
//
// The default values of in/out are 0 and -1 respectively. Negative values are
// treated as 'relative to the length of the input', so an out point of -1 is
// interpreted as the last frame in the input. Similarly, in=-100 out=-1 will
// provide a clip of the last 100 frames of the connected input.
//
// Note that if in > out, the clip will be played in reverse, so to play a
// clip backwards, it is sufficient to use:
//
// file.mpg filter:clip in=-1 out=0
//
// #filter:deinterlace
//
// Deinterlaces an image on demand.
//
// #filter:lerp
//
// Linear interpolation keyframing filter.
//
// #filter:visualise
//
// Provides a wave visualisation of the audio.
//
// #filter:playlist
//
// A simple n-ary filter - the number of possibly connected slots is
// defined by the 'slots' property. The number of frames reported by the filter
// is defined as the sum of the frames in all the connected slots. 
// 
// It is not this filters responsibility to ensure that all connected slots 
// have the same frame rate/sample information - that is left as an exercise
// for the filter graph builder to deal with.

// TODO: Split file up - source file per plugin needed.

#ifdef WIN32
// 'this' : used in base member initializer list
#pragma warning(disable:4355)

#define _USE_MATH_DEFINES
#endif // WIN32

#include <opencorelib/cl/core.hpp>
#include <opencorelib/cl/enforce_defines.hpp>

#include <openmedialib/ml/openmedialib_plugin.hpp>
#include <openmedialib/ml/filter_simple.hpp>
#include <openmedialib/ml/audio.hpp>
#include <opencorelib/cl/lru.hpp>
#include <opencorelib/cl/utilities.hpp>

#include <iostream>
#include <cstdlib>
#include <vector>
#include <map>
#include <string>
#include <cmath>
#include <limits>

#ifndef BOOST_THREAD_DYN_DLL
    #define BOOST_THREAD_DYN_DLL
#endif
#include <boost/thread/recursive_mutex.hpp>
#include <boost/thread/condition.hpp>
#include <boost/thread/thread.hpp>
#include <boost/cstdint.hpp>
#include <openpluginlib/pl/pcos/observer.hpp>

#ifdef _MSC_VER
#	pragma warning ( push )
#	pragma warning ( disable: 4251 )
#endif

namespace pl = olib::openpluginlib;
namespace cl = olib::opencorelib;
namespace pcos = olib::openpluginlib::pcos;
namespace ml = olib::openmedialib::ml;
namespace il = olib::openimagelib::il;

namespace olib { namespace openmedialib { namespace ml { 

// Utility functionality common to the plugin
inline void fill( il::image_type_ptr img, size_t plane, unsigned char val )
{
	unsigned char *ptr = img->data( plane );
	int width = img->width( plane );
	int height = img->height( plane );
	int diff = img->pitch( plane );
	if ( ptr ) // && val >= 16 && val <= 240 )
	{
		while( height -- )
		{
			memset( ptr, val, width );
			ptr += diff;
		}
	}
}

inline void fillRGB( il::image_type_ptr img, unsigned char r, unsigned char g, unsigned char b )
{
	unsigned char *ptr = img->data( );
	int width = img->width( );
	int height = img->height( );
	if ( ptr )
	{
		while( height -- )
		{
			int x = width;
			while ( x -- )
			{
				memset( ptr++, r, 1 );
				memset( ptr++, g, 1 );
				memset( ptr++, b, 1 );
			}
		}
	}
}

static pl::pcos::key key_is_background_( pcos::key::from_string( "is_background" ) );
static pl::pcos::key key_use_last_image_( pcos::key::from_string( "use_last_image" ) );

// A frame generator which provides a fixed colour
//
// NB: Decided to expose the user specified colour at a sample level rather 
// then a string (like 0xrrggbbaa) mainly to simplify the interface and to avoid
// parsing errors. 
//
// Since properties are primitive types only (int, double, string etc), a structure 
// isn't currently an option (though an array of primitives is arguably better for 
// the tuples required here).
//
// TODO: Provide additional properties to allow specification of the colour in 
// different colour spaces (RGBA, YIQ, YCbCr etc). 
//
// TODO: Provide support for additional image outputs (yuv planar formats supported)

class ML_PLUGIN_DECLSPEC colour_input : public input_type
{
	public:
		// Constructor and destructor
		colour_input( ) 
			: input_type( ) 
			, prop_colourspace_( pcos::key::from_string( "colourspace" ) )
			, prop_r_( pcos::key::from_string( "r" ) )
			, prop_g_( pcos::key::from_string( "g" ) )
			, prop_b_( pcos::key::from_string( "b" ) )
			, prop_a_( pcos::key::from_string( "a" ) )
			, prop_width_( pcos::key::from_string( "width" ) )
			, prop_height_( pcos::key::from_string( "height" ) )
			, prop_fps_num_( pcos::key::from_string( "fps_num" ) )
			, prop_fps_den_( pcos::key::from_string( "fps_den" ) )
			, prop_sar_num_( pcos::key::from_string( "sar_num" ) )
			, prop_sar_den_( pcos::key::from_string( "sar_den" ) )
			, prop_interlace_( pcos::key::from_string( "interlace" ) )
			, prop_out_( pcos::key::from_string( "out" ) )
			, prop_deferred_( pcos::key::from_string( "deferred" ) )
			, prop_background_( pcos::key::from_string( "background" ) )
		{
			// Default to a black PAL video
			properties( ).append( prop_colourspace_ = std::wstring( L"yuv420p" ) );
			properties( ).append( prop_r_ = 0x00 );
			properties( ).append( prop_g_ = 0x00 );
			properties( ).append( prop_b_ = 0x00 );
			properties( ).append( prop_a_ = 0xff );
			properties( ).append( prop_width_ = 720 );
			properties( ).append( prop_height_ = 576 );
			properties( ).append( prop_fps_num_ = 25 );
			properties( ).append( prop_fps_den_ = 1 );
			properties( ).append( prop_sar_num_ = 59 );
			properties( ).append( prop_sar_den_ = 54 );
			properties( ).append( prop_interlace_ = 0 );
			properties( ).append( prop_out_ = INT_MAX );
			properties( ).append( prop_deferred_ = 1 );
			properties( ).append( prop_background_ = 1 );
		}

		virtual ~colour_input( ) { }

		// Indicates if the input will enforce a packet decode
		virtual bool requires_image( ) const { return prop_deferred_.value< int >( ) == 0; }

		// Basic information
		virtual const std::wstring get_uri( ) const { return L"colour:"; }
		virtual const std::wstring get_mime_type( ) const { return L""; }

		// Audio/Visual
		virtual int get_frames( ) const { return prop_out_.value< int >( ); }
		virtual bool is_seekable( ) const { return true; }

		// Visual
		virtual void get_fps( int &num, int &den ) const 
		{
			num = prop_fps_num_.value< int >( );
			den = prop_fps_den_.value< int >( );
		}

		virtual double fps( ) const
		{
			int num, den;
			get_fps( num, den );
			return den != 0 ? double( num ) / double( den ) : 1;
		}

		virtual void get_sar( int &num, int &den ) const 
		{
			num = prop_sar_num_.value< int >( );
			den = prop_sar_den_.value< int >( );
		}

		virtual int get_width( ) const { return prop_width_.value< int >( ); }
		virtual int get_height( ) const { return prop_height_.value< int >( ); }

	protected:
		// Fetch method
		void do_fetch( frame_type_ptr &result )
		{
			// Return a frame
			result = frame_type_ptr( new frame_type( ) );

			// Obtain property values
			acquire_values( );

			il::image_type_ptr image;
			il::image_type_ptr alpha;

			// Create and populate the image
			if ( prop_deferred_.value< int >( ) == 0 )
			{
				image = il::allocate( prop_colourspace_.value< std::wstring >( ).c_str(), get_width( ), get_height( ) );
				if ( image )
				{
					image->set_writable( true );
					populate( image );
				}

				if ( prop_a_.value< int >( ) != 0xff )
				{
					alpha = il::allocate( L"l8", get_width( ), get_height( ) );
					if ( alpha )
					{
						alpha->set_writable( true );
						fill( alpha, 0, ( unsigned char )( prop_a_.value< int >( ) ) );
					}
				}

				image->set_field_order( il::field_order_flags( prop_interlace_.value< int >( ) ) );
			}
			else
			{
				if ( deferred_image_ == 0 || deferred_image_->width( ) != get_width( ) || deferred_image_->height( ) != get_height( ) )
				{
					deferred_image_ = il::allocate( prop_colourspace_.value< std::wstring >( ).c_str(), get_width( ), get_height( ) );
					if ( deferred_image_ )
					{
						deferred_image_->set_writable( false );
						populate( deferred_image_ );
					}

					if ( prop_a_.value< int >( ) != 0xff )
					{
						deferred_alpha_ = il::allocate( L"l8", get_width( ), get_height( ) );
						if ( deferred_alpha_ )
						{
							deferred_alpha_->set_writable( false );
							fill( deferred_alpha_, 0, ( unsigned char )( prop_a_.value< int >( ) ) );
						}
					}
				}

				image = deferred_image_;
				alpha = deferred_alpha_;

				image->set_field_order( il::field_order_flags( prop_interlace_.value< int >( ) ) );
			}

			if ( get_process_flags( ) & ml::process_image )
			{
				// Construct a frame and populate with basic information
				result->set_sar( prop_sar_num_.value< int >( ), prop_sar_den_.value< int >( ) );
				result->set_fps( prop_fps_num_.value< int >( ), prop_fps_den_.value< int >( ) );
				result->set_pts( get_position( ) * 1.0 / fps( ) );
				result->set_duration( 1.0 / fps( ) );
				result->set_position( get_position( ) );

				// Set the image
				result->set_image( image );
				result->set_alpha( alpha );

				// Identify image as a background
				pl::pcos::property prop( key_is_background_ );
				result->properties( ).append( prop = prop_background_.value< int >( ) );
			}
		}

		virtual bool reuse( ) { return false; }

		void populate( il::image_type_ptr image )
		{
			if ( il::is_yuv_planar( image ) )
			{
				int y, u, v;
				il::rgb24_to_yuv444( y, u, v, ( unsigned char )prop_r_.value< int >( ), ( unsigned char )prop_g_.value< int >( ), ( unsigned char )prop_b_.value< int >( ) );
				fill( image, 0, ( unsigned char )y );
				fill( image, 1, ( unsigned char )u );
				fill( image, 2, ( unsigned char )v );
			}
			else if ( image->pf( ).length( ) == 6 && image->pf( ).substr( 0, 6 ) == L"r8g8b8" )
			{
				fillRGB( image, ( unsigned char )prop_r_.value< int >( ), ( unsigned char )prop_g_.value< int >( ), ( unsigned char )prop_b_.value< int >( ) );
			}
		}

	private:
		pcos::property prop_colourspace_;
		pcos::property prop_r_;
		pcos::property prop_g_;
		pcos::property prop_b_;
		pcos::property prop_a_;
		pcos::property prop_width_;
		pcos::property prop_height_;
		pcos::property prop_fps_num_;
		pcos::property prop_fps_den_;
		pcos::property prop_sar_num_;
		pcos::property prop_sar_den_;
		pcos::property prop_interlace_;
		pcos::property prop_out_;
		pcos::property prop_deferred_;
		pcos::property prop_background_;
		il::image_type_ptr deferred_image_;
		il::image_type_ptr deferred_alpha_;
};

// A generic frame pusher input
//
// Doesn't do much - frames that are pushed in are retrieved on the fetch. Seeks
// are ignored.
//
// TODO: Impose a property driven max size.
//
// TODO: Introduce thread safety

class ML_PLUGIN_DECLSPEC pusher_input : public input_type
{
	public:
		// Constructor and destructor
		pusher_input( ) 
			: input_type( ) 
			, prop_length_( pcos::key::from_string( "length" ) )
			, last_frame_( )
		{ 
			properties( ).append( prop_length_ = INT_MAX );
		}

		virtual ~pusher_input( ) { }

		// Indicates if the input will enforce a packet decode
		virtual bool requires_image( ) const { return false; }

		// Basic information
		virtual const std::wstring get_uri( ) const { return L"pusher:"; }
		virtual const std::wstring get_mime_type( ) const { return L""; }

		// Audio/Visual
		virtual int get_frames( ) const 
		{ 
			if ( prop_length_.value< int >( ) == -1 )
				return int( queue_.size( ) ); 
			else
				return prop_length_.value< int >( );
		}

		virtual bool is_seekable( ) const { return false; }

		// Visual
		virtual void get_fps( int &num, int &den ) const { }
		virtual void get_sar( int &num, int &den ) const { }
		virtual int get_width( ) const { return 0; }
		virtual int get_height( ) const { return 0; }

		// Push method
		virtual bool push( frame_type_ptr frame )
		{
			if ( frame )
			{
				queue_.push_back( frame );

				ARENFORCE( queue_.size() == 1 || queue_[0]->get_position() != queue_[1]->get_position() );
			
				// Work around pushing the same frame multiple times to the store
				if( last_frame_ && frame->get_position( ) == last_frame_->get_position( ) )
				{
					ARENFORCE( queue_.size() == 1 );
					last_frame_ = ml::frame_type_ptr( );
				}
			}
			else
			{
				queue_.erase( queue_.begin( ), queue_.end( ) );
				last_frame_ = ml::frame_type_ptr( );
			}

			return true;
		}

	protected:
		// Fetch method
		void do_fetch( frame_type_ptr &result )
		{
			if( last_frame_ && last_frame_->get_position( ) == get_position( ) )
			{
				if ( queue_.size( ) == 0 || queue_[ 0 ]->get_position( ) != get_position( ) )
				{
					result = last_frame_->shallow( );
					return;
				}
			}
			
			// Obtain property values
			acquire_values( );

			// Acquire frame
			ARENFORCE_MSG( !queue_.empty(), "Attempting to fetch from depleted pusher input!" )( get_position() );
			result = *( queue_.begin( ) );
			queue_.pop_front( );
			
			last_frame_ = result->shallow( );
		}

	private:
		pcos::property prop_length_;
		std::deque< frame_type_ptr > queue_;
		ml::frame_type_ptr last_frame_;
};

// Chroma replacer.
//
// Replaces all chroma with the u and v values specified. All output is
// yuv planar only - if the input is not yuv planar, yuv420p is output.
//
// Default behaviour is greyscale.
//
// Properties:
//
//	u, v = chroma values [default: 128, 128]
//		Values should be in the range 16 to 240 - not change occurs when the
//		value is out of range.

class ML_PLUGIN_DECLSPEC chroma_filter : public filter_simple
{
	public:
		chroma_filter( )
			: filter_simple( )
			, prop_enable_( pcos::key::from_string( "enable" ) )
			, prop_u_( pcos::key::from_string( "u" ) )
			, prop_v_( pcos::key::from_string( "v" ) )
		{
			properties( ).append( prop_enable_ = 1 );
			properties( ).append( prop_u_ = 128 );
			properties( ).append( prop_v_ = 128 );
		}

		// Indicates if the input will enforce a packet decode
		virtual bool requires_image( ) const { return prop_enable_.value< int >( ) == 1; }

		virtual const std::wstring get_uri( ) const { return L"chroma"; }

	protected:
		void do_fetch( frame_type_ptr &result )
		{
			acquire_values( );

			result = fetch_from_slot( );

			if ( prop_enable_.value< int >( ) && result && result->get_image( ) )
			{
				if ( !is_yuv_planar( result ) )
					result = frame_convert( result, L"yuv420p" );
				il::image_type_ptr img = result->get_image( );
				img = il::conform( img, il::writable );
				if ( img )
				{
					fill( img, 1, ( unsigned char )prop_u_.value< int >( ) );
					fill( img, 2, ( unsigned char )prop_v_.value< int >( ) );
				}
				result->set_image( img );
			}
		}

	private:
		pcos::property prop_enable_;
		pcos::property prop_u_;
		pcos::property prop_v_;
};

// Conform filter
//
// Ensures all frames which pass through have image and/or audio components
// as requested.
//
// Properties:
//
//	image = 0, 1 [default] or 2
//		if 0: remove the image if present
//		if 1: if an image is not present, create a black image
//		if 2: do nothing
//
//	audio = 0, 1 [default] or 2
//		if 0: remove the audio if present
//		if 1: if an audio is not present, create silence at frame size
//		if 2: do nothing
//
//	NB: Currently, there's no control over the image or audio provided.
//	Downstream filters should be used to conform things further.
//

class ML_PLUGIN_DECLSPEC conform_filter : public filter_simple
{
	public:
		conform_filter( )
			: filter_simple( )
			, prop_image_( pcos::key::from_string( "image" ) )
			, prop_audio_( pcos::key::from_string( "audio" ) )
			, prop_frequency_( pcos::key::from_string( "frequency" ) )
			, prop_channels_( pcos::key::from_string( "channels" ) )
			, prop_pf_( pcos::key::from_string( "pf" ) )
			, prop_default_( pcos::key::from_string( "default" ) )
		{
			properties( ).append( prop_image_ = 1 );
			properties( ).append( prop_audio_ = 1 );
			properties( ).append( prop_frequency_ = 48000 );
			properties( ).append( prop_channels_ = 2 );
			properties( ).append( prop_pf_ = std::wstring( L"yuv420p" ) );
			properties( ).append( prop_default_ = std::wstring( L"" ) );
		}

		// Indicates if the input will enforce a packet decode
		virtual bool requires_image( ) const { return false; }

		virtual const std::wstring get_uri( ) const { return L"conform"; }

	protected:
		void do_fetch( frame_type_ptr &result )
		{
			result = fetch_from_slot( )->shallow( );

			if ( result )
			{
				if ( prop_image_.value< int >( ) == 1 )
				{
					if ( !result->has_image( ) )
					{
						default_image( result );
					}
				}
				else if ( prop_image_.value< int >( ) == 0 )
				{
					result->set_image( il::image_type_ptr( ) );
				}

				if ( prop_audio_.value< int >( ) == 1 )
				{
					if ( result->get_audio( ) == ml::audio_type_ptr( ) )
					{
						int frequency = prop_frequency_.value< int >( );
						int channels = prop_channels_.value< int >( );
						int samples = audio::samples_for_frame( get_position( ), frequency, result->get_fps_num( ), result->get_fps_den( ) );
						audio_type_ptr temp = audio::allocate( audio::pcm16_id, frequency, channels, samples, true );
						result->set_audio( temp );
					}
				}
				else if ( prop_audio_.value< int >( ) == 0 )
				{
					result->set_audio( audio_type_ptr( ) );
				}
			}
		}

		void default_image( frame_type_ptr &result )
		{
			if ( prop_default_.value< std::wstring >( ) != L"" )
			{
				std::wstring resource = prop_default_.value< std::wstring >( );

				// Detect changes in default property
				if ( resource != current_default_  )
				{
					input_default_ = input_type_ptr( );
					input_pusher_ = input_type_ptr( );
					current_default_ = resource;
				}

				// Create the input if we haven't done so before
				if ( input_default_ == 0 )
				{
					if ( resource.substr( 0, 7 ) == std::wstring( L"filter:" ) )
					{
						ml::filter_type_ptr filter = create_filter( resource.substr( 7 ) );
						if ( filter )
						{
							input_pusher_ = input_type_ptr( new pusher_input( ) );
							filter->connect( input_pusher_ );
							input_default_ = filter;
						}
						else
						{
							PL_LOG( pl::level::error, boost::format( "Unable to create requested filter: %s" ) % cl::str_util::to_string( resource ) );
						}
					}
					else
					{
						input_default_ = create_input( resource );
						if ( input_default_ == 0 || input_default_->get_frames( ) <= 0 )
						{
							PL_LOG( pl::level::error, boost::format( "Unable to create requested input: %s" ) % cl::str_util::to_string( resource ) );
							input_default_ = input_type_ptr( );
						}
					}
				}

				// Fetch a frame and apply image and sar to our result
				if ( input_default_ )
				{
					// Determine if we're in push mode to a filter, or fetching from an input
					if ( input_pusher_ )
					{
						// Push and fetch here
						result->set_image( il::image_type_ptr( ) );
						input_pusher_->push( result );
						input_default_->seek( get_position( ) );
						result = input_default_->fetch( );
					}
					else if ( input_default_->get_frames( ) > 0 )
					{
						// Allow animations to be used but allow them to be consistent (in case of downstream caching)
						if ( input_default_->get_frames( ) > 1 )
							input_default_->seek( get_position( ) % input_default_->get_frames( ) );

						// Fetch the frame
						frame_type_ptr frame = input_default_->fetch( );

						if ( frame && frame->has_image( ) )
						{
							// Set image and sar
							result->set_image( frame->get_image( ) );
							result->set_sar( frame->get_sar_num( ), frame->get_sar_den( ) );
						}
					}
				}

				// Force non background setting
				pl::pcos::property prop = result->properties( ).get_property_with_key( key_is_background_ );
				if ( prop.valid( ) && prop.value< int >( ) == 1 )
					prop = 0;
			}

			if ( !result->has_image( ) )
			{
				il::image_type_ptr image = il::allocate( prop_pf_.value< std::wstring >( ).c_str( ), 720, 576 );
				fill( image, 0, ( unsigned char )16 );
				fill( image, 1, ( unsigned char )128 );
				fill( image, 2, ( unsigned char )128 );
				result->set_image( image );
				result->set_sar( 59, 54 );
			}
		}

	private:
		pcos::property prop_image_;
		pcos::property prop_audio_;
		pcos::property prop_frequency_;
		pcos::property prop_channels_;
		pcos::property prop_pf_;
		pcos::property prop_default_;
		input_type_ptr input_default_;
		input_type_ptr input_pusher_;
		std::wstring current_default_;
};

// Crop filter
//
// Requires a single input - crops an image to the specified relative 
// coordinates.
//
// Note: that if you have multiple crop filters in sequence, the default 
// behaviour is to reduce the image relative to the currently reported 
// dimensions.
//
// Property usage:
//
//	absolute = 0 [default] or 1
//		crop the reported image dimensions or the full uncropped image
//
//	rx, ry, rw, rh = double, double, double, double (0,0,1,1 is default)
//		the visible area of the background is specified in the range of 0.0 to 1.0

class ML_PLUGIN_DECLSPEC crop_filter : public filter_simple
{
	public:
		crop_filter( )
			: filter_simple( )
			, prop_enable_( pcos::key::from_string( "enable" ) )
			, prop_absolute_( pcos::key::from_string( "absolute" ) )
			, prop_pixel_( pcos::key::from_string( "pixel" ) )
			, prop_rx_( pcos::key::from_string( "rx" ) )
			, prop_ry_( pcos::key::from_string( "ry" ) )
			, prop_rw_( pcos::key::from_string( "rw" ) )
			, prop_rh_( pcos::key::from_string( "rh" ) )
		{
			properties( ).append( prop_enable_ = 1 );
			properties( ).append( prop_absolute_ = 0 );
			properties( ).append( prop_pixel_ = 0 );
			properties( ).append( prop_rx_ = 0.0 );
			properties( ).append( prop_ry_ = 0.0 );
			properties( ).append( prop_rw_ = 1.0 );
			properties( ).append( prop_rh_ = 1.0 );
		}

		// Indicates if the input will enforce a packet decode
		virtual bool requires_image( ) const { return prop_enable_.value< int >( ) == 1; }

		virtual const std::wstring get_uri( ) const { return L"crop"; }

	protected:
		void do_fetch( frame_type_ptr &result )
		{
			acquire_values( );

			result = fetch_from_slot( );

			if ( prop_enable_.value< int >( ) && result && result->get_image( ) )
			{
				if ( !is_yuv_planar( result ) )
					result = frame_convert( result, L"yuv420p" );
				il::image_type_ptr img = result->get_image( );

				// Ensure the image component is writable
				img = il::conform( img, il::writable );
				result->set_image( img );

				if ( img )
				{
					if ( prop_absolute_.value< int >( ) == 1 )
						result = frame_crop_clear( result );

					int scale_w = img->width( );
					int scale_h = img->height( );

					if ( prop_pixel_.value< int >( ) )
					{
						scale_w = 1;
						scale_h = 1;
					}

					int px = int( scale_w * prop_rx_.value< double >( ) );
					int py = int( scale_h * prop_ry_.value< double >( ) );
					int pw = int( scale_w * prop_rw_.value< double >( ) );
					int ph = int( scale_h * prop_rh_.value< double >( ) );

					if ( px < 0 )
					{
						pw += px;
						px = 0;
					}

					if ( py < 0 )
					{
						ph += py;
						py = 0;
					}

					if ( ( px + pw ) > img->width( ) )
					{
						pw = img->width( ) - px;
					}

					if ( ( py + ph ) > img->height( ) )
					{
						ph = img->height( ) - py;
					}

					result = frame_crop( result, px, py, pw, ph );
					result->set_image( il::image_type_ptr( static_cast<il::image_type*>( result->get_image( )->clone( il::cropped ) ) ) );
				}
			}
		}

	private:
		pcos::property prop_enable_;
		pcos::property prop_absolute_;
		pcos::property prop_pixel_;
		pcos::property prop_rx_;
		pcos::property prop_ry_;
		pcos::property prop_rw_;
		pcos::property prop_rh_;
};

// Composite filter
//
// Requires 2 connected inputs - by default, it composites the second input
// on to the first - this is mainly seen as a mechanism to provide letter/pillar
// boxing type of functionality.
//
// It provides a number of modes - these are mainly for the purposes of simplifying
// the usage from the applications point of view. 
//
// When the application provides control over the geometry of the overlay, it can 
// specify the physical pixel geometry (which needs to be background aware) or it can
// specify relative position (which typically gives more flexibility in terms of the
// background resolution and aspect ratio).
//
// Coordinates of 0,0 specify the top left hand corner.
//
// Property usage:
//
//	rx, ry, rw, rh = double, double, double, double (0,0,1,1 is default)
//		the visible area of the background is specified in the range of 0.0 to 1.0
//
//	mix = 0.0 to 1.0 [default]
//		level of video mixing for the requested frame
//
//	mix_in, mix_mid, mix_out = double, double, double (1,1,1)
//		Proposed/unimplemented: 3 point audio mixing for the current frame 
//
//	mode = fill [default], distort, letter, pillar, smart, ardendo
//		fill ensures that the overlay is scaled to fit in the specied geometry 
//		whilst maintaining aspect ratio
//		distort ensures the overlay is scaled to fit the specified geometry
//		ignoring aspect ratio
//		letterbox scales the image to fit horizontally whilst maintaining
//		aspect ratio (cropping may occur)
//		pillarbox scales the image to fit vertically whilst maintaining
//		aspect ratio (cropping may occur)
//		smart scales and crops to ensure that the full destination resolution
//		is used
//		ardendo uses the same sizing as distort, but the rx and ry are measured
//		in pixels and give the relative displacement from a centered position
//		rather than from the top left corners of the images
//
//	halign = 0, 1 or 2 (for left, centre, right)
//	valign = 0, 1 or 2 (for left, centre, right)
//		Horizontal and veritical alignment
//
//	TODO: Provide support for non yuv planar formats.
//
//	TODO: Additional alpha compositing modes.

struct geometry { int x, y, w, h, cx, cy, cw, ch; };

class ML_PLUGIN_DECLSPEC composite_filter : public filter_type
{
	public:
		typedef frame_type_ptr ( *frame_rescale_type )( frame_type_ptr, int, int, il::rescale_filter );
		typedef il::image_type_ptr ( *image_rescale_type )( const il::image_type_ptr &, int, int, int, il::rescale_filter );

		composite_filter( )
			: filter_type( )
			, prop_enable_( pcos::key::from_string( "enable" ) )
			, prop_mode_( pcos::key::from_string( "mode" ) )
			, prop_swap_( pcos::key::from_string( "swap" ) )
			, prop_rx_( pcos::key::from_string( "rx" ) )
			, prop_ry_( pcos::key::from_string( "ry" ) )
			, prop_rw_( pcos::key::from_string( "rw" ) )
			, prop_rh_( pcos::key::from_string( "rh" ) )
			, prop_mix_( pcos::key::from_string( "mix" ) )
			, prop_interp_( pcos::key::from_string( "interp" ) )
			, prop_slot_( pcos::key::from_string( "slot" ) )
			, prop_frame_rescale_cb_( pcos::key::from_string( "frame_rescale_cb" ) ) //Deprecated
			, prop_image_rescale_cb_( pcos::key::from_string( "image_rescale_cb" ) ) //Deprecated
		{
			properties( ).append( prop_enable_ = 1 );
			properties( ).append( prop_mode_ = std::wstring( L"fill" ) );
			properties( ).append( prop_swap_ = 0 );
			properties( ).append( prop_rx_ = 0.0 );
			properties( ).append( prop_ry_ = 0.0 );
			properties( ).append( prop_rw_ = 1.0 );
			properties( ).append( prop_rh_ = 1.0 );
			properties( ).append( prop_mix_ = 1.0 );
			properties( ).append( prop_interp_ = std::wstring( L"bilinear" ) );
			properties( ).append( prop_slot_ = 0 );

			properties( ).append( prop_frame_rescale_cb_ = boost::uint64_t( 0 ) ); //Deprecated
			properties( ).append( prop_image_rescale_cb_ = boost::uint64_t( 0 ) ); //Deprecated
		}

		// Indicates if the input will enforce a packet decode
		virtual bool requires_image( ) const { return prop_enable_.value< int >( ) == 1; }

		virtual const std::wstring get_uri( ) const { return L"composite"; }

		virtual const size_t slot_count( ) const { return 2; }

		virtual int get_frames( ) const
		{
			input_type_ptr input = fetch_slot( prop_slot_.value< int >( ) );
			return input ? input->get_frames( ) : 0;
		}

	protected:
		void do_fetch( frame_type_ptr &result )
		{
			acquire_values( );

			frame_type_ptr overlay;

			if ( prop_enable_.value< int >( ) == 0 )
			{
				result = fetch_from_slot( prop_slot_.value< int >( ) );
			}
			else
			{
				result = fetch_from_slot( prop_swap_.value< int >( ) ? 1 : 0, false );
				overlay = fetch_from_slot( prop_swap_.value< int >( ) ? 0 : 1 );
			}

			if ( result && overlay )
			{
				il::image_type_ptr dst = result->get_image( );
				il::image_type_ptr src = overlay->get_image( );

				if ( dst && src )
				{
					struct geometry geom = calculate_full( result, overlay );
					result = composite( result, overlay, geom );

					// If we've composited on to a background, it's no longer a background...
					pl::pcos::property prop = result->properties( ).get_property_with_key( key_is_background_ );
					if ( prop.valid( ) && prop.value< int >( ) == 1 )
						prop = 0;
				}

				if ( result->get_audio( ) && overlay->get_audio( ) )
				{
					audio_type_ptr mix = audio::mixer( result->get_audio( ), overlay->get_audio( ) );
					result->set_audio( mix );
				}

				if ( !result->get_audio( ) )
				{
					result->set_audio( overlay->get_audio( ) );
				}
			}
			else if ( overlay )
			{
				// Incorrect, but better than nothing for now
				result = overlay;
			}

			if ( result )
				result->set_position( get_position( ) );
		}

		struct geometry calculate_full( frame_type_ptr dst, frame_type_ptr src )
		{
			struct geometry result;

			double src_sar_num = double( src->get_sar_num( ) ); 
			double src_sar_den = double( src->get_sar_den( ) );
			double dst_sar_num = double( dst->get_sar_num( ) );
			double dst_sar_den = double( dst->get_sar_den( ) );
			int src_w = src->get_image( )->width( );
			int src_h = src->get_image( )->height( );
			int dst_w = dst->get_image( )->width( );
			int dst_h = dst->get_image( )->height( );

			if ( src_sar_num == 0 ) src_sar_num = 1;
			if ( src_sar_den == 0 ) src_sar_den = 1;
			if ( dst_sar_num == 0 ) dst_sar_num = 1;
			if ( dst_sar_den == 0 ) dst_sar_den = 1;

			// Relative positioning
			result.x = int( dst_w * prop_rx_.value< double >( ) );
			result.y = int( dst_h * prop_ry_.value< double >( ) );
			result.w = int( dst_w * prop_rw_.value< double >( ) );
			result.h = int( dst_h * prop_rh_.value< double >( ) );

			// Specify the initial crop area
			result.cx = 0;
			result.cy = 0;
			result.cw = src_w;
			result.ch = src_h;

			// Target destination area
			dst_w = result.w;
			dst_h = result.h;

			// Letter and pillar box calculations
			int letter_h = int( 0.5 + ( result.w * src_h * src_sar_den * dst_sar_num ) / ( src_w * src_sar_num * dst_sar_den ) );
			int pillar_w = int( 0.5 + ( result.h * src_w * src_sar_num * dst_sar_den ) / ( src_h * src_sar_den * dst_sar_num ) );

			// Handle the requested mode
			if ( prop_mode_.value< std::wstring >( ) == L"fill" )
			{
				result.h = dst_h;
				result.w = pillar_w;

				if ( result.w > dst_w )
				{
					result.w = dst_w;
					result.h = letter_h;
				}
			}
			else if ( prop_mode_.value< std::wstring >( ) == L"smart" )
			{
				result.h = dst_h;
				result.w = pillar_w;

				if ( result.w < dst_w )
				{
					result.w = dst_w;
					result.h = letter_h;
				}
			}
			else if ( prop_mode_.value< std::wstring >( ).find( L"letter" ) == 0 )
			{
				result.w = dst_w;
				result.h = letter_h;
			}
			else if ( prop_mode_.value< std::wstring >( ).find( L"pillar" ) == 0 )
			{
				result.h = dst_h;
				result.w = pillar_w;
			}
			else if ( prop_mode_.value< std::wstring >( ).find( L"native" ) == 0 )
			{
				result.w = src_w;
				result.h = src_h;
			}
			else if ( prop_mode_.value< std::wstring >( ).find( L"corrected" ) == 0 )
			{
				result.w = int( 0.5 + ( src_w * src_sar_num * dst_sar_den ) / ( src_sar_den * dst_sar_num ) );
				result.h = src_h;
			}
			else if ( prop_mode_.value< std::wstring >( ).find( L"ardendo" ) == 0 )
			{
				result.x = (src_w - result.w ) / 2 + prop_rx_.value< double >( );
				result.y = (src_h - result.h ) / 2 + prop_ry_.value< double >( );
			}

			// Correct the cropping as required
			if ( dst_w >= result.w )
			{
				result.x += ( int( dst_w ) - result.w ) / 2;
			}
			else
			{
				double diff = double( result.w - dst_w ) / result.w;
				result.cx = int( result.cw * ( diff / 2.0 ) );
				result.cw = int( result.cw * ( 1.0 - diff ) );
				result.w = dst_w;
			}

			if ( dst_h >= result.h )
			{
				result.y += ( int( dst_h ) - result.h ) / 2;
			}
			else
			{
				double diff = double( result.h - dst_h ) / result.h;
				result.cy = int( result.ch * ( diff / 2.0 ) );
				result.ch = int( result.ch * ( 1.0 - diff ) );
				result.h = dst_h;
			}

			if ( result.w % 2 ) result.w += 1;

			//If both the source and destination images are interlaced, 
			//we need to place the source at an even line to avoid
			//flipping the field order.
			if ( dst->get_image()->field_order( ) != il::progressive &&
			     src->get_image()->field_order( ) != il::progressive )
			{
				if( result.y % 2 ) result.y += 1;
			}

			return result;
		}

		frame_type_ptr composite( frame_type_ptr background, frame_type_ptr fg, struct geometry geom )
		{
			// Ensure conformance
			std::wstring original_pf = background->pf();
			if ( !is_yuv_planar( background ) )
			{
				background = frame_convert( background, L"yuv444p" );
			}

			ml::frame_type_ptr foreground = frame_convert( fg->shallow( ), background->get_image( )->pf( ) );
			foreground->set_image( il::conform( foreground->get_image( ), il::writable ) );
			foreground->set_alpha( il::conform( foreground->get_alpha( ), il::writable ) );

			// Crop to computed geometry
			foreground = frame_crop( foreground, geom.cx, geom.cy, geom.cw, geom.ch );

			// Aquire requested rescaling algorithm
			il::rescale_filter filter = il::BILINEAR_SAMPLING;
			if ( prop_interp_.value< std::wstring >( ) == L"point" )
				filter = il::POINT_SAMPLING;
			else if ( prop_interp_.value< std::wstring >( ) == L"bicubic" )
				filter = il::BICUBIC_SAMPLING;
			if ( background->get_image( )->field_order( ) == il::progressive ) 
			    foreground->set_image( il::deinterlace( foreground->get_image( ) ) );
			
			foreground = ml::frame_rescale( foreground, geom.w, geom.h, filter );

			// We're going to update both image and alpha here
			background->set_image( il::conform( background->get_image( ), il::writable ) );
			background->set_alpha( il::conform( background->get_alpha( ), il::writable ) );

			// Extract images
			il::image_type_ptr dst = background->get_image( );
			il::image_type_ptr src = foreground->get_image( );

			// Obtain the src and dst image dimensions
			int src_width = src->width( );
			int src_height = src->height( );
			int dst_width = dst->width( );
			int dst_height = dst->height( );

			// Calculate the src x,y offset
			int dst_x = geom.x;
			int dst_y = geom.y;
			int src_x = 0;
			int src_y = 0;

			if ( dst_x < 0 )
			{
				src_width += dst_x;
				src_x -= dst_x;
				dst_x = 0;
			} 	 

			if ( dst_y < 0 )
			{
				src_height += dst_y;
				src_y -= dst_y;
				dst_y = 0;
			}
	  	 
			// Trim from below and to the right
			if ( dst_x + src_width > dst_width )
				src_width -= ( dst_x + src_width - dst_width );

			if ( dst_y + src_height > dst_height )
				src_height -= ( dst_y + src_height - dst_height );

			// Scale down the alphas to the size of the chroma planes 
			il::image_type_ptr half_src_alpha; 
			il::image_type_ptr half_dst_alpha;

			if ( background->get_alpha( ) )
				half_dst_alpha = il::rescale( background->get_alpha( ), background->get_image( )->width( 1 ), background->get_image( )->height( 1 ), 1, filter );
			if ( foreground->get_alpha( ) )
				half_src_alpha = il::rescale( foreground->get_alpha( ), foreground->get_image( )->width( 1 ), foreground->get_image( )->height( 1 ), 1, filter );

			// Determine the x and y factors for the chroma plane sizes
			int plane_x_factor = background->get_image( )->width( ) / background->get_image( )->width( 1 );
			int plane_y_factor = background->get_image( )->height( ) / background->get_image( )->height( 1 );

			// Composite the image if we have any image left...
			for ( size_t p = 0; p < size_t( dst->plane_count( ) ); p ++ )
			{
				if ( src_width > 0 && src_height > 0 )
				{
					// Calculate the byte offsets
					unsigned char *src_ptr = src->data( p ) + src_y * src->pitch( p ) + src_x;
					unsigned char *dst_ptr = dst->data( p ) + dst_y * dst->pitch( p ) + dst_x;

					// Obtain the pitches of both images and reduce by the distance travelled on each row
					int src_remainder = src->pitch( p ) - src_width;
					int dst_remainder = dst->pitch( p ) - src_width;
					int temp_h = src_height;
					int mix = int( 255 * prop_mix_.value< double >( ) );
					int xim = 255 - mix;

					if ( !background->get_alpha( ) && !foreground->get_alpha( ) )
					{
						while ( temp_h -- )
						{
							int temp_w = src_width;
							while( temp_w -- )
							{
								*dst_ptr = static_cast< unsigned char >( ( *dst_ptr * xim + *src_ptr * mix ) / 255 );
								src_ptr ++;
								dst_ptr ++;
							}
							dst_ptr += dst_remainder;
							src_ptr += src_remainder;
						}
					}
					else if ( !background->get_alpha( ) && foreground->get_alpha( ) )
					{
						il::image_type_ptr src_alpha = p == 0 ? foreground->get_alpha( ) : half_src_alpha;
						unsigned char *src_alpha_ptr = src_alpha->data( ) + src_y * src_alpha->pitch( ) + src_x;
						int src_alpha_remainder = src_alpha->pitch( ) - src_width;
						int alpha = 0;
						if ( temp_h > src_alpha->height( ) ) temp_h = src_alpha->height( );
						while ( temp_h -- )
						{
							int temp_w = src_width;
							while( temp_w -- )
							{
								alpha = ( *src_alpha_ptr ++ * mix ) / 255;
								*dst_ptr = static_cast< unsigned char >( ( *dst_ptr * ( 255 - alpha ) + *src_ptr * alpha ) / 255 );
								src_ptr ++;
								dst_ptr ++;
							}
							dst_ptr += dst_remainder;
							src_ptr += src_remainder;
							src_alpha_ptr += src_alpha_remainder;
						}
					}
					else if ( background->get_alpha( ) && !foreground->get_alpha( ) )
					{
						il::image_type_ptr dst_alpha = p == 0 ? background->get_alpha( ) : half_dst_alpha;
						unsigned char *dst_alpha_ptr = dst_alpha->data( ) + dst_y * dst_alpha->pitch( ) + dst_x;
						int dst_alpha_remainder = dst_alpha->pitch( ) - src_width;
						while ( temp_h -- )
						{
							int temp_w = src_width;
							while( temp_w -- )
							{
								*dst_ptr = static_cast< unsigned char >( ( *dst_ptr * ( xim ) + *src_ptr * mix ) / 255 );
								*dst_alpha_ptr = 255;
								src_ptr ++;
								dst_ptr ++;
								dst_alpha_ptr ++;
							}
							dst_ptr += dst_remainder;
							src_ptr += src_remainder;
							dst_alpha_ptr += dst_alpha_remainder;
						}
					}
					else
					{
						il::image_type_ptr dst_alpha = p == 0 ? background->get_alpha( ) : half_dst_alpha;
						unsigned char *dst_alpha_ptr = dst_alpha->data( ) + dst_y * dst_alpha->pitch( ) + dst_x;
						int dst_alpha_remainder = dst_alpha->pitch( ) - src_width;
						il::image_type_ptr src_alpha = p == 0 ? foreground->get_alpha( ) : half_src_alpha;
						unsigned char *src_alpha_ptr = src_alpha->data( ) + src_y * src_alpha->pitch( ) + src_x;
						int src_alpha_remainder = src_alpha->pitch( ) - src_width;
						int alpha = 0;
						while ( temp_h -- )
						{
							int temp_w = src_width;
							while( temp_w -- )
							{
								alpha = ( mix * *src_alpha_ptr ) / 255;
								*dst_ptr = static_cast< unsigned char >( ( *dst_ptr * ( 255 - alpha ) + *src_ptr * alpha ) >> 8 );
								*dst_alpha_ptr = ( ( *src_alpha_ptr * mix ) / 255 ) | *dst_alpha_ptr;
								src_alpha_ptr ++;
								dst_alpha_ptr ++;
								src_ptr ++;
								dst_ptr ++;
							}
							dst_ptr += dst_remainder;
							src_ptr += src_remainder;
							src_alpha_ptr += src_alpha_remainder;
							dst_alpha_ptr += dst_alpha_remainder;
						}
					}
				}

				// FIXME: This is the part which is yuv planar dependent
				if ( p == 0 )
				{
					src_x /= plane_x_factor;
					src_y /= plane_y_factor;
					dst_x /= plane_x_factor;
					dst_y /= plane_y_factor;
					src_width /= plane_x_factor;
					src_height /= plane_y_factor;
					dst_width /= plane_x_factor;
					dst_height /= plane_y_factor;
				}
			}

			//If we converted to yuv444p at the top, we convert back to the original here
			if( original_pf != background->pf( ) )
			{
				background = frame_convert( background, original_pf );
			}

			return background;
		}

	private:
		pcos::property prop_enable_;
		pcos::property prop_mode_;
		pcos::property prop_swap_;
		pcos::property prop_rx_;
		pcos::property prop_ry_;
		pcos::property prop_rw_;
		pcos::property prop_rh_;
		pcos::property prop_mix_;
		pcos::property prop_interp_;
		pcos::property prop_slot_;

		pcos::property prop_frame_rescale_cb_; //Deprecated
		pcos::property prop_image_rescale_cb_; //Deprecated
};

// Colour correction filter
//
// Requires a single connected input.
//
// Carries out typical colour correction operations (hue, contrast, brightness
// and saturation).
//
// Works in the YCbCr colour space only.
//
// Properties:
//
// 	contrast = 0.0 to 1.992 (default: 1.0)
// 		Specifies the contrast adjustment (multiplies Y, Cb and Cr)
//
//	brightness = -32 to 31 (default: 0)
//		Specifies the brightness (added to Y)
//
//	hue = -30 to 30 (default 0)
//		Specifies the hue (Cb = Cb cos hue + Cr sin hue, Cr = Cb cos hue - Cr sin hue)
//
//	saturation = 0.0 to 1.992 (default: 1.0)
//		Specifies the saturation (multiplies Cb and Cr)
//
// Notes:
//
// No range checks are carried out - garbage in, garbage out.
//
// TODO: Support additional colour images

class ML_PLUGIN_DECLSPEC correction_filter : public filter_simple
{
	public:
		correction_filter( )
			: filter_simple( )
			, prop_enable_( pcos::key::from_string( "enable" ) )
			, prop_contrast_( pcos::key::from_string( "contrast" ) )
			, prop_brightness_( pcos::key::from_string( "brightness" ) )
			, prop_hue_( pcos::key::from_string( "hue" ) )
			, prop_saturation_( pcos::key::from_string( "saturation" ) )
		{
			properties( ).append( prop_enable_ = 1 );
			properties( ).append( prop_contrast_ = 1.0 );
			properties( ).append( prop_brightness_ = 0.0 );
			properties( ).append( prop_hue_ = 0.0 );
			properties( ).append( prop_saturation_ = 1.0 );
		}

		// Indicates if the input will enforce a packet decode
		virtual bool requires_image( ) const { return prop_enable_.value< int >( ) == 1; }

		virtual const std::wstring get_uri( ) const { return L"correction"; }

	protected:
		void do_fetch( frame_type_ptr &result )
		{
			acquire_values( );

			result = fetch_from_slot( );

			if ( result && prop_enable_.value< int >( ) )
				result = process_image( result );
		}

	private:
		frame_type_ptr process_image( frame_type_ptr result )
		{
			if ( !is_yuv_planar( result ) )
				result = frame_convert( result, L"yuv420p" );
			result->set_image( il::conform( result->get_image( ), il::writable ) );
			il::image_type_ptr img = result->get_image( );
			if ( img )
			{
				// Interpret properties
				int contrast = int( 256 * prop_contrast_.value< double >( ) );
				int brightness = int( 256 * prop_brightness_.value< double >( ) );
				int sin_hue = int( 256 * ( std::sin( M_PI * prop_hue_.value< double >( ) / 180.0 ) ) );
				int cos_hue = int( 256 * ( std::cos( M_PI * prop_hue_.value< double >( ) / 180.0 ) ) );
				int contrast_saturation = int( 256 * prop_contrast_.value< double >( ) * prop_saturation_.value< double >( ) );

				unsigned char *y = img->data( 0 );
				unsigned char *cb = img->data( 1 );
				unsigned char *cr = img->data( 2 );

				int w = img->width( 0 );
				int h = img->height( 0 );
				int y_rem = img->pitch( 0 ) - img->linesize( 0 );
				int cb_rem = img->pitch( 1 ) - img->linesize( 1 );
				int cr_rem = img->pitch( 2 ) - img->linesize( 2 );
				int t;

				while( h -- )
				{
					t = w;
					while( t -- )
					{
						*y = limit( ( ( ( *y - 16 ) * contrast + brightness ) >> 8 ) + 16, 16, 235 );
						y ++;
					}
					y += y_rem;
				}

				w = img->width( 1 );
				h = img->height( 1 );

				while( h -- )
				{
					t = w;
					while( t -- )
					{
						*cb = limit( ( ( ( ( ( *cb - 128 ) * cos_hue + ( *cr - 128 ) * sin_hue ) >> 8 ) * contrast_saturation ) >> 8 ) + 128, 16, 240 );
						cb ++;
						*cr = limit( ( ( ( ( ( *cr - 128 ) * cos_hue - ( *cb - 128 ) * sin_hue ) >> 8 ) * contrast_saturation ) >> 8 ) + 128, 16, 240 );
						cr ++;
					}
					cb += cb_rem;
					cr += cr_rem;
				}
			}

			return result;
		}

		inline int limit( int v, int l, int u )
		{
			return v < l ? l : v > u ? u : v;
		}

		pcos::property prop_enable_;
		pcos::property prop_contrast_;
		pcos::property prop_brightness_;
		pcos::property prop_hue_;
		pcos::property prop_saturation_;
};

// Frame rate filter
//
// Changes the frame rate to the requested value.
//
// Properties:
//
// 	fps_num = numerator [default: 25]
// 	fps_den = denominator [default: 1]
//

static pl::pcos::key key_audio_reversed_( pcos::key::from_string( "audio_reversed" ) );

class ML_PLUGIN_DECLSPEC frame_rate_filter : public filter_type
{
	typedef cl::lru< int, ml::frame_type_ptr > frame_cache;
	typedef boost::rational< boost::int64_t > rational;

	public:

		frame_rate_filter( )
			: filter_type( )
			, position_( 0 )
			, expected_( -1 )
			, src_frames_( 0 )
			, src_frequency_( 0 )
			, src_fps_num_( -1 )
			, src_fps_den_( -1 )
			, src_has_image_( false )
			, prop_fps_num_( pcos::key::from_string( "fps_num" ) )
			, prop_fps_den_( pcos::key::from_string( "fps_den" ) )
			, prop_audio_direction_( pcos::key::from_string( "audio_direction" ) )
			, prop_check_on_connect_( pcos::key::from_string( "check_on_connect" ) )
			, current_dir_( 1 )
			, reseat_( )
			, next_input_( 0 )
			, valid_on_connect_( false )
		{
			properties( ).append( prop_fps_num_ = 25 );
			properties( ).append( prop_fps_den_ = 1 );
			properties( ).append( prop_audio_direction_ = 0 );
			properties( ).append( prop_check_on_connect_ = 1 );
			reseat_ = audio::create_reseat( );
		}

		// Indicates if the input will enforce a packet decode
		virtual bool requires_image( ) const 
		{ return src_has_image_ && !( prop_fps_num_.value< int >( ) == src_fps_num_ && prop_fps_den_.value< int >( ) == src_fps_den_ ); }

		virtual int get_frames( ) const
		{
			using cl::utilities::clamp;
			const int fps_num = prop_fps_num_.value< int >( );
			const int fps_den = prop_fps_den_.value< int >( );
			if ( fps_num > 0 && fps_den > 0 && src_frames_ != INT_MAX )
			{
				const boost::int64_t dst_frames = round_up( map_source_to_dest( src_frames_ ) );
				return int( clamp< boost::int64_t >( dst_frames, 0, boost::integer_traits< int >::const_max ) );
			}
			else
			{
				return src_frames_;
			}
		}

		virtual const std::wstring get_uri( ) const { return L"frame_rate"; }

		virtual void seek( const int position, const bool relative = false )
		{
			if ( relative )
				position_ += position;
			else
				position_ = position;

			if ( position_ < 0 )
				position_ = 0;

			if ( position >= get_frames( ) )
				position_ = get_frames( ) - 1;
		}

		virtual int get_position( ) const
		{
			return position_;
		}

		bool incomplete_input( input_type_ptr input )
		{
			bool result = !input || input->get_uri( ) == L"nudger:";
			if ( !result )
				for( size_t i = 0; !result && i < input->slot_count( ); i ++ )
					result = incomplete_input( input->fetch_slot( i ) );
			return result;
		}

		virtual void on_slot_change( input_type_ptr input, int )
		{
			valid_on_connect_ = !incomplete_input( input );
			if ( valid_on_connect_ )
				check_input( input );
		}

		void check_input( input_type_ptr input )
		{
			input->sync( );

			frame_type_ptr frame = input->fetch( );

			src_frames_ = input->get_frames( );
			src_fps_num_ = -1;
			src_fps_den_ = -1;
			src_frequency_ = 0;
			valid_on_connect_ = frame != ml::frame_type_ptr( );

			if ( valid_on_connect_ )
			{
				src_fps_num_ = frame->get_fps_num( );
				src_fps_den_ = frame->get_fps_den( );
				src_has_image_ = frame->has_image( );
				if ( frame->get_audio( ) )
					src_frequency_ = frame->get_audio( )->frequency( );
			}
		}

	protected:
		void do_fetch( frame_type_ptr &result )
		{
			input_type_ptr input = fetch_slot( );

			if ( !valid_on_connect_ )
			{
				check_input( input );
				ARENFORCE_MSG( valid_on_connect_, "Can't obtain info" );
			}

			const int fps_num = prop_fps_num_.value< int >( );
			const int fps_den = prop_fps_den_.value< int >( );

			// Check which direction we're heading in regardless of audio
			if ( position_ != expected_ )
				current_dir_ = expected_ - position_ == 2 ? -1 : 1;

			if ( last_frame_ && last_frame_->get_position( ) == get_position( ) )
			{
				result = last_frame_->shallow( );
			}
			else if ( input && fps_num > 0 && fps_den > 0 )
			{
				if ( fps_num == src_fps_num_ && fps_den == src_fps_den_ )
				{
					input->seek( position_ );
					result = input->fetch( );
					ARENFORCE( result );
					result = result->shallow( );
					handle_reverse_input_audio( result );
				}
				else if ( src_frequency_ == 0 )
				{
					input->seek( round_down( map_dest_to_source( position_ ) ) );
					result = input->fetch( );
					ARENFORCE( result );
					result = result->shallow( );
				}
				else
				{
					result = time_shift( input, fps_num, fps_den );
				}
			}
			else if ( input )
			{
				result = fetch_from_slot( );
				ARENFORCE( result );
				result = result->shallow( );
				handle_reverse_input_audio( result );
			}

			if ( result )
			{
				result->set_position( position_ );
				if( fps_num > 0 && fps_den > 0 )
					result->set_fps( fps_num, fps_den );

				last_frame_ = result->shallow( );
			}

			// This is the next frame we expect
			expected_ = position_ + current_dir_;
		}

		virtual void sync_frames( )
		{
			src_frames_ = fetch_slot( 0 ) ? fetch_slot( 0 )->get_frames( ) : 0;
		}

	private:
		rational map_source_to_dest( int position ) const
		{
			const rational src_fps( src_fps_num_, src_fps_den_ );
			const rational dst_fps( prop_fps_num_.value< int >( ), prop_fps_den_.value< int >( ) );
			return position * dst_fps / src_fps;
		}

		rational map_dest_to_source( int position ) const
		{
			const rational src_fps( src_fps_num_, src_fps_den_ );
			const rational dst_fps( prop_fps_num_.value< int >( ), prop_fps_den_.value< int >( ) );
			return position * src_fps / dst_fps;
		}

		static boost::int64_t round_down( rational r )
		{
			return boost::rational_cast< boost::int64_t >( r );
		}

		static boost::int64_t round_up( rational r )
		{
			const boost::int64_t i = boost::rational_cast< boost::int64_t >( r );
			if( r.denominator() == 1 )
				return i;
			else
				return i + 1;
		}

		ml::frame_type_ptr time_shift( ml::input_type_ptr &input, const int fps_num, const int fps_den )
		{
			//If we're currently playing backwards (current_dir_ is -1), then we should fetch the frames in backwards order
			//as well, so as to not confuse the filters between us and the input about what direction we are playing in.
			//When fetching backwards, we expect the audio on the frames to have been reversed (if that's not the case, we
			//reverse it ourselves). Consider the following simplified example input with 4 samples per frame, and assume
			//that we want to convert to a framerate that has 5 samples per frame instead:
			//
			// frame no:           0         1
			// sample no:      |3 2 1 0| |7 6 5 4|
			//
			//We must now fetch from frame 1 first, taking sample 4, then fetch from frame 0, taking samples 3, 2, 1, and 0.

			ml::frame_type_ptr result;

			// Determine frame position in input and number of samples that we require
			const int target = ( current_dir_ > 0 )
				? round_down( map_dest_to_source( position_ ) )
				: round_up( map_dest_to_source( position_ + 1 ) ) - 1;
			const int samples = audio::samples_for_frame( position_, src_frequency_, fps_num, fps_den );

			// After a seek, we need to know how many samples to discard from the first audio packet we extract
			int discard = 0;

			// Handle a seek now
			if ( position_ != expected_ )
			{
				int start_input = target;

				// We always need to clear the reseat after a seek - the cache is cleared to allow for non-sample accurate inputs
				reseat_->clear( );
				cache_.clear( );

				// We now need to align ourselves with the exact sample in the input that is required to satisfy the output

				if( current_dir_ > 0 )
				{
					boost::int64_t samples_in  = audio::samples_to_frame( start_input, src_frequency_, src_fps_num_, src_fps_den_ );
					boost::int64_t samples_out = audio::samples_to_frame( position_, src_frequency_, fps_num, fps_den );

					// If the sample offset of the input is after the output, we need to step back
					if ( samples_in > samples_out )
						samples_in = audio::samples_to_frame( -- start_input, src_frequency_, src_fps_num_, src_fps_den_ );

					ARENFORCE_MSG( samples_in <= samples_out, "Forward miscalculation - samples_in %1% > samples_out %2%" )( samples_in )( samples_out );

					// We will now discard the samples we don't need
					discard = samples_out - samples_in;
				}
				else
				{
					boost::int64_t samples_in  = audio::samples_to_frame( start_input + 1, src_frequency_, src_fps_num_, src_fps_den_ );
					boost::int64_t samples_out = audio::samples_to_frame( position_ + 1, src_frequency_, fps_num, fps_den );

					// If the sample offset of the input is before the output, we need to step forward
					if ( samples_in < samples_out )
						samples_in = audio::samples_to_frame( ++ start_input, src_frequency_, src_fps_num_, src_fps_den_ );

					ARENFORCE_MSG( samples_in >= samples_out, "Reverse miscalculation - samples_in %1% < samples_out %2%" )( samples_in )( samples_out );

					// We will now discard the samples we don't need
					discard = samples_in - samples_out;
				}

				// Reset the input position we shall start reading from
				next_input_ = start_input;
			}

			// Keep fetching while we're able seek on the input, and the number of samples we want has not been obtained
			while ( next_input_ >= 0 && next_input_ < src_frames_ && !reseat_->has( samples ) )
			{
				// Break if we haven't got the target and we're moving away from it
				if ( !cache_.fetch( target ) )
				{
					ARENFORCE_MSG( ( next_input_ <= target && current_dir_ == 1 ) || ( next_input_ >= target && current_dir_ == -1 ), 
								   "Moving away from target %1% at %2% + %3%" )( target )( next_input_ )( current_dir_ );
				}

				// Seek and fetch the next input frame
				input->seek( next_input_ );
				frame_type_ptr frame = input->fetch( );
				ARENFORCE( frame && frame->get_audio() );

				// Make a shallow copy to allow modification (like audio direction)
				frame = frame->shallow( );

				// Reverse the audio as required
				handle_reverse_input_audio( frame );

				// Append audio to the reseat and discard what we need to on the first frame
				if ( discard < frame->get_audio( )->samples( ) )
				{
					reseat_->append( frame->get_audio( ), discard );
					discard = 0;
				}
				else
				{
					discard -= frame->get_audio( )->samples( );
				}

				// Cache it to allow us to recover images if necessary
				cache_.append( frame->get_position( ), frame );

				// Determine the next input position required
				next_input_ += current_dir_;
			}

			// Finally, if we have the frame from the input that we want, clone it and modify as required 
			// (note that fps and position are handled in the do_fetch to avoid duplication)
			if ( cache_.fetch( target ) )
			{
				result = cache_.fetch( target )->shallow( );

				if ( reseat_->size( ) )
					result->set_audio( reseat_->retrieve( samples, true ) );
				else
					result->set_audio( ml::audio_type_ptr( ) );

				handle_reverse_output_audio( result );

				if ( result->get_image( ) )
					result->get_image( )->set_writable( false );
				if ( result->get_alpha( ) )
					result->get_alpha( )->set_writable( false );
			}

			return result;
		}

		void handle_reverse_input_audio( ml::frame_type_ptr frame ) const
		{
			ml::audio_type_ptr audio = frame->get_audio( );

			if ( audio )
			{
				int reversed = pl::pcos::value< int >( frame->properties( ), key_audio_reversed_, 0 );
				if ( ( current_dir_ < 0 && reversed == 0 ) || ( current_dir_ > 0 && reversed == 1 ) )
				{
					pl::pcos::assign< int >( frame->properties( ), key_audio_reversed_, reversed ? 0 : 1 );
					frame->set_audio( ml::audio::reverse( audio ) );
				}
			}
		}

		void handle_reverse_output_audio( ml::frame_type_ptr frame ) const
		{
			ml::audio_type_ptr audio = frame->get_audio( );

			if ( audio )
			{
				int reversed = pl::pcos::value< int >( frame->properties( ), key_audio_reversed_, 0 );
				if ( ( current_dir_ < 0 && reversed == 0 ) || ( current_dir_ > 0 && reversed == 1 ) )
					pl::pcos::assign< int >( frame->properties( ), key_audio_reversed_, reversed ? 0 : 1 );
			}
		}

		int position_;
		int expected_;
		int src_frames_;
		int src_frequency_;
		int src_fps_num_;
		int src_fps_den_;
		bool src_has_image_;
		pcos::property prop_fps_num_;
		pcos::property prop_fps_den_;
		pcos::property prop_audio_direction_;
		pcos::property prop_check_on_connect_;
		int current_dir_;
		audio::reseat_ptr reseat_;
		std::map < int, frame_type_ptr > map_;
		frame_type_ptr last_frame_;
		int next_input_;
		frame_cache cache_;
		bool valid_on_connect_;
};

//
// Clip filter
//
// Clip the connected input to the specified in/out points.
//
// Properties:
//
// 	in = -n to n [default: 0]
// 		The first frame expressed in the source frame rate.
// 		Negative offsets are treated as relative to the length of the
// 		connected input.
//
// 	out = -n to n [default: inherited from connected input]
// 		The last frame expressed in the source frame rate.
// 		Negative offsets are treated as relative to the length of the
// 		connected input.
//
// Caveat: Properties should be set prior to the first fetch.

class ML_PLUGIN_DECLSPEC clip_filter : public filter_type
{
	public:
		clip_filter( )
			: filter_type( )
			, position_( 0 )
			, last_position_( -1 )
			, prop_in_( pcos::key::from_string( "in" ) )
			, prop_out_( pcos::key::from_string( "out" ) )
			, src_frames_( 0 )
		{
			properties( ).append( prop_in_ = 0 );
			properties( ).append( prop_out_ = -1 );
		}

		// Indicates if the input will enforce a packet decode
		virtual bool requires_image( ) const { return false; }

		virtual int get_frames( ) const
		{
			// Calculate from input_frames
			int frames = get_out( src_frames_ ) - get_in( src_frames_ );
			return ( frames <= 0 ? - frames : frames );
		}

		virtual const std::wstring get_uri( ) const { return L"clip"; }

		virtual void seek( const int position, const bool relative = false )
		{
			if( relative )
				position_ += position;
			else
				position_ = position;
		}

		virtual int get_position( ) const
		{
			return position_;
		}

	protected:

		void do_fetch( frame_type_ptr &result )
		{
			input_type_ptr input = fetch_slot( );

			if ( input )
			{
				if ( get_in( src_frames_ ) < get_out( src_frames_ ) )
				{
					input->seek( get_in( src_frames_ ) + get_position( ) );
					result = input->fetch( );
				}
				else
				{
					input->seek( get_in( src_frames_ ) - get_position( ) );
					result = input->fetch( );
					if ( result && result->get_audio( ) )
					{
						int reversed = pl::pcos::value< int >( result->properties( ), key_audio_reversed_, 0 );
						if ( reversed == 0 )
							result->set_audio( audio::reverse( result->get_audio( ) ) );
						pl::pcos::assign< int >( result->properties( ), key_audio_reversed_, 0 );
					}
				}

				if ( result )
					result->set_position( get_position( ) );
			}
		}

		virtual void sync_frames( )
		{
			src_frames_ = fetch_slot( 0 ) ? fetch_slot( 0 )->get_frames( ) : 0;
		}

	private:
		inline int get_in( const int &frames ) const
		{
			int in = prop_in_.value< int >( );
			if ( in >= frames )
				in = frames - 1;
			if ( in < 0 )
				in = frames + in;
			if ( in < 0 )
				in = 0;
			return in;
		}

		inline int get_out( const int &frames ) const
		{
			int out = prop_out_.value< int >( );
			if ( out >= frames )
				out = frames;
			if ( out < 0 )
				out = frames + out + 1;
			if ( out < 0 )
				out = 0;
			return out;
		}

		int position_;
		int last_position_;
		pcos::property prop_in_;
		pcos::property prop_out_;
		mutable int src_frames_;
};

// Deinterlace
//
// Deinterlaces an image on demand.

class ML_PLUGIN_DECLSPEC deinterlace_filter : public filter_simple
{
	public:
		deinterlace_filter( )
			: filter_simple( )
			, prop_enable_( pcos::key::from_string( "enable" ) )
			, prop_force_( pcos::key::from_string( "force" ) )
		{
			properties( ).append( prop_enable_ = 1 );
			properties( ).append( prop_force_ = 0 );
		}

		// Indicates if the input will enforce a packet decode
		virtual bool requires_image( ) const { return prop_enable_.value< int >( ) == 1; }

		virtual const std::wstring get_uri( ) const { return L"deinterlace"; }

	protected:
		void do_fetch( frame_type_ptr &result )
		{
			acquire_values( );

			result = fetch_from_slot( );

			if ( prop_enable_.value< int >( ) && result && result->get_image( ) )
			{
				if ( result->get_image( )->field_order( ) != il::progressive || prop_force_.value< int >( ) )
				{
					result->set_image( il::conform( result->get_image( ), il::writable ) );
					if ( prop_force_.value< int >( ) )
						result->get_image( )->set_field_order( prop_force_.value< int >( ) == 2 ? il::bottom_field_first : il::top_field_first );
					result->set_image( il::deinterlace( result->get_image( ) ) );
				}
			}
		}

	private:
		pcos::property prop_enable_;
		pcos::property prop_force_;
};

// Lerp
//
// Linear interpolation keyframing filter.

static bool key_sort( const pcos::key &k1, const pcos::key &k2 )
{
	return strcmp( k1.as_string( ), k2.as_string( ) ) < 0;
}

class ML_PLUGIN_DECLSPEC lerp_filter : public filter_simple
{
	public:
		lerp_filter( )
			: filter_simple( )
			, prop_enable_( pcos::key::from_string( "enable" ) )
			, prop_in_( pcos::key::from_string( "in" ) )
			, prop_out_( pcos::key::from_string( "out" ) )
		{
			properties( ).append( prop_enable_ = 1 );
			properties( ).append( prop_in_ = 0 );
			properties( ).append( prop_out_ = -1 );
		}

		// Indicates if the input will enforce a packet decode
		virtual bool requires_image( ) const { return false; }

		virtual const std::wstring get_uri( ) const { return L"lerp"; }

	protected:
		void do_fetch( frame_type_ptr &result )
		{
			acquire_values( );

			input_type_ptr input = fetch_slot( );

			if ( input )
			{
				input->seek( get_position( ) );
				result = input->fetch( );
				if ( prop_enable_.value< int >( ) )
				{
					pcos::key_vector keys = properties( ).get_keys( );
					std::sort( keys.begin( ), keys.end( ), key_sort );
					int frames = get_frames( );
					pcos::property_container input_props = properties( );
					pcos::property_container &frame_props = result->properties( );
					for( pcos::key_vector::iterator it = keys.begin( ); result && it != keys.end( ); ++it )
						evaluate( frame_props, input_props, *it, frames );
				}
			}
		}

		void correct_in_out( int &in, int &out, const int &frames )
		{
			if ( in < 0 )
				in = frames + in;
			if ( in < 0 )
				in = 0;
			if ( in >= frames )
				in = frames - 1;

			if ( out < 0 )
				out = frames + out;
			if ( out <= 0 )
				out = 0;
			if ( out >= frames )
				out = frames - 1;
		}

		void assign_property( pl::pcos::property_container &props, pcos::key &target, double result )
		{
			pl::pcos::property prop = props.get_property_with_key( target );
			if ( prop.valid( ) )
			{
				prop = result;
			}
			else
			{
				pcos::property final( target );
				props.append( final = result );
			}
		}

		void assign_property( pl::pcos::property_container &props, pcos::key &target, std::wstring result )
		{
			pl::pcos::property prop = props.get_property_with_key( target );
			if ( prop.valid( ) )
			{
				prop = result;
			}
			else
			{
				pcos::property final( target );
				props.append( final = result );
			}
		}

		virtual void evaluate( pl::pcos::property_container &props, pl::pcos::property_container &input, pcos::key &key, const int &frames )
		{
			std::string name( key.as_string( ) );

			if ( name.substr( 0, 2 ) == "@@" && frames )
			{
				name = name.substr( 2 );
				if ( name.find( ":" ) != std::string::npos )
					name = name.substr( 0, name.find( ":" ) );

				pcos::key target = pcos::key::from_string( name.c_str( ) );

				pcos::property prop = input.get_property_with_key( key );
				std::wstring value = prop.value< std::wstring >( );

				double lower, upper, result;
				int in = prop_in_.value< int >( );
				int out = prop_out_.value< int >( );
				int position = get_position( ) % frames;
				int temp = 0;

				int count = sscanf( cl::str_util::to_string( value ).c_str( ), "%lf:%lf:%d:%d:%d", &lower, &upper, &in, &out, &temp );

				if ( in < 0 || out < 0 )
					correct_in_out( in, out, frames );

				if ( temp == 0 && count > 0 && position >= in && position <= out )
				{
					if ( count >= 2 )
						result = lower + ( double( position - in ) / double( out - in ) ) * ( upper - lower );
					else
						result = double( lower );

					assign_property( props, target, result );
				}
				else if ( temp == 1 )
				{
					if ( in == out )
					{
						assign_property( props, target, double( upper ) );
					}
					else
					{
						double result = lower + ( double( in + 1 ) / double( out ) ) * ( upper - lower );
						assign_property( props, target, result );
						char temp[ 1024 ];
						sprintf( temp, "%lf:%lf:%d:%d:%d", lower, upper, in + 1, out, 1 );
						prop = cl::str_util::to_wstring( temp );
					}
				}
				else if ( count == 0 )
				{
					if ( value.find( L":" ) != std::wstring::npos )
					{
						count = sscanf( cl::str_util::to_string( value.substr( value.find( L":" ) ) ).c_str( ), ":%d:%d", &in, &out );
						if ( in < 0 || out < 0 )
							correct_in_out( in, out, frames );
						if ( position >= in && position <= out )
							assign_property( props, target, value.substr( 0, value.find( L":" ) ) );
					}
					else
					{
						assign_property( props, target, value );
					}
				}
			}
		}

		pcos::property prop_enable_;
		pcos::property prop_in_;
		pcos::property prop_out_;
};

// Bezier
//
// Experimental Bezier interpolation
//
// Horrifically inefficient. Some code originating from:
//
// http://en.wikipedia.org/wiki/B%C3%A9zier_curve

class ML_PLUGIN_DECLSPEC bezier_filter : public lerp_filter
{
	public:
		bezier_filter( ) : lerp_filter( ) { }
		virtual const std::wstring get_uri( ) const { return L"bezier"; }

	protected:
		typedef struct
		{
    		double x;
    		double y;
		}
		point;

		void calculate( double &x, double &y, point points[ 4 ], double t )
		{
    		double ax, bx, cx;
    		double ay, by, cy;
    		double t_squared, t_cubed;

    		// calculate the polynomial coefficients
    		cx = 3.0 * ( points[ 1 ].x - points[ 0 ].x );
    		bx = 3.0 * ( points[ 2 ].x - points[ 1 ].x ) - cx;
    		ax = points[ 3 ].x - points[ 0 ].x - cx - bx;
        
    		cy = 3.0 * ( points[ 1 ].y - points[ 0 ].y );
    		by = 3.0 * ( points[ 2 ].y - points[ 1 ].y ) - cy;
    		ay = points[ 3 ].y - points[ 0 ].y - cy - by;
        
    		// calculate the curve point at parameter value t
    		t_squared = t * t;
    		t_cubed = t_squared * t;
        
    		x = ( ax * t_cubed ) + ( bx * t_squared ) + ( cx * t ) + points[ 0 ].x;
    		y = ( ay * t_cubed ) + ( by * t_squared ) + ( cy * t ) + points[ 0 ].y;
		}

		void evaluate( pl::pcos::property_container &props, pl::pcos::property_container &input, pcos::key &key, const int &frames )
		{
			std::string name( key.as_string( ) );

			if ( name.substr( 0, 2 ) == "@@" && frames )
			{
				int position = get_position( ) % frames;
				pcos::property prop = input.get_property_with_key( key );
				std::wstring value = prop.value< std::wstring >( );

				point points[ 4 ];
				int in = prop_in_.value< int >( );
				int out = prop_out_.value< int >( );

				int count = sscanf( cl::str_util::to_string( value ).c_str( ), "%lf,%lf:%lf,%lf:%lf,%lf:%lf,%lf:%d:%d", 
																	  &points[ 0 ].x, &points[ 0 ].y,
																	  &points[ 1 ].x, &points[ 1 ].y,
																	  &points[ 2 ].x, &points[ 2 ].y,
																	  &points[ 3 ].x, &points[ 3 ].y,
																	  &in, &out );

				correct_in_out( in, out, frames );

				if ( count > 0 && position >= in && position <= out )
				{
					double x = 0.0;
					double y = 0.0;
				
					calculate( x, y, points, double( position - in ) / double( out - in ) );

					size_t index = name.rfind( "," );
					if ( index !=  std::string::npos )
					{
						pcos::key target1 = pcos::key::from_string( name.substr( 2, index - 2 ).c_str( ) );
						pcos::key target2 = pcos::key::from_string( name.substr( index - 1 ).c_str( ) );
						assign_property( props, target1, x );
						assign_property( props, target2, y );
					}
					else
					{
						pcos::key target = pcos::key::from_string( name.substr( 2 ).c_str( ) );
						assign_property( props, target, x );
					}
				}
			}
		}
};

// Wave Visualisation
//
// Provides a wave visualisation of the audio.
//
// Expects a single input.
//
// Properties:
//
//	force = 0 [default], 1 or 2
//		if 0: only provide visualisation when no image exists
//		if 1: destroy existing image and provide a new one
//		if 2: overlay visualisation on the existing image (needs geometry)
//
//	width, height = 640, 480 [default]
//		dimensions to generate image with (ignored in overlay case)
//
//	colourspace = "r8g8b8" [default] or "yuv420p"
//		colourspace to generate (only used in overlay case if input is unsupported)
//
//	type = 0 [default]
//		reserved for future multiple visualisation type (wave form only for now)
//
// TODO: Geometry of overlay
//
// TODO: Better visualisations

class ML_PLUGIN_DECLSPEC visualise_filter : public filter_simple
{
	public:
		visualise_filter( )
			: filter_simple( )
			, prop_force_( pcos::key::from_string( "force" ) )
			, prop_width_( pcos::key::from_string( "width" ) )
			, prop_height_( pcos::key::from_string( "height" ) )
			, prop_sar_num_(  pcos::key::from_string( "sar_num" ) )
			, prop_sar_den_(  pcos::key::from_string( "sar_den" ) )
			, prop_type_( pcos::key::from_string( "type" ) )
			, prop_hold_( pcos::key::from_string( "hold" ) )
			, prop_colourspace_( pcos::key::from_string( "colourspace" ) )
			, prop_scattered_( pcos::key::from_string( "scattered" ) )
			, prop_reverse_( pcos::key::from_string( "reverse" ) )
			, previous_( )
			, previous_sar_num_( 59 )
			, previous_sar_den_( 54 )
		{
			properties( ).append( prop_force_ = 0 );
			properties( ).append( prop_width_ = 640 );
			properties( ).append( prop_height_ = 480 );
			properties( ).append( prop_sar_num_ = 1 );
			properties( ).append( prop_sar_den_ = 1 );
			properties( ).append( prop_type_ = 0 );
			properties( ).append( prop_hold_ = 1 );
			properties( ).append( prop_colourspace_ = std::wstring( L"yuv420p" ) );
			properties( ).append( prop_scattered_ = 0 );
			properties( ).append( prop_reverse_ = 0 );
		}

		virtual const std::wstring get_uri( ) const { return L"visualise"; }

		// Indicates if the input will enforce a packet decode
		virtual bool requires_image( ) const { return prop_force_.value< int >( ) == 1; }

		virtual void on_slot_change( ml::input_type_ptr input, int )
		{
			previous_ = il::image_type_ptr( );
		}

	protected:
		void do_fetch( frame_type_ptr &result )
		{
			acquire_values( );

			result = fetch_from_slot( );

			if ( result && ( ( previous_ == 0 && !result->has_image( ) ) || prop_force_.value< int >( ) ) )
			{
				visualise( result );
			}
			else if ( result && !result->has_image( ) )
			{
				result->set_image( previous_ );
				result->set_sar( previous_sar_num_, previous_sar_den_ );
			}
			else if ( result )
			{
				if ( prop_hold_.value< int >( ) )
				{
					previous_ = result->get_image( );
					result->get_sar( previous_sar_num_, previous_sar_den_ );
				}
			}
		}

	private:
		void visualise( frame_type_ptr frame )
		{
			if ( frame->get_audio( ) != 0 )
			{
				int type = prop_type_.value< int >( );
				std::wstring colourspace = prop_colourspace_.value< std::wstring >( );
				int width = prop_width_.value< int >( );
				int height = prop_height_.value< int >( );
				il::image_type_ptr image = frame->get_image( );

				if ( image == 0 || prop_force_.value< int >( ) == 1 )
				{
					if ( colourspace == L"yuv420p" )
					{
						int by, bu, bv;
						il::rgb24_to_yuv444( by, bu, bv, 0, 0, 0 );
						image = il::allocate( L"yuv420p", width, height );
						fill( image, 0, by );
						fill( image, 1, bu );
						fill( image, 2, bv );
					}
					else
					{
						image = il::allocate( L"r8g8b8", width, height );
						memset( image->data( ), 0, image->size( ) );
					}

					frame->set_image( image );
					frame->set_sar( prop_sar_num_.value< int >( ), prop_sar_den_.value< int >( ) );
				}
				else
				{
					if ( image->pf( ) == L"yuv420p" || image->pf( ) == L"r8g8b8" )
						colourspace = image->pf( );
					else
						frame_convert( frame, colourspace.c_str() );
				}

				if ( type == 0 && colourspace == L"r8g8b8" )
					wave_rgb( frame );
				else if ( type == 0 && colourspace == L"yuv420p" )
					wave_yuv( frame );
				else if ( type == 1 && colourspace == L"yuv420p" )
					wave_yuv_split( frame );
				else
					wave_rgb( frame );
			}
		}

		void wave_rgb( frame_type_ptr frame )
		{
			frame->set_image( il::conform( frame->get_image( ), il::writable ) );

			audio_type_ptr audio = ml::audio::coerce< ml::audio::pcm16 >( frame->get_audio( ) );

			if ( prop_reverse_.value< int >( ) )
				audio = ml::audio::reverse( audio );

			il::image_type_ptr image = frame->get_image( );

			int width = image->width( );
			int height = image->height( );

			short *buff = ( short * )audio->pointer( );
			int samples = audio->samples( );
			int channels = audio->channels( );

			double sample_offset = double( samples ) / width;
			int pitch = image->pitch( );
			unsigned char *middle = image->data( ) + pitch * image->height( ) / 2;

			short sample;
			unsigned char *p;
			int i, j;
			int offset;

			if ( prop_scattered_.value< int >( ) == 0 ) {
				for ( i = 0; i < width; i ++ )
				{
					offset = i * 3;
					for ( j = 0; j < channels; j ++ )
					{
						sample = int( ( double( ( buff + int( sample_offset * i ) * channels )[ j ] ) / 32767.0 ) * ( height / 2 ) );
						p = middle - sample * pitch + offset;
						*p ++ = j == 0 ? 255 : 0;
						*p ++ = j == 0 ? 0 : 255;
						*p ++ = 0;
					}
				}
			} else {
				// We'll split the view into at most 2 columns
				// and at most (channels / 2) rows, ordered as
				// columns in a news paper.
				// The grid layout will be:
				// _Channels_ | _size_
				//  1 {/2=0}  |  1 x 1
				//  2 {/2=1}  |  1 x 2
				//  3 {/2=1}  |  2 x 2 (bottom right empty)
				//  4 {/2=2}  |  2 x 2
				//  5 {/2=2}  |  2 x 3 (bottom right empty)
				//  6 {/2=3}  |  2 x 3
				//  7 {/2=3}  |  2 x 4 (bottom right empty)
				//     ...

				if (channels > 2)
				{
					width /= 2;
					height /= ((channels + 1) / 2);
				}
				else
					height /= channels;

				sample_offset = double( samples ) / width;

				for ( j = 0; j < channels; j ++ )
				{
					size_t x, y;
					if ( channels <= 2 || j < ((channels+1) / 2) ) {
						x = 0;
						y = j * height;
					} else {
						x = width;
						y = (j - (channels + 1) / 2) * height;
					}

					middle = image->data( ) + pitch * ( y + height / 2 );

					for ( i = 0; i < width; i ++ )
					{
						offset = (x + i) * 3;

						sample = int( ( double( ( buff + int( sample_offset * i ) * channels )[ j ] ) / 32767.0 ) * ( height / 2 ) );
						p = middle - sample * pitch + offset;
						*p ++ = j == 0 ? 255 : 0;
						*p ++ = j == 0 ? 0 : 255;
						*p ++ = 0;
					}
				}
			}
		}

		void line( unsigned char *p, unsigned char *m, int pitch, unsigned char v )
		{
			if ( p <= m + pitch )
			{
				while( p <= m + pitch )
				{
					*p = 128 + ( *p + v - 256 ) / 2;
					p += pitch;
				}
			}
			else if ( p > m + pitch )
			{
				while( p > m )
				{
					*p = 128 + ( *p + v - 256 ) / 2;
					p -= pitch;
				}
			}
		}

		void wave_yuv( frame_type_ptr frame )
		{
			frame->set_image( il::conform( frame->get_image( ), il::writable ) );

			int ry, ru, rv;
			il::rgb24_to_yuv444( ry, ru, rv, 255, 0, 0 );

			int gy, gu, gv;
			il::rgb24_to_yuv444( gy, gu, gv, 255, 255, 255 );

			audio_type_ptr audio = ml::audio::coerce< ml::audio::pcm16 >( frame->get_audio( ) );

			if ( prop_reverse_.value< int >( ) )
				audio = ml::audio::reverse( audio );

			il::image_type_ptr image = frame->get_image( );
			int width = image->width( );
			int height = image->height( );

			short *buff = ( short * )audio->pointer( );
			int samples = audio->samples( );
			int channels = audio->channels( );

			double sample_offset = double( samples ) / width;
			int pitch_y = image->pitch( 0 );
			int pitch_u = image->pitch( 1 );
			int pitch_v = image->pitch( 2 );
			unsigned char *middle_y = image->data( 0 ) + pitch_y * image->height( ) / 2;
			unsigned char *middle_u = image->data( 1 ) + pitch_u * image->height( ) / 4;
			unsigned char *middle_v = image->data( 2 ) + pitch_v * image->height( ) / 4;

			short sample;
			unsigned char *p;
			int i, j;

			for ( i = 0; i < width; i ++ )
			{
				for ( j = 0; j < channels; j ++ )
				{
					sample = int( ( double( ( buff + int( sample_offset * i ) * channels )[ j ] ) / 32769.0 ) * ( height / 2 ) );
					p = middle_y - sample * pitch_y + i;
					line( p, middle_y, pitch_y, j == 0 ? ry : gy );
					sample /= 2;
					p = middle_u - sample * pitch_u + i / 2;
					line( p, middle_u, pitch_u, j == 0 ? ru : gu );
					p = middle_v - sample * pitch_v + i / 2;
					line( p, middle_v, pitch_v, j == 0 ? rv : gv );
				}
			}
		}

		void wave_yuv_split( frame_type_ptr frame )
		{
			frame->set_image( il::conform( frame->get_image( ), il::writable ) );

			int ry, ru, rv;
			il::rgb24_to_yuv444( ry, ru, rv, 255, 0, 0 );

			int gy, gu, gv;
			il::rgb24_to_yuv444( gy, gu, gv, 255, 255, 255 );

			audio_type_ptr audio = ml::audio::coerce< ml::audio::pcm16 >( frame->get_audio( ) );
			il::image_type_ptr image = frame->get_image( );
			int width = image->width( );
			int height = image->height( );

			short *buff = ( short * )audio->pointer( );
			int samples = audio->samples( );
			int channels = audio->channels( );

			double sample_offset = double( samples ) / width;
			int pitch_y = image->pitch( 0 );
			int pitch_u = image->pitch( 1 );
			int pitch_v = image->pitch( 2 );

			short sample;
			unsigned char *p;
			int i, j;

			for ( i = 0; i < width; i ++ )
			{
				unsigned char *middle_y = image->data( 0 ) + pitch_y * height / ( 2 * channels );
				unsigned char *middle_u = image->data( 1 ) + pitch_u * height / ( 4 * channels );
				unsigned char *middle_v = image->data( 2 ) + pitch_v * height / ( 4 * channels );
				for ( j = 0; j < channels; j ++ )
				{
					sample = int( ( double( ( buff + int( sample_offset * i ) * channels )[ j ] ) / 32769.0 ) * ( height / ( 2 * channels ) ) );
					p = middle_y - sample * pitch_y + i;
					line( p, middle_y, pitch_y, j % 2 == 0 ? ry : gy );
					sample /= 2;
					p = middle_u - sample * pitch_u + i / 2;
					line( p, middle_u, pitch_u, j % 2 == 0 ? ru : gu );
					p = middle_v - sample * pitch_v + i / 2;
					line( p, middle_v, pitch_v, j % 2 == 0 ? rv : gv );
					middle_y += pitch_y * height / channels;
					middle_u += pitch_u * height / ( 2 * channels );
					middle_v += pitch_v * height / ( 2 * channels );
				}
			}
		}


	private:
		pcos::property prop_force_;
		pcos::property prop_width_;
		pcos::property prop_height_;
		pcos::property prop_sar_num_;
		pcos::property prop_sar_den_;
		pcos::property prop_type_;
		pcos::property prop_hold_;
		pcos::property prop_colourspace_;
		pcos::property prop_scattered_;
		pcos::property prop_reverse_;
		il::image_type_ptr previous_;
		int previous_sar_num_;
		int previous_sar_den_;
};

// Playlist filter
//
// A simple n-ary filter - the number of possibly connected slots is
// defined by the 'slots' property. The number of frames reported by the filter
// is defined as the sum of the frames in all the connected slots. 
// 
// It is not this filters responsibility to ensure that all connected slots 
// have the same frame rate/sample information - that is left as an exercise
// for the filter graph builder to deal with.
//
// Properties:
//
// 	slots = n (default: 2)
// 		The maximum number of connected slots

class ML_PLUGIN_DECLSPEC playlist_filter : public filter_type
{
	public:
		playlist_filter( )
			: filter_type( )
			, prop_slots_( pcos::key::from_string( "slots" ) )
			, total_frames_( 0 )
		{
			properties( ).append( prop_slots_ = 2 );
		}

		// Indicates if the input will enforce a packet decode
		virtual bool requires_image( ) const { return false; }

		virtual const size_t slot_count( ) const { return size_t( prop_slots_.value< int >( ) ); }

		virtual const std::wstring get_uri( ) const { return L"playlist"; }

		virtual int get_frames( ) const 
		{
			return total_frames_;
		}

	protected:
		void do_fetch( frame_type_ptr &result )
		{
			size_t slot = slot_for_position( get_position( ) );
			input_type_ptr input = fetch_slot( slot );

			if ( input )
			{
				input->seek( get_position( ) - slot_offset( slot ) );
				result = input->fetch( );
				if ( result )
					result->set_position( get_position( ) );
			}
		}

		virtual void sync_frames( )
		{
			slots_.erase( slots_.begin( ), slots_.end( ) );
			total_frames_ = 0;
			for ( size_t i = 0; i < slot_count( ); i ++ )
			{
				total_frames_ += fetch_slot( i ) ? fetch_slot( i )->get_frames( ) : 0;
				slots_.push_back( total_frames_ );
			}
		}

	private:
		size_t slot_for_position( int position )
		{
			size_t result = 0;
			for ( size_t i = 0; i < slots_.size( ); i ++ )
			{
				if ( fetch_slot( i ) )
				{
					result = i;
					if ( position < slots_[ i ] ) break;
				}
			}
			return result;
		}

		int slot_offset( size_t slot )
		{
			return slot == 0 ? 0 : slots_[ slot - 1 ];
		}

		pcos::property prop_slots_;
		std::vector< int > slots_;
		int total_frames_;
};

//
// Plugin object
//

class ML_PLUGIN_DECLSPEC gensys_plugin : public openmedialib_plugin
{
public:
	virtual input_type_ptr input( const std::wstring &request )
	{
		if ( request == L"nothing:" )
			return input_type_ptr( );
		else if ( request == L"pusher:" )
			return input_type_ptr( new pusher_input( ) );
		else
			return input_type_ptr( new colour_input( ) );
	}

	virtual filter_type_ptr filter( const std::wstring &request )
	{
		if ( request == L"chroma" )
			return filter_type_ptr( new chroma_filter( ) );
		else if ( request == L"clip" )
			return filter_type_ptr( new clip_filter( ) );
		else if ( request == L"composite" )
			return filter_type_ptr( new composite_filter( ) );
		else if ( request == L"conform" )
			return filter_type_ptr( new conform_filter( ) );
		else if ( request == L"correction" )
			return filter_type_ptr( new correction_filter( ) );
		else if ( request == L"crop" )
			return filter_type_ptr( new crop_filter( ) );
		else if ( request == L"deinterlace" )
			return filter_type_ptr( new deinterlace_filter( ) );
		else if ( request == L"frame_rate" )
			return filter_type_ptr( new frame_rate_filter( ) );
		else if ( request == L"lerp" )
			return filter_type_ptr( new lerp_filter( ) );
		else if ( request == L"bezier" )
			return filter_type_ptr( new bezier_filter( ) );
		else if ( request == L"playlist" )
			return filter_type_ptr( new playlist_filter( ) );
		else if ( request == L"visualise" )
			return filter_type_ptr( new visualise_filter( ) );
		else if ( request == L"visualize" )
			return filter_type_ptr( new visualise_filter( ) );
		return filter_type_ptr( );
	}
};

} } }

//
// Access methods for openpluginlib
//

extern "C"
{
	ML_PLUGIN_DECLSPEC bool openplugin_init( void )
	{
		return true;
	}

	ML_PLUGIN_DECLSPEC bool openplugin_uninit( void )
	{
		return true;
	}
	
	ML_PLUGIN_DECLSPEC bool openplugin_create_plugin( const char*, pl::openplugin** plug )
	{
		*plug = new ml::gensys_plugin;
		return true;
	}
	
	ML_PLUGIN_DECLSPEC void openplugin_destroy_plugin( pl::openplugin* plug )
	{ 
		delete static_cast< ml::gensys_plugin * >( plug ); 
	}
}

#ifdef _MSC_VER
#	pragma warning ( pop )
#endif

