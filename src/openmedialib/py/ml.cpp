// ml - A media library representation.

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#include <openpluginlib/py/python.hpp>
#include <openmedialib/ml/openmedialib_plugin.hpp>
#include <openmedialib/ml/frame.hpp>
#include <openmedialib/ml/stack.hpp>
#include <openmedialib/py/py.hpp>
#include <boost/python/ptr.hpp>
#include <boost/filesystem/path.hpp>
#include <map>

namespace fs  = boost::filesystem;
namespace ml  = olib::openmedialib;
namespace py  = boost::python;
namespace pl  = olib::openpluginlib;
namespace pcos= olib::openpluginlib::pcos;
namespace cl  = olib::opencorelib;

namespace olib { namespace openmedialib { namespace ml { 

BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS( input_fetch_overloads, fetch, 0, 1 );
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS( input_seek_overloads, seek, 1, 2 );

namespace detail {

static std::string stream_to_string( ml::stream_type_ptr stream )
{
	std::string result;

	if ( stream )
		result = std::string( ( char * )stream->bytes( ), stream->length( ) );

	return result;
}

static std::string audio_to_string( ml::audio_type_ptr audio )
{
	std::string result;

	if ( audio )
		result = std::string( ( char * )audio->pointer( ), audio->size( ) );

	return result;
}

void py_audio( )
{
	py::class_<ml::audio::base, boost::noncopyable, ml::audio_type_ptr>( "audio", py::no_init )
		.def( "frequency", &ml::audio::base::frequency )
		.def( "channels", &ml::audio::base::channels )
		.def( "samples", &ml::audio::base::samples )
		.def( "sample_storage_size", &ml::audio::base::sample_storage_size )
		.def( "af", &ml::audio::base::af, py::return_value_policy< py::return_by_value >( ) )
		.def( "position", &ml::audio::base::position )
		.def( "set_position", &ml::audio::base::set_position )
		.def( "size", &ml::audio::base::size )
	;
	py::def( "to_string", &audio_to_string );
}

void py_stream( )
{
	py::class_<ml::stream_type, boost::noncopyable, ml::stream_type_ptr>( "stream", py::no_init )
		.def( "length", &ml::stream_type::length )
		.def( "position", &ml::stream_type::position )
		.def( "bytes", &ml::stream_type::bytes, py::return_value_policy< py::return_by_value >( ) )
	;
	py::def( "to_string", &stream_to_string );
}

BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS( image_width_overloads, width, 0, 2 );
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS( image_height_overloads, height, 0, 2 );
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS( image_pitch_overloads, pitch, 0, 2 );
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS( image_offset_overloads, offset, 0, 2 );
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS( image_linesize_overloads, linesize, 0, 2 );

static std::string image_to_string( ml::image_type_ptr image )
{
	std::string result;

	if ( image )
	{
		int used = 0;

		// Calculate byte size for the full image (all planes)
		for ( int p = 0; p < image->plane_count( ); p ++ )
			used += image->linesize( p ) * image->height( p );

		// Allocate the raster
		boost::uint8_t *dst = new boost::uint8_t[ used ];
		boost::uint8_t *data = dst;

		if ( dst )
		{
			// Copy each plane into the raster
			for ( int p = 0; p < image->plane_count( ); p ++ )
			{
				boost::uint8_t *src = static_cast< boost::uint8_t * >( image->ptr( p ) );
				int height = image->height( p );

				while( height -- )
				{
					memcpy( dst, src, image->linesize( p ) );
					dst += image->linesize( p );
					src += image->pitch( p );
				}
			}

			// Construct the result
			result = std::string( ( char * )data, used );
		}

		delete [] data;
	}

	return result;
}

int field_order( ml::image_type_ptr image )
{   
	return int( image->field_order( ) );
}

void set_field_order( ml::image_type_ptr image, int order )
{   
	image->set_field_order( ml::image::field_order_flags( order ) );
}

void py_image( )
{
	py::class_<ml::image::image, boost::noncopyable, ml::image_type_ptr>( "image", py::no_init )
		.def( "matching", &ml::image::image::matching )
		.def( "plane_count", &ml::image::image::plane_count )
		.def( "width", &ml::image::image::width, image_width_overloads( py::args( "index", "crop" ), "width" ) )
		.def( "height", &ml::image::image::height, image_height_overloads( py::args( "index", "crop" ), "height" ) )
		.def( "pitch", &ml::image::image::pitch, image_pitch_overloads( py::args( "index", "crop" ), "pitch" ) )
		.def( "offset", &ml::image::image::offset, image_offset_overloads( py::args( "index", "crop" ), "offset" ) )
		.def( "linesize", &ml::image::image::linesize, image_linesize_overloads( py::args( "index", "crop" ), "linesize" ) )
		.def( "get_crop_x", &ml::image::image::get_crop_x )
		.def( "get_crop_y", &ml::image::image::get_crop_y )
		.def( "get_crop_w", &ml::image::image::get_crop_w )
		.def( "get_crop_h", &ml::image::image::get_crop_h )
		.def( "is_writable", &ml::image::image::is_writable )
		.def( "set_writable", &ml::image::image::set_writable )
		.def( "position", &ml::image::image::position )
		.def( "set_position", &ml::image::image::set_position )
		.def( "field_order", &field_order )
		.def( "set_field_order", &set_field_order )
		.def( "pf", &ml::image::image::pf, py::return_value_policy< py::return_by_value >( ) )
		.def( "size", &ml::image::image::size )
		.def( "crop", &ml::image::image::crop )
		.def( "crop_clear", &ml::image::image::crop_clear )
		;

	py::def( "to_string", &image_to_string );
}

ml::image_type_ptr rescale( ml::image_type_ptr image, int width, int height )
{
	return ml::image::rescale( image, width, height, ml::image::BILINEAR_SAMPLING );
}

ml::image_type_ptr null_image( )
{
	return ml::image_type_ptr( );
}

ml::image_type_ptr allocate( const olib::t_string pf, int width, int height )
{
	return ml::image::allocate( pf, width, height );
}

ml::image_type_ptr convert( const image_type_ptr &src, const olib::t_string& pf )
{
	return ml::image::convert( src, pf );
}

void py_utility( )
{
	py::def( "conform", &ml::image::conform );
	py::def( "convert", &convert );
	py::def( "allocate", &allocate );
	py::def( "field", &ml::image::field );
	py::def( "deinterlace", &ml::image::deinterlace );
	py::def( "rescale", &rescale );
	py::def( "null_image", &null_image );
}

void py_frame( )
{
	py::class_<ml::frame_type, boost::noncopyable, ml::frame_type_ptr>( "frame", py::no_init )
		.def( "shallow", &ml::frame_type::shallow, py::return_value_policy< py::return_by_value >( ) )
		.def( "deep", &ml::frame_type::deep, py::return_value_policy< py::return_by_value >( ) )
		.def( "properties", &ml::frame_type::properties, py::return_value_policy< py::return_by_value >( ) )
		.def( "property", &ml::frame_type::property, py::return_value_policy< py::return_by_value >( ) )
		.def( "property_with_key", &ml::frame_type::property_with_key, py::return_value_policy< py::return_by_value >( ) )
		.def( "set_alpha", &ml::frame_type::set_alpha )
		.def( "get_alpha", &ml::frame_type::get_alpha )
		.def( "set_image", &ml::frame_type::set_image )
		.def( "get_image", &ml::frame_type::get_image )
		.def( "set_stream", &ml::frame_type::set_stream )
		.def( "get_stream", &ml::frame_type::get_stream )
		.def( "set_audio", &ml::frame_type::set_audio )
		.def( "get_audio", &ml::frame_type::get_audio )
		.def( "set_pts", &ml::frame_type::set_pts )
		.def( "get_pts", &ml::frame_type::get_pts )
		.def( "set_position", &ml::frame_type::set_position )
		.def( "get_position", &ml::frame_type::get_position )
		.def( "set_duration", &ml::frame_type::set_duration )
		.def( "get_duration", &ml::frame_type::get_duration )
		.def( "set_sar", &ml::frame_type::set_sar )
		.def( "get_sar", &ml::frame_type::get_sar )
		.def( "set_fps", &ml::frame_type::set_fps )
		.def( "get_fps", &ml::frame_type::get_fps )
		.def( "fps", &ml::frame_type::fps )
		.def( "get_sar_num", &ml::frame_type::get_sar_num )
		.def( "get_sar_den", &ml::frame_type::get_sar_den )
		.def( "get_fps_num", &ml::frame_type::get_fps_num )
		.def( "get_fps_den", &ml::frame_type::get_fps_den )
		.def( "aspect_ratio", &ml::frame_type::aspect_ratio )
		.def( "in_error", &ml::frame_type::in_error )
		.def( "width", &ml::frame_type::width )
		.def( "height", &ml::frame_type::height )
	;
}

void py_input( )
{
	py::enum_<ml::process_flags>( "process_flags" )
		.value( "image", ml::process_image )
		.value( "audio", ml::process_audio )
	;

	py::class_<ml::input_type, boost::noncopyable, ml::input_type_ptr>( "input", py::no_init )
		.def( "init", &ml::input_type::init )
		.def( "slots", &ml::input_type::slot_count )
		.def( "reset", &ml::input_type::reset )
		.def( "connect", &ml::input_type::connect )
		.def( "properties", &ml::input_type::properties, py::return_value_policy< py::return_by_value >( ) )
		.def( "property", &ml::input_type::property, py::return_value_policy< py::return_by_value >( ) )
		.def( "property_with_key", &ml::input_type::property_with_key, py::return_value_policy< py::return_by_value >( ) )
		.def( "get_uri", &ml::input_type::get_uri )
		.def( "get_mime_type", &ml::input_type::get_mime_type )
		.def( "get_frames", &ml::input_type::get_frames )
		.def( "is_seekable", &ml::input_type::is_seekable )
		.def( "set_process_flags", &ml::input_type::set_process_flags )
		.def( "get_process_flags", &ml::input_type::get_process_flags )
		.def( "seek", &ml::input_type::seek, input_seek_overloads( ) )
		.def( "get_position", &ml::input_type::get_position )
		.def( "push", &ml::input_type::push )
		.def( "sync", &ml::input_type::sync )
		.def( "fetch", ( ml::frame_type_ptr( input_type::* )( int ) )0, input_fetch_overloads( ) )
		.def( "fetch_slot", &ml::input_type::fetch_slot )
		.def( "is_thread_safe", &ml::input_type::is_thread_safe )
		.def( "requires_image", &ml::input_type::requires_image )
	;
}

void py_store( )
{
	py::class_<ml::store_type, boost::noncopyable, ml::store_type_ptr>( "store", py::no_init )
		.def( "properties", &ml::store_type::properties, py::return_value_policy< py::return_by_value >( ) )
		.def( "property", &ml::store_type::property, py::return_value_policy< py::return_by_value >( ) )
		.def( "init", &ml::store_type::init )
		.def( "push", &ml::store_type::push )
		.def( "flush", &ml::store_type::flush )
		.def( "complete", &ml::store_type::complete )
		.def( "empty", &ml::store_type::empty )
	;
}

void py_filter( )
{
	py::class_<ml::filter_type, boost::noncopyable, ml::filter_type_ptr, py::bases< input_type > >( "filter", py::no_init )
		.def( "connect", &ml::filter_type::connect )
		.def( "fetch_slot", &ml::filter_type::fetch_slot )
	;
}

void py_audio_reseat( )
{
	py::class_<ml::audio::reseat, boost::noncopyable, ml::audio::reseat_ptr>( "audio_reseat", py::no_init )
		.def( "append", &ml::audio::reseat::append )
		.def( "retrieve", &ml::audio::reseat::retrieve )
		.def( "clear", &ml::audio::reseat::clear )
		.def( "has", &ml::audio::reseat::has )
	;
}

ml::input_type_ptr create_input0( const std::wstring &resource )
{
	ml::input_type_ptr input = ml::create_input( resource );
	if ( input )
		input->report_exceptions( false );
	return input;
}

ml::input_type_ptr create_input( const std::string &resource )
{
	ml::input_type_ptr input = ml::create_input( cl::str_util::to_wstring( resource ) );
	if ( input )
		input->report_exceptions( false );
	return input;
}

ml::store_type_ptr create_store( const std::string &resource, const ml::frame_type_ptr &frame )
{
	return ml::create_store( cl::str_util::to_wstring( resource ), frame );
}

ml::filter_type_ptr create_filter( const std::string &resource )
{
	ml::filter_type_ptr filter = ml::create_filter( cl::str_util::to_wstring( resource ) );
	if ( filter )
		filter->report_exceptions( false );
	return filter;
}

bool is( ml::input_type_ptr a, ml::input_type_ptr b )
{
	return a == b;
}

audio_type_ptr audio_allocate0( const olib::t_string &af, int frequency, int channels, int samples )
{
	return audio::allocate( audio::af_to_id( af ), frequency, channels, samples );
}

audio_type_ptr audio_allocate1( const audio_type_ptr &audio, int frequency, int channels, int samples )
{
	return audio::allocate( audio, frequency, channels, samples );
}

int audio_samples_for_frame( int frame, int frequency, int fps_num, int fps_den )
{
	return ml::audio::samples_for_frame( frame, frequency, fps_num, fps_den );
}

int audio_samples_to_frame( int frame, int frequency, int fps_num, int fps_den )
{
	return ml::audio::samples_to_frame( frame, frequency, fps_num, fps_den );
}

frame_type_ptr frame_convert0( frame_type_ptr frame, const olib::t_string &pf )
{
	return ml::frame_convert( ml::rescale_object_ptr(), frame, pf );
}

frame_type_ptr frame_convert1( ml::rescale_object_ptr ro, frame_type_ptr frame, const olib::t_string &pf )
{
	return ml::frame_convert( ro, frame, pf );
}

frame_type_ptr frame_rescale( frame_type_ptr frame, int width, int height, ml::image::rescale_filter filter = ml::image::BICUBIC_SAMPLING )
{
	return ml::frame_rescale( frame, width, height, filter );
}

void py_plugin( )
{
	py::def( "init", &ml::init );
	py::def( "uninit", &ml::uninit );
	py::def( "create_delayed_input", &ml::create_delayed_input );
	py::def( "create_input", &detail::create_input0 );
	py::def( "create_input", &detail::create_input );
	py::def( "create_store", &detail::create_store );
	py::def( "create_filter", &detail::create_filter );
	py::def( "audio_resample", &ml::audio_resample );
	py::def( "audio_allocate", &detail::audio_allocate0 );
	py::def( "audio_allocate", &detail::audio_allocate1 );
	py::def( "audio_channel_convert", &ml::audio::channel_convert );
	py::def( "audio_channel_extract", &ml::audio::channel_extract );
	py::def( "audio_channel_extract_count", &ml::audio::channel_extract_count );
	py::def( "audio_mixer", &ml::audio::mixer );
	py::def( "audio_pitch", &ml::audio::pitch );
	py::def( "audio_reverse", &ml::audio::reverse );
	py::def( "audio_volume", &ml::audio::volume );
	py::def( "audio_samples_for_frame", &detail::audio_samples_for_frame );
	py::def( "audio_samples_to_frame", &detail::audio_samples_to_frame );
	py::def( "create_audio_reseat", &ml::audio::create_reseat );
	py::def( "audio_mix", &ml::audio::mixer );
	py::def( "audio_channel_convert", &ml::audio::channel_convert );
	py::def( "frame_convert", &detail::frame_convert0 );
	py::def( "frame_convert", &detail::frame_convert1 );
	py::def( "frame_rescale", &frame_rescale );
	py::def( "frame_crop_clear", &ml::frame_crop_clear );
	py::def( "frame_crop", &ml::frame_crop );
	py::def( "frame_volume", &ml::frame_volume );
	py::def( "equals", &detail::is );
}

void py_stack( )
{
	ml::stack &( ml::stack::*push0 )( ml::input_type_ptr ) = &ml::stack::push;
	ml::stack &( ml::stack::*push1 )( std::string )        = &ml::stack::push;

	py::class_< ml::stack >( "stack" )
        .def( "push", push0, py::return_value_policy< py::reference_existing_object >( )  )
        .def( "push", push1, py::return_value_policy< py::reference_existing_object >( )  )
        .def( "pop", &ml::stack::pop )
        .def( "release", &ml::stack::release )
    ;
}

} } } }
