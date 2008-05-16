// ml - A media library representation.

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#ifndef OPENMEDIALIB_FILTER_INC_
#define OPENMEDIALIB_FILTER_INC_

#include <vector>
#include <map>
#include <boost/shared_ptr.hpp>
#include <openmedialib/ml/frame.hpp>
#include <openmedialib/ml/input.hpp>
#include <openpluginlib/pl/pcos/property_container.hpp>
#include <openpluginlib/pl/utf8_utils.hpp>

namespace olib { namespace openmedialib { namespace ml {

namespace opl  = olib::openpluginlib;
namespace pcos = olib::openpluginlib::pcos;

class filter_type;
typedef enum location { from_default, from_frame, from_filter };

class ML_DECLSPEC filter_key 
{
	public:
		filter_key( std::string &dst_name, location loc )
			: dst_key_( pcos::key::from_string( dst_name.c_str( ) ) )
			, dst_location_( loc )
			, src_key_( pcos::key::from_string( "dummy" ) )
		{ }

		void refresh( pcos::property input );
		void assign( frame_type_ptr &, filter_type * );

	private:
		pcos::key dst_key_;
		location dst_location_;
		opl::wstring prev_value_;
		std::string default_;
		pcos::key src_key_;
		location src_location_;
};

typedef boost::shared_ptr< filter_key > filter_key_ptr;

class ML_DECLSPEC filter_type : public input_type
{
	public:
		explicit filter_type( );
		virtual ~filter_type( ) { }

		// Filters can request any number of inputs - override this method as required
		virtual const size_t slot_count( ) const 
		{ return 1; }

		// Connect an input to a slot
		virtual bool connect( input_type_ptr input, size_t slot = 0 );

		// NB: The default implementation assumes that the filter wraps a single connected
		// input and that it doesn't affect duration, image constitution or image/audio 
		// availability - a specific implementation of a filter can override all or part
		// of this behaviour (ie: to allow temporal interpolation, transitions, audio
		// visualisation, rescalers and so on). It might prove beneficial for OML to 
		// provide additional subtypes, but for now, this will be seen as an exercise for
		// the plugin implementation to address.
		//
		// Whilst no rules are enforced, the conceptual view is that even in the case of a
		// multiple input filter, the first connected input drives the rules of the output
		// (ie: frame rate, duration, aspect ratio, colour space, audio params and so on) - 
		// thus the output should conform to the first input. In other words, the default
		// implementation of these methods should be sufficient for the majority of cases.

		// Basic information
		virtual const openpluginlib::wstring get_uri( )  const = 0;

		virtual const openpluginlib::wstring get_mime_type( ) const
		{ return slots_[ 0 ] ? slots_[ 0 ]->get_mime_type( ) : L""; }

		// Default seek functionality
		// NB: This implementation allows position to overrun the length of the clip
		// whereas the input implementation clamps the position to the length
		virtual void seek( const int position, const bool relative = false );

		// Query the current position
		virtual int get_position( ) const { return position_; }

		// Audio/Visual
		virtual int get_frames( ) const
		{ return slots_[ 0 ] ? slots_[ 0 ]->get_frames( ) : 0; }

		virtual bool is_seekable( ) const 
		{ return slots_[ 0 ] ? slots_[ 0 ]->is_seekable( ) : false; }

		// Visual
		virtual int get_video_streams( ) const
		{ return slots_[ 0 ] ? slots_[ 0 ]->get_video_streams( ) : 0; }

		// Audio
		virtual int get_audio_streams( ) const
		{ return slots_[ 0 ] ? slots_[ 0 ]->get_audio_streams( ) : 0; }

		// Set video and audio streams
		virtual bool set_video_stream( const int stream )
		{ return slots_[ 0 ] ? slots_[ 0 ]->set_video_stream( stream ) : false; }

		virtual bool set_audio_stream( const int stream )
		{ return slots_[ 0 ] ? slots_[ 0 ]->set_audio_stream( stream ) : false; }

		// Virtual frame fetch method
		virtual frame_type_ptr fetch( ) = 0;

		// Invoked when a slot is changed
		virtual void on_slot_change( input_type_ptr, int ) { }

		// Retrieve the input associated to a slot
		virtual input_type_ptr fetch_slot( size_t slot = 0 ) const
		{ return slot < slots_.size( ) ? slots_[ slot ] : input_type_ptr( ); }

		virtual void acquire_values( );

		virtual bool is_thread_safe( );

		// By default, filters aren't reused
		virtual bool reuse( ) { return false; }

	protected:

		// Convenience function - fetch a frame from the slot and optionally ensure frame properties
		// are assigned to the filter
		frame_type_ptr fetch_from_slot( int index = 0, bool assign = true );

		void assign_frame_props( frame_type_ptr frame );

	private:
		std::vector < input_type_ptr > slots_;
		std::map < std::size_t, filter_key_ptr > keys_;
		int position_;
};

typedef boost::shared_ptr< filter_type > filter_type_ptr;

} } }

#endif
