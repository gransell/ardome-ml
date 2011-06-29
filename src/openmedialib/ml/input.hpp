
// ml - A media library representation.

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#ifndef OPENMEDIALIB_INPUT_INC_
#define OPENMEDIALIB_INPUT_INC_

#include <openmedialib/ml/types.hpp>
#include <openmedialib/ml/frame.hpp>

#include <openpluginlib/pl/pcos/property_container.hpp>
#include <openpluginlib/pl/log.hpp>
#include <openpluginlib/pl/utf8_utils.hpp>

#include <opencorelib/cl/core.hpp>
#include <opencorelib/cl/base_exception.hpp>
#include <opencorelib/cl/utilities.hpp>
#include <opencorelib/cl/log_defines.hpp>

#include <boost/enable_shared_from_this.hpp>

namespace olib { namespace openmedialib { namespace ml {

// Enumerated type to allow the user of the input to restrict the scope of the 
// fetch - for example, if you only need audio, there is no need to process the
// associated image.

typedef enum
{
	process_image = 0x1,
	process_audio = 0x2,
	process_next_intra = 0x4
}
process_flags;

// The input abstract class - implementations will extend this class and (minimally)
// provide the pure virtual methods.

class ML_DECLSPEC input_type : public boost::enable_shared_from_this< input_type >
{
	public:
		// Constructor/destructor
		explicit input_type( )
			: properties_( )
			, initialized_( false )
			, prop_debug_( olib::openpluginlib::pcos::key::from_string( "debug" ) )
			, position_( 0 )
			, process_( process_image | process_audio )
			, report_exceptions_( true )
		{
			properties( ).append( prop_debug_ = 0 );
		}

		virtual ~input_type( ) { }

		// Provides a mechanism for defering initialisation from the ctor
		bool init( ) 
		{ 
			if ( !initialized_ ) 
			{
				initialized_ = initialize( ); 
				if ( !initialized_ )
				{
					ARLOG_ERR( "Initialisation of %1% failed" )( get_uri( ) );
				}
			}
			return initialized_; 
		}

		const bool initialized( ) const { return initialized_; }

		// Indicates if the input or filter will enforce an packet decode
		virtual bool requires_image( ) const = 0;

		// Work around for bugs in python
		void report_exceptions( bool value )
		{ report_exceptions_ = value; }

		bool reporting_exceptions( ) const
		{ return report_exceptions_; }

		// Filters reimplement these
		virtual const size_t slot_count( ) const { return 0; }
		virtual bool connect( input_type_ptr, size_t = 0 ) { return false; }
		virtual input_type_ptr fetch_slot( size_t = 0 ) const { return input_type_ptr( ); }

		// Property object
		olib::openpluginlib::pcos::property_container properties( ) const 
		{ return properties_; }

		// Convenience method
		olib::openpluginlib::pcos::property property( const char *name ) const
		{ return properties_.get_property_with_string( name ); }

		// Convenience method
		olib::openpluginlib::pcos::property property_with_key( const olib::openpluginlib::pcos::key &name ) const
		{ return properties_.get_property_with_key( name ); }

		// Virtual method for state reset
		virtual void reset( ) { }

		// Basic information
		virtual const openpluginlib::wstring get_uri( ) const = 0;
		virtual const openpluginlib::wstring get_mime_type( ) const = 0;

		// Audio/Visual
		virtual void sync( )
		{
			for ( size_t i = 0; i < slot_count( ); i ++ )
				if ( fetch_slot( i ) )
					fetch_slot( i )->sync( );
			sync_frames( );
		}

		virtual int get_frames( ) const = 0;
		virtual bool is_seekable( ) const = 0;

		// Visual
		virtual int get_video_streams( ) const = 0;

		// Audio
		virtual int get_audio_streams( ) const = 0;

		// Returns the number of audio channel in the specified stream.
		// The index should be between 0 and the result from get_audio_streams() minus 1.
		virtual int get_audio_channels_in_stream( int stream_index ) const = 0;

		// Provides a hint to the input implementation - allows the user to say 
		// 'I only want image or audio, or just fetch the nearest intra frame to here' in the next fetch
		// NB: usage is not enforced - up to the implementation to decide whether they do it or not
		virtual void set_process_flags( int flags ) { process_ = flags; }
		virtual int get_process_flags( ) { return process_; }

		// Default seek functionality
		virtual void seek( const int position, const bool relative = false )
		{
			position_ = opencorelib::utilities::clamp( relative ? position_ + position : position, 0, int( get_frames( ) - 1 ) );
		}

		// Query the current position
		virtual int get_position( ) const { return position_; }

		// frame fetch method
		virtual frame_type_ptr fetch( )
		{
			frame_type_ptr result;
			exception_ptr exception;

			try
			{
				do_fetch( result );
			}
			catch( olib::opencorelib::base_exception &be )
			{
				exception = exception_ptr( new olib::opencorelib::base_exception( be ) );
			}
			catch( std::exception &be )
			{
				exception = exception_ptr( new std::exception( be ) );
			}

			if ( !result )
				result = frame_type_ptr( new frame_type( ) );

			if ( report_exceptions_ && exception )
				result->push_exception( exception, shared_from_this( ) );

			if ( !report_exceptions_ )
				result->clear_exceptions( );

			return result;
		}

		// Overloaded convenience fetch method
		virtual frame_type_ptr fetch( int position )
		{
			seek( position );
			return fetch( );
		}

		// Virtual push method for a frame
		virtual bool push( frame_type_ptr ) { return false; }

		// Allow the app to refresh a cached frame
		virtual void refresh_cache( frame_type_ptr ) { }

		// Deprecated
		virtual void acquire_values( ) { }

		// Determines if the particular input is thread safe (should be re-implemented as required
		virtual bool is_thread_safe( ) { return true; }

		// Indication for node reuse
		virtual bool reuse( ) { return true; }

		// Determine the debug level
		inline int debug_level( ) { return prop_debug_.value< int >( ); }

	protected:
		// Virtual method for initialization
		virtual bool initialize( ) { return true; }

		// Pure virtual do_fetch
		virtual void do_fetch( frame_type_ptr & ) = 0;

		// Virtual frame count caching
		virtual void sync_frames( ) { }

		olib::openpluginlib::pcos::property_container properties_;

	private:
		bool initialized_;
		olib::openpluginlib::pcos::property prop_debug_;
		int position_;
		int process_;
		bool report_exceptions_;
};

} } }

#endif
