// ml - A media library representation.

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#ifndef OPENMEDIALIB_FILTER_INC_
#define OPENMEDIALIB_FILTER_INC_

#include <openmedialib/ml/types.hpp>
#include <openmedialib/ml/input.hpp>

#include <vector>
#include <map>
#include <boost/shared_ptr.hpp>
#include <openpluginlib/pl/pcos/property_container.hpp>
#include <openpluginlib/pl/utf8_utils.hpp>

namespace olib { namespace openmedialib { namespace ml {

typedef enum location { from_default, from_frame, from_filter } location;

class ML_DECLSPEC filter_key 
{
	public:
		filter_key( std::string &dst_name, location loc )
			: dst_key_( olib::openpluginlib::pcos::key::from_string( dst_name.c_str( ) ) )
			, dst_location_( loc )
			, src_key_( olib::openpluginlib::pcos::key::from_string( "dummy" ) )
		{ }

		void refresh( olib::openpluginlib::pcos::property input );
		void assign( frame_type_ptr &, filter_type * );

	private:
		olib::openpluginlib::pcos::key dst_key_;
		location dst_location_;
		olib::openpluginlib::wstring prev_value_;
		std::string default_;
		olib::openpluginlib::pcos::key src_key_;
		location src_location_;
};

typedef boost::shared_ptr< filter_key > filter_key_ptr;

class ML_DECLSPEC filter_type : public input_type
{
	public:
		explicit filter_type( );
		virtual ~filter_type( ) { }

		/// Filters can request any number of inputs - override this method as required
		virtual const size_t slot_count( ) const;

		/// Connect an input to a slot
		virtual bool connect( input_type_ptr input, size_t slot = 0 );

		/// NB: The default implementation assumes that the filter wraps a single connected
		/// input and that it doesn't affect duration, image constitution or image/audio 
		/// availability - a specific implementation of a filter can override all or part
		/// of this behaviour (ie: to allow temporal interpolation, transitions, audio
		/// visualisation, rescalers and so on). It might prove beneficial for OML to 
		/// provide additional subtypes, but for now, this will be seen as an exercise for
		/// the plugin implementation to address.
		///
		/// Whilst no rules are enforced, the conceptual view is that even in the case of a
		/// multiple input filter, the first connected input drives the rules of the output
		/// (ie: frame rate, duration, aspect ratio, colour space, audio params and so on) - 
		/// thus the output should conform to the first input. In other words, the default
		/// implementation of these methods should be sufficient for the majority of cases.

		/// Basic information
		virtual const openpluginlib::wstring get_uri( ) const = 0;

		/// Default seek functionality
		/// NB: This implementation allows position to overrun the length of the clip
		/// whereas the input implementation clamps the position to the length
		virtual void seek( const int position, const bool relative = false );

		/// Query the current position
		virtual int get_position( ) const;

		/// Audio/Visual
		virtual int get_frames( ) const;

		/// Indicates if the graph is seekable
		virtual bool is_seekable( ) const;

		/// Invoked when a slot is changed
		virtual void on_slot_change( input_type_ptr, int );

		/// Retrieve the input associated to a slot
		virtual input_type_ptr fetch_slot( size_t slot = 0 ) const;

		/// Determines if the graph is thread safe
		virtual bool is_thread_safe( );

		/// Get the mime type associated to the input
		/// DEPRECATE?
		virtual const openpluginlib::wstring get_mime_type( ) const;

		// Provides a hint to the input implementation - allows the user to say 
		// 'I only want image or audio, or just fetch the nearest intra frame to here' in the next fetch
		// NB: usage is not enforced - up to the implementation to decide whether they do it or not
		virtual void set_process_flags( int flags );

		/// Invoke the callback associated to this input
		/// DEPRECATE?
		virtual void acquire_values( );

		/// By default, filters aren't reused
		/// DEPRECATE?
		virtual bool reuse( ) { return false; }

		/// Visual
		/// DEPRECATE?
		virtual int get_video_streams( ) const;

		/// Audio
		/// DEPRECATE?
		virtual int get_audio_streams( ) const;

		/// Set video streams
		/// DEPRECATE?
		virtual bool set_video_stream( const int stream );

		/// Set audio streams
		/// DEPRECATE?
		virtual bool set_audio_stream( const int stream );

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
