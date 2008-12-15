// ml - A media library representation.

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#include <openpluginlib/py/python.hpp>
#include <openmedialib/ml/openmedialib_plugin.hpp>
#include <openmedialib/ml/frame.hpp>
#include <openmedialib/py/py.hpp>
#include <boost/python/ptr.hpp>
#include <boost/filesystem/path.hpp>
#include <map>

namespace fs  = boost::filesystem;
namespace ml  = olib::openmedialib;
namespace py  = boost::python;
namespace pl  = olib::openpluginlib;
namespace pcos= olib::openpluginlib::pcos;

namespace olib { namespace openmedialib { namespace ml { 

class input_delegate : public ml::input_type, public py::wrapper< ml::input_type >
{
	public:
		input_delegate( ) : input_type( ) { }
		virtual ~input_delegate( ) { }

		// Indicates if the input will enforce a packet decode
		virtual bool requires_image( ) const { return true; }

		virtual void reset( )
		{
			py::override method = get_override( "reset" );
			if ( method ) method( ); 
		}

		virtual const openpluginlib::wstring get_uri( ) const
		{
			py::override method = get_override( "get_uri" );
			if ( method ) return method( ); 
			return openpluginlib::wstring( L"" );
		}

		virtual const openpluginlib::wstring get_mime_type( ) const
		{
			py::override method = get_override( "get_mime_type" );
			if ( method ) return method( ); 
			return openpluginlib::wstring( L"" );
		}

		virtual int get_frames( ) const
		{
			py::override method = get_override( "get_frames" );
			if ( method ) return py::call<int>( method.ptr( ) ); 
			return 0;
		}

		virtual bool is_seekable( ) const
		{
			py::override method = get_override( "is_seekable" );
			if ( method ) return py::call<bool>( method.ptr( ) ); 
			return false;
		}

		virtual int get_video_streams( ) const
		{
			py::override method = get_override( "get_video_streams" );
			if ( method ) return py::call<int>( method.ptr( ) ); 
			return 0;
		}

		virtual int get_audio_streams( ) const
		{
			py::override method = get_override( "get_audio_streams" );
			if ( method ) return py::call<int>( method.ptr( ) ); 
			return 0;
		}

		virtual bool set_video_stream( const int index ) 
		{ 
			py::override method = get_override( "set_video_streams" );
			if ( method ) return py::call<bool>( method.ptr( ), index ); 
			return false; 
		}

		virtual bool set_audio_stream( const int index ) 
		{ 
			py::override method = get_override( "set_audio_streams" );
			if ( method ) return py::call<bool>( method.ptr( ), index ); 
			return false; 
		}

		virtual void seek( const int position, const bool relative = false )
		{
			py::override method = get_override( "seek" );
			if ( method ) method( position, relative ); 
			else input_type::seek( position, relative ); 
		}

		virtual int get_position( ) const
		{
			py::override method = get_override( "get_position" );
			if ( method ) return py::call<int>( method.ptr( ) ); 
			return input_type::get_position( ); 
		}

		virtual bool push( frame_type_ptr frame )
		{
			py::override method = get_override( "push" );
			if ( method ) return py::call<bool>( method.ptr( ), frame ); 
			return false;
		}

		virtual bool reuse( ) 
		{
			py::override method = get_override( "reuse" );
			if ( method ) return py::call<bool>( method.ptr( ) ); 
			return false;
		}

		virtual bool is_thread_safe( ) const
		{
			return false;
		}

	protected:
		virtual void do_fetch( frame_type_ptr &result  )
		{
			py::override method = get_override( "fetch" );
			if ( method ) method( result ); 
		}

};

class filter_delegate : public ml::filter_type, public py::wrapper< ml::filter_type >
{
	public:
		filter_delegate( ) : filter_type( ) { }
		virtual ~filter_delegate( ) { }

		// Indicates if the input will enforce a packet decode
		virtual bool requires_image( ) const { return true; }

		virtual void reset( )
		{
			py::override method = get_override( "reset" );
			if ( method ) method( ); 
		}

		virtual const openpluginlib::wstring get_uri( ) const
		{
			py::override method = get_override( "get_uri" );
			if ( method ) return method( ); 
			return openpluginlib::wstring( L"" );
		}

		virtual const size_t slot_count( ) const
		{
			py::override method = get_override( "slot_count" );
			if ( method ) return py::call<const size_t>( method.ptr( ) ); 
			return 1;
		}

		virtual int get_frames( ) const
		{
			py::override method = get_override( "get_frames" );
			if ( method ) return py::call<int>( method.ptr( ) ); 
			return 0;
		}

		virtual void seek( const int position, const bool relative = false )
		{
			py::override method = get_override( "seek" );
			if ( method ) method( position, relative ); 
			else input_type::seek( position, relative ); 
		}

		virtual int get_position( ) const
		{
			py::override method = get_override( "get_position" );
			if ( method ) return py::call<int>( method.ptr( ) );  
			return input_type::get_position( ); 
		}

		virtual void on_slot_change( input_type_ptr input, int slot )
		{
			py::override method = get_override( "on_slot_change" );
			if ( method ) method( input, slot ); 
		}

		virtual bool reuse( ) 
		{
			py::override method = get_override( "reuse" );
			if ( method ) return py::call<bool>( method.ptr( ) ); 
			return false;
		}

		virtual bool is_thread_safe( ) const
		{
			return false;
		}

	protected:
		virtual void do_fetch( frame_type_ptr &result  )
		{
			py::override method = get_override( "fetch" );
			if ( method ) method( result ); 
		}
};

class store_delegate : public ml::store_type, public py::wrapper< ml::store_type >
{
	public:
		store_delegate( ) : store_type( ) { }
		virtual ~store_delegate( ) { }

		virtual bool init( ) 
		{ 
			py::override method = get_override( "init" );
			if ( method ) return py::call<bool>( method.ptr( ) );
			return true;
		}

		virtual bool push( frame_type_ptr frame )
		{
			py::override method = get_override( "push" );
			if ( method ) return py::call<bool>( method.ptr( ), frame );
			return false;
		}

		virtual frame_type_ptr flush( ) 
		{ 
			py::override method = get_override( "flush" );
			if ( method ) return method( );
			return frame_type_ptr( ); 
		}

		virtual void complete( ) 
		{ 
			py::override method = get_override( "complete" );
			if ( method ) method( );
		}

		virtual bool empty( ) 
		{
			py::override method = get_override( "empty" );
			if ( method ) return py::call<bool>( method.ptr( ) );
			return false; 
		}
};

class callback_delegate : public ml::input_callback, public py::wrapper< ml::input_callback >
{
	public:
		callback_delegate( ) : input_callback( ) { }
		virtual ~callback_delegate( ) { }

		void assign( pcos::property_container properties, int position )
		{
			py::override method = get_override( "assign" );
			if ( method ) method( properties, position );
		}
};

class stream_handler_delegate : public ml::stream_handler, public py::wrapper< ml::stream_handler >
{
	public:
		stream_handler_delegate( ) : stream_handler( ) { }
		virtual ~stream_handler_delegate( ) { }

		virtual bool open( const pl::wstring url, int flags )
		{
			bool result = false;
			py::override method = get_override( "open" );
			if ( method )
				result = py::call<bool>( method.ptr( ), url, flags );
			return result;
		}

		virtual pl::string read( int size )
		{
			pl::string result( "" );
			py::override method = get_override( "read" );
			if ( method )
				result = py::call< pl::string >( method.ptr( ), size );
			return result;
		}

		virtual int write( const pl::string &data )
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

void py_audio( )
{
	py::class_<ml::audio_type, boost::noncopyable, ml::audio_type_ptr>( "audio", py::no_init )
		.def( "frequency", &ml::audio_type::frequency )
		.def( "channels", &ml::audio_type::channels )
		.def( "samples", &ml::audio_type::samples )
		.def( "af", &ml::audio_type::af )
		.def( "pts", &ml::audio_type::pts )
		.def( "set_pts", &ml::audio_type::set_pts )
		.def( "position", &ml::audio_type::position )
		.def( "set_position", &ml::audio_type::set_position )
		.def( "size", &ml::audio_type::size )
		.def( "is_cropped", &ml::audio_type::is_cropped )
		.def( "crop_clear", &ml::audio_type::crop_clear )
		.def( "crop", &ml::audio_type::crop )
		.def( "get_som", &ml::audio_type::get_som )
		.def( "get_eom", &ml::audio_type::get_eom )
	;
}

void py_frame( )
{
	py::class_<ml::frame_type, boost::noncopyable, ml::frame_type_ptr>( "frame", py::no_init )
		.def( "shallow_copy", &ml::frame_type::shallow_copy, py::return_value_policy< py::return_by_value >( ) )
		.def( "deep_copy", &ml::frame_type::deep_copy, py::return_value_policy< py::return_by_value >( ) )
		.def( "properties", &ml::frame_type::properties, py::return_value_policy< py::return_by_value >( ) )
		.def( "property", &ml::frame_type::property, py::return_value_policy< py::return_by_value >( ) )
		.def( "set_alpha", &ml::frame_type::set_alpha )
		.def( "get_alpha", &ml::frame_type::get_alpha )
		.def( "set_image", &ml::frame_type::set_image )
		.def( "get_image", &ml::frame_type::get_image )
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
		.def( "get_sar_num", &ml::frame_type::get_sar_num )
		.def( "get_sar_den", &ml::frame_type::get_sar_den )
		.def( "get_fps_num", &ml::frame_type::get_fps_num )
		.def( "get_fps_den", &ml::frame_type::get_fps_den )
		.def( "aspect_ratio", &ml::frame_type::aspect_ratio )
		.def( "in_error", &ml::frame_type::in_error )
	;
}

void py_input( )
{
	py::class_<ml::callback_delegate, boost::noncopyable>( "callback_delegate" )
		.def( "assign", &ml::callback_delegate::assign )
	;

	py::register_ptr_to_python< boost::shared_ptr< callback_delegate > >( );

	py::enum_<ml::process_flags>( "process_flags" )
		.value( "image", ml::process_image )
		.value( "audio", ml::process_audio )
	;

	py::class_<ml::input_delegate, boost::noncopyable>( "input_delegate" )
		.def( "slots", &ml::input_type::slot_count )
		.def( "reset", &ml::input_type::reset )
		.def( "connect", &ml::input_type::connect )
		.def( "register", &ml::input_type::register_callback )
		.def( "properties", &ml::input_type::properties, py::return_value_policy< py::return_by_value >( ) )
		.def( "property", &ml::input_type::property, py::return_value_policy< py::return_by_value >( ) )
		.def( "get_uri", &ml::input_delegate::get_uri )
		.def( "get_mime_type", &ml::input_delegate::get_mime_type )
		.def( "get_frames", &ml::input_delegate::get_frames )
		.def( "is_seekable", &ml::input_delegate::is_seekable )
		.def( "get_video_streams", &ml::input_delegate::get_video_streams )
		.def( "get_audio_streams", &ml::input_delegate::get_audio_streams )
		.def( "set_video_stream", &ml::input_delegate::set_video_stream )
		.def( "set_audio_stream", &ml::input_delegate::set_audio_stream )
		.def( "set_process_flags", &ml::input_delegate::set_process_flags )
		.def( "get_process_flags", &ml::input_delegate::get_process_flags )
		.def( "seek", &ml::input_delegate::seek )
		.def( "get_position", &ml::input_delegate::get_position )
		.def( "push", &ml::input_delegate::push )
		.def( "fetch", &ml::input_delegate::fetch )
		.def( "fetch_callback", &ml::input_delegate::fetch_callback )
		.def( "reuse", &ml::input_delegate::reuse )
		.def( "is_thread_safe", &ml::input_delegate::is_thread_safe )
	;

	py::register_ptr_to_python< boost::shared_ptr< input_delegate > >( );

	py::class_<ml::input_type, boost::noncopyable, ml::input_type_ptr>( "input", py::no_init )
		.def( "init", &ml::input_type::init )
		.def( "slots", &ml::input_type::slot_count )
		.def( "reset", &ml::input_type::reset )
		.def( "connect", &ml::input_type::connect )
		.def( "register", &ml::input_type::register_callback )
		.def( "properties", &ml::input_type::properties, py::return_value_policy< py::return_by_value >( ) )
		.def( "property", &ml::input_type::property, py::return_value_policy< py::return_by_value >( ) )
		.def( "get_uri", &ml::input_type::get_uri )
		.def( "get_mime_type", &ml::input_type::get_mime_type )
		.def( "get_frames", &ml::input_type::get_frames )
		.def( "is_seekable", &ml::input_type::is_seekable )
		.def( "get_video_streams", &ml::input_type::get_video_streams )
		.def( "get_audio_streams", &ml::input_type::get_audio_streams )
		.def( "set_video_stream", &ml::input_type::set_video_stream )
		.def( "set_audio_stream", &ml::input_type::set_audio_stream )
		.def( "set_process_flags", &ml::input_type::set_process_flags )
		.def( "get_process_flags", &ml::input_type::get_process_flags )
		.def( "seek", &ml::input_type::seek )
		.def( "get_position", &ml::input_type::get_position )
		.def( "push", &ml::input_type::push )
		.def( "fetch", &ml::input_type::fetch )
		.def( "fetch_callback", &ml::input_type::fetch_callback )
		.def( "reuse", &ml::input_type::reuse )
		.def( "fetch_slot", &ml::input_type::fetch_slot )
		.def( "is_thread_safe", &ml::input_type::is_thread_safe )
	;
}

void py_store( )
{
	py::class_<ml::store_delegate, boost::noncopyable>( "store_delegate" )
		.def( "properties", &ml::store_delegate::properties, py::return_value_policy< py::return_by_value >( ) )
		.def( "property", &ml::store_delegate::property, py::return_value_policy< py::return_by_value >( ) )
		.def( "init", &ml::store_delegate::init )
		.def( "push", &ml::store_delegate::push )
		.def( "complete", &ml::store_delegate::complete )
		.def( "flush", &ml::store_delegate::flush )
		.def( "empty", &ml::store_delegate::empty )
	;

	py::register_ptr_to_python< boost::shared_ptr< store_delegate > >( );

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
	py::class_<ml::filter_delegate, boost::noncopyable>( "filter_delegate" )
		.def( "slots", &ml::filter_delegate::slot_count )
		.def( "fetch", &ml::filter_delegate::fetch )
		.def( "reset", &ml::filter_delegate::reset )
		.def( "connect", &ml::filter_delegate::connect )
		.def( "register", &ml::filter_delegate::register_callback )
		.def( "properties", &ml::filter_delegate::properties, py::return_value_policy< py::return_by_value >( ) )
		.def( "property", &ml::filter_delegate::property, py::return_value_policy< py::return_by_value >( ) )
		.def( "get_uri", &ml::filter_delegate::get_uri )
		.def( "get_mime_type", &ml::filter_delegate::get_mime_type )
		.def( "get_frames", &ml::filter_delegate::get_frames )
		.def( "is_seekable", &ml::filter_delegate::is_seekable )
		.def( "get_video_streams", &ml::filter_delegate::get_video_streams )
		.def( "get_audio_streams", &ml::filter_delegate::get_audio_streams )
		.def( "set_video_stream", &ml::filter_delegate::set_video_stream )
		.def( "set_audio_stream", &ml::filter_delegate::set_audio_stream )
		.def( "set_process_flags", &ml::filter_delegate::set_process_flags )
		.def( "get_process_flags", &ml::filter_delegate::get_process_flags )
		.def( "seek", &ml::filter_delegate::seek )
		.def( "get_position", &ml::filter_delegate::get_position )
		.def( "acquire_values", &ml::filter_delegate::acquire_values )
		.def( "on_slot_change", &ml::filter_delegate::on_slot_change )
		.def( "fetch_slot", &ml::filter_delegate::fetch_slot )
		.def( "reuse", &ml::filter_delegate::reuse )
	;

	py::register_ptr_to_python< boost::shared_ptr< filter_delegate > >( );

	py::class_<ml::filter_type, boost::noncopyable, ml::filter_type_ptr, py::bases< input_type > >( "filter", py::no_init )
		.def( "connect", &ml::filter_type::connect )
		.def( "fetch_slot", &ml::filter_type::fetch_slot )
		.def( "reuse", &ml::filter_type::reuse )
	;
}

void py_audio_reseat( )
{
	py::class_<ml::audio_reseat, boost::noncopyable, ml::audio_reseat_ptr>( "audio_reseat", py::no_init )
		.def( "append", &ml::audio_reseat::append )
		.def( "retrieve", &ml::audio_reseat::retrieve )
		.def( "clear", &ml::audio_reseat::clear )
		.def( "has", &ml::audio_reseat::has )
	;
}

ml::input_type_ptr create_input0( const pl::wstring &resource )
{
	ml::input_type_ptr input = ml::create_input( resource );
	if ( input )
		input->report_exceptions( false );
	return input;
}

ml::input_type_ptr create_input( const std::string &resource )
{
	ml::input_type_ptr input = ml::create_input( pl::to_wstring( resource ) );
	if ( input )
		input->report_exceptions( false );
	return input;
}

ml::store_type_ptr create_store( const std::string &resource, const ml::frame_type_ptr &frame )
{
	return ml::create_store( pl::to_wstring( resource ), frame );
}

ml::filter_type_ptr create_filter( const std::string &resource )
{
	ml::filter_type_ptr filter = ml::create_filter( pl::to_wstring( resource ) );
	if ( filter )
		filter->report_exceptions( false );
	return filter;
}

void py_plugin( )
{
	py::def( "create_input", &detail::create_input0 );
	py::def( "create_input", &detail::create_input );
	py::def( "create_store", &detail::create_store );
	py::def( "create_filter", &detail::create_filter );
	py::def( "audio_resample", &ml::audio_resample );
	py::def( "audio_samples_for_frame", &ml::audio_samples_for_frame );
	py::def( "audio_samples_to_frame", &ml::audio_samples_to_frame );
	py::def( "create_audio_reseat", &ml::create_audio_reseat );
	py::def( "audio_mix", &ml::audio_mix );
	py::def( "audio_channel_convert", &ml::audio_channel_convert );
	py::def( "frame_convert", &ml::frame_convert );
	py::def( "frame_rescale", &ml::frame_rescale );
	py::def( "frame_crop_clear", &ml::frame_crop_clear );
	py::def( "frame_crop", &ml::frame_crop );
	py::def( "frame_volume", &ml::frame_volume );
}

PyObject *stream_handler_object = 0;

static stream_handler_ptr stream_handler_callback( const opl::wstring url, int flags )
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

} } } }
