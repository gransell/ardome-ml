// ml - A media library representation.

// Copyright (C) 2008 Ardendo
// Released under the LGPL.

#ifndef OPENMEDIALIB_PACKET_H_
#define OPENMEDIALIB_PACKET_H_

#include <openmedialib/ml/image/image.hpp>
#include <openmedialib/ml/types.hpp>
#include <openpluginlib/pl/pcos/property_container.hpp>

#include <boost/shared_ptr.hpp>
#include <boost/cstdint.hpp>
#include <string>

namespace olib { namespace openmedialib { namespace ml {

enum stream_id
{
	stream_unknown = 0,
	stream_video,
	stream_audio
};

class ML_DECLSPEC stream_type 
{
	public:
		virtual ~stream_type( ) { }

		/// Return the properties associated to this frame.
		virtual olib::openpluginlib::pcos::property_container &properties( ) = 0;

		/// Indicates whether this is from a video or audio stream
		virtual const enum stream_id id( ) const = 0;

		/// Indicates the container format of the input
		virtual const std::string &container( ) const = 0;

		/// Indicates the codec used to decode the stream
		virtual const std::string &codec( ) const = 0;

		/// Returns the length of the packet
		virtual size_t length( ) = 0;

		/// Returns a pointer to the first byte of the packet
		virtual boost::uint8_t *bytes( ) = 0;

		/// Returns the position of the key frame associated to this packet
		virtual const boost::int64_t key( ) const = 0;

		/// Returns the position of the packet
		virtual const boost::int64_t position( ) const = 0;

		/// Returns the bitrate of the packet
		virtual const int bitrate( ) const = 0;
		
		/// Returns a estimated gop size of the file this stream is part of
		/// A value of 0 means unknown
		virtual const int estimated_gop_size( ) const = 0;

		/// Returns the dimensions of the image associated to this packet (0,0 if n/a)
		virtual const dimensions size( ) const { return dimensions( 0, 0 ); }

		/// Returns the sar of the image associated to this packet (1,1 if n/a)
		virtual const fraction sar( ) const { return fraction( 1, 1 ); }

		/// Returns the picture format of the image associated to this packet
		virtual const t_string pf( ) const { return t_string( "" ); }

		/// Returns the field order of the image associated with this packet
		virtual olib::openmedialib::ml::image::field_order_flags field_order( ) const = 0;

		/// Returns the frequency associated to the audio in the packet (0 if n/a)
		virtual const int frequency( ) const { return 0; }

		/// Returns the channels associated to the audio in the packet (0 if n/a)
		virtual const int channels( ) const { return 0; }

		/// Returns the samples associated to the audio in the packet (0 if n/a)
		virtual const int samples( ) const { return 0; }

		/// Returns the sample size of the audio in the packet (0 if n/a)
		virtual const int sample_size( ) const { return 0; }

		/// Moves to the next packet (reimplement on relevant subclasses)
		virtual bool next( ) { return false; }

		/// Indicates if there is more data to be retrieved from this packet
		virtual bool more( ) { return false; }
};

} } }

#endif

