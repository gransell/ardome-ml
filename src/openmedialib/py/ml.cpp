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

class stream_handler_delegate : public ml::stream_handler, public py::wrapper< ml::stream_handler >
{
	public:
		stream_handler_delegate( ) : stream_handler( ) { }
		virtual ~stream_handler_delegate( ) { }

		virtual bool open( const std::wstring url, int flags )
		{
			bool result = false;
			py::override method = get_override( "open" );
			if ( method )
				result = py::call<bool>( method.ptr( ), url, flags );
			return result;
		}

		virtual std::string read( int size )
		{
			std::string result( "" );
			py::override method = get_override( "read" );
			if ( method )
				result = py::call< std::string >( method.ptr( ), size );
			return result;
		}

		virtual int write( const std::string &data )
		{
			int result = -1;
			py::override method = get_override( "write" );
			if ( method )
				result = py::call<int>( method.ptr( ), data );
			return result;
		}

		virtual long seek( long position, int whence )
		{
			long result = -1;
			py::override method = get_override( "seek" );
			if ( method )
				result = py::call<long>( method.ptr( ), position, whence );
			return result;
		}

		virtual int close( )
		{
			int result = -1;
			py::override method = get_override( "close" );
			if ( method )
				result = py::call<int>( method.ptr( ) );
			return result;
		}

		virtual bool is_stream( ) 
		{
			bool result = true;
			py::override method = get_override( "is_stream" );
			if ( method )
				result = py::call<bool>( method.ptr( ) );
			return result;
		}

		virtual bool is_thread_safe( ) 
		{ 
			bool result = true;
			py::override method = get_override( "is_thread_safe" );
			if ( method )
				result = py::call<bool>( method.ptr( ) ); 
			return result;
		}
};

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
		.def( "sample_size", &ml::audio::base::sample_size )
		.def( "af", &ml::audio::base::af, py::return_value_policy< py::return_by_value >( ) )
		.def( "position", &ml::audio::base::position )
		.def( "set_position", &ml::audio::base::set_position )
		.def( "size", &ml::audio::base::size )
	;
	py::def( "to_string", &audio_to_string );
}

void py_stream_type( )
{
	py::class_<ml::stream_type, boost::noncopyable, ml::stream_type_ptr>( "stream", py::no_init )
		.def( "length", &ml::stream_type::length )
		.def( "bytes", &ml::stream_type::bytes, py::return_value_policy< py::return_by_value >( ) )
	;
	py::def( "to_string", &stream_to_string );
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

audio_type_ptr audio_allocate0( const std::wstring &af, int frequency, int channels, int samples )
{
	return audio::allocate( af, frequency, channels, samples );
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

void py_plugin( )
{
	py::def( "init", &ml::init );
	py::def( "uninit", &ml::uninit );
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
	py::def( "frame_convert", &ml::frame_convert );
	py::def( "frame_rescale", &ml::frame_rescale );
	py::def( "frame_crop_clear", &ml::frame_crop_clear );
	py::def( "frame_crop", &ml::frame_crop );
	py::def( "frame_volume", &ml::frame_volume );
	py::def( "equals", &detail::is );
}

PyObject *stream_handler_object = 0;

static stream_handler_ptr stream_handler_callback( const std::wstring url, int flags )
{
	if ( stream_handler_object )
		return py::call< stream_handler_ptr >( stream_handler_object, url, flags );
	return stream_handler_ptr( );
}

static void stream_handler_register( PyObject *object )
{
	stream_handler_object = boost::ref( object );
}

void py_stream( )
{
	py::class_<ml::stream_handler_delegate, boost::noncopyable>( "stream_handler_delegate" )
		.def( "open", &ml::stream_handler_delegate::open )
		.def( "read", &ml::stream_handler_delegate::read )
		.def( "write", &ml::stream_handler_delegate::write )
		.def( "seek", &ml::stream_handler_delegate::seek )
		.def( "close", &ml::stream_handler_delegate::close )
		.def( "is_stream", &ml::stream_handler_delegate::is_stream )
		.def( "is_thread_safe", &ml::stream_handler_delegate::is_thread_safe )
	;

	py::register_ptr_to_python< boost::shared_ptr< stream_handler_delegate > >( );

	py::def( "stream_handler_register", &detail::stream_handler_register );

	ml::stream_handler_register( stream_handler_callback );
}

void py_stack( )
{
	void ( ml::stack::*push0 )( ml::input_type_ptr & )  = &ml::stack::push;
	void ( ml::stack::*push1 )( ml::filter_type_ptr & ) = &ml::stack::push;
	void ( ml::stack::*push2 )( std::string )           = &ml::stack::push;

	py::class_< ml::stack >( "stack" )
        .def( "push", push0 )
        .def( "push", push1 )
        .def( "push", push2 )
        .def( "pop", &ml::stack::pop )
    ;
}

} } } }
