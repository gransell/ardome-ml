
// ml - A media library representation.

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#ifndef OPENMEDIALIB_UTILITIES_INC_
#define OPENMEDIALIB_UTILITIES_INC_

#include <deque>
#include <openmedialib/ml/types.hpp>
#include <boost/cstdint.hpp>
#include <openmedialib/ml/frame.hpp>
#include <openmedialib/ml/audio_utilities.hpp>

namespace olib { namespace openmedialib { namespace ml {

// Calculates the largest common denominator of the terms given
ML_DECLSPEC boost::int64_t gcd( boost::int64_t a, boost::int64_t b );
ML_DECLSPEC boost::int64_t remove_gcd( boost::int64_t &a, boost::int64_t &b );

// Courtesy functions for quick input/output plugin look ups
ML_DECLSPEC bool has_plugin_for( const std::wstring &resource, const std::wstring &type );
ML_DECLSPEC input_type_ptr create_delayed_input( const std::wstring & );
ML_DECLSPEC input_type_ptr create_input( const std::wstring & );
ML_DECLSPEC input_type_ptr create_input( const std::string & );
ML_DECLSPEC store_type_ptr create_store( const std::wstring &, frame_type_ptr );
ML_DECLSPEC store_type_ptr create_store( const std::string &, frame_type_ptr );
ML_DECLSPEC filter_type_ptr create_filter( const std::wstring & );

ML_DECLSPEC audio_type_ptr audio_resample( const audio_type_ptr &, int frequency );

ML_DECLSPEC frame_type_ptr frame_convert( rescale_object_ptr, frame_type_ptr, const olib::t_string & );
ML_DECLSPEC frame_type_ptr frame_convert( frame_type_ptr, const olib::t_string & );
ML_DECLSPEC frame_type_ptr frame_rescale( frame_type_ptr, int, int, image::rescale_filter filter = image::BICUBIC_SAMPLING );
ML_DECLSPEC frame_type_ptr frame_rescale( rescale_object_ptr, frame_type_ptr, int, int, image::rescale_filter filter = image::BICUBIC_SAMPLING );
ML_DECLSPEC frame_type_ptr frame_crop_clear( frame_type_ptr );
ML_DECLSPEC frame_type_ptr frame_crop( frame_type_ptr, int, int, int, int );

// Convenience function to change volume on a frame
extern ML_DECLSPEC frame_type_ptr frame_volume( frame_type_ptr, float );

// A general abstraction for callback based reading and writing - this allows applications
// to provide their own mechanism for stream reception and output (depending on the 
// capabilities of the input_type).

class ML_DECLSPEC stream_handler
{
	public:
		virtual ~stream_handler( ) { }
		virtual bool open( const std::wstring, int ) { return false; }
		virtual std::string read( int ) { return std::string( "" ); }
		virtual int write( const std::string & ) { return -1; }
		virtual long seek( long, int ) { return long( -1 ); }
		virtual int close( ) { return 0; }
		virtual bool is_stream( ) { return true; }
		virtual bool is_thread_safe( ) { return false; }
};

typedef boost::shared_ptr< stream_handler > stream_handler_ptr;

// Method to obtain an instance of a stream_handler (used by plugins)

extern ML_DECLSPEC stream_handler_ptr stream_handler_fetch( const std::wstring, int );

// Method to register the stream_handling generator (used by application or libs)
//
// NB: We may eventually want more than one...

extern ML_DECLSPEC void stream_handler_register( stream_handler_ptr ( * )( const std::wstring, int ) );

// Determine if the image associated to the frame is yuv planar

inline bool is_yuv_planar( const frame_type_ptr &frame )
{
	//FIXME
	return frame ? true : false;
}

namespace audio
{
	class ML_DECLSPEC reseat
	{
		public:
			typedef std::deque< audio_type_ptr > bucket;
			typedef bucket::const_iterator const_iterator;
			typedef bucket::iterator iterator;
	
		public:
			virtual ~reseat( ) { }
			virtual bool append( audio_type_ptr, boost::uint32_t sample_offset = 0 ) = 0;
			virtual audio_type_ptr retrieve( int samples, bool pad = false ) = 0;
			virtual void clear( ) = 0;
			virtual bool has( int ) = 0;
			virtual int size( ) = 0;
			virtual iterator begin( ) = 0;
			virtual const_iterator begin( ) const = 0;
			virtual iterator end( ) = 0;
			virtual const_iterator end( ) const = 0;
	};
}

} } }

#endif
