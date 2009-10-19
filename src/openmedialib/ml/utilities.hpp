
// ml - A media library representation.

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#ifndef OPENMEDIALIB_UTILITIES_INC_
#define OPENMEDIALIB_UTILITIES_INC_

#include <deque>
#include <openimagelib/il/basic_image.hpp>
#include <openimagelib/il/utility.hpp>
#include <boost/cstdint.hpp>
#include <openmedialib/ml/frame.hpp>
#include <openmedialib/ml/audio_utilities.hpp>

namespace olib { namespace openmedialib { namespace ml {

// Calculates the largest common denominator of the terms given
ML_DECLSPEC boost::int64_t gcd( boost::int64_t a, boost::int64_t b );
ML_DECLSPEC boost::int64_t remove_gcd( boost::int64_t &a, boost::int64_t &b );

// Courtesy functions for quick input/output plugin look ups
ML_DECLSPEC bool has_plugin_for( const openpluginlib::wstring &resource, const openpluginlib::wstring &type );
ML_DECLSPEC input_type_ptr create_delayed_input( const openpluginlib::wstring & );
ML_DECLSPEC input_type_ptr create_input( const openpluginlib::wstring & );
ML_DECLSPEC input_type_ptr create_input( const openpluginlib::string & );
ML_DECLSPEC store_type_ptr create_store( const openpluginlib::wstring &, frame_type_ptr );
ML_DECLSPEC store_type_ptr create_store( const openpluginlib::string &, frame_type_ptr );
ML_DECLSPEC filter_type_ptr create_filter( const openpluginlib::wstring & );

ML_DECLSPEC audio_type_ptr audio_mix( const audio_type_ptr& input_a, const audio_type_ptr& input_b );

ML_DECLSPEC audio_type_ptr audio_resample( const audio_type_ptr &, int frequency );
ML_DECLSPEC audio_type_ptr audio_reverse( audio_type_ptr );

ML_DECLSPEC frame_type_ptr frame_convert( frame_type_ptr, const openpluginlib::wstring & );
ML_DECLSPEC frame_type_ptr frame_rescale( frame_type_ptr, int, int, olib::openimagelib::il::rescale_filter filter );
ML_DECLSPEC frame_type_ptr frame_crop_clear( frame_type_ptr );
ML_DECLSPEC frame_type_ptr frame_crop( frame_type_ptr, int, int, int, int );

// Convenience function to change volume on a frame
extern ML_DECLSPEC frame_type_ptr frame_volume( frame_type_ptr, float );

class ML_DECLSPEC audio_reseat
{
	public:
		typedef std::deque< audio_type_ptr > bucket;
		typedef bucket::const_iterator const_iterator;
		typedef bucket::iterator iterator;

	public:
		virtual ~audio_reseat( ) { }
		virtual bool append( audio_type_ptr ) = 0;
		virtual audio_type_ptr retrieve( int samples, bool pad = false ) = 0;
		virtual void clear( ) = 0;
		virtual bool has( int ) = 0;
		virtual iterator begin( ) = 0;
		virtual const_iterator begin( ) const = 0;
		virtual iterator end( ) = 0;
		virtual const_iterator end( ) const = 0;
};

typedef boost::shared_ptr< audio_reseat > audio_reseat_ptr;

extern ML_DECLSPEC audio_reseat_ptr create_audio_reseat( );

// A general abstraction for callback based reading and writing - this allows applications
// to provide their own mechanism for stream reception and output (depending on the 
// capabilities of the input_type).

class ML_DECLSPEC stream_handler
{
	public:
		virtual ~stream_handler( ) { }
		virtual bool open( const openpluginlib::wstring, int ) { return false; }
		virtual openpluginlib::string read( int ) { return openpluginlib::string( "" ); }
		virtual int write( const openpluginlib::string & ) { return -1; }
		virtual long seek( long, int ) { return long( -1 ); }
		virtual int close( ) { return 0; }
		virtual bool is_stream( ) { return true; }
		virtual bool is_thread_safe( ) { return false; }
};

typedef boost::shared_ptr< stream_handler > stream_handler_ptr;

// Method to obtain an instance of a stream_handler (used by plugins)

extern ML_DECLSPEC stream_handler_ptr stream_handler_fetch( const openpluginlib::wstring, int );

// Method to register the stream_handling generator (used by application or libs)
//
// NB: We may eventually want more than one...

extern ML_DECLSPEC void stream_handler_register( stream_handler_ptr ( * )( const openpluginlib::wstring, int ) );

// Determine if the image associated to the frame is yuv planar

inline bool is_yuv_planar( const frame_type_ptr &frame )
{
	return frame ? olib::openimagelib::il::is_yuv_planar( frame->get_image( ) ) : false;
}

} } }

#endif
