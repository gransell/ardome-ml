// ml - A media library representation.

// Copyright (C) 2008 Ardendo
// Released under the LGPL.

#ifndef OPENMEDIALIB_PACKET_H_
#define OPENMEDIALIB_PACKET_H_

#include <openmedialib/ml/types.hpp>
#include <openimagelib/il/basic_image.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/cstdint.hpp>

namespace olib { namespace openmedialib { namespace ml {

/// Identifiers of supported packets
enum packet_id
{
	unknown = 0,
	dv25
};

/// The packet_type interface
class ML_DECLSPEC packet_type 
{
	public:
		/// Destructor
		virtual ~packet_type( ) { }

		/// Returns the id of the packet
		virtual enum packet_id id( ) = 0;

		/// Returns the length of the packet
		virtual size_t length( ) = 0;

		/// Returns a pointer to the first byte of the packet
		virtual boost::uint8_t *bytes( ) = 0;

		/// Provides the sample aspect ratio of the image
		virtual fraction sar( ) = 0;

		/// Provides the dimensions of the image
		virtual dimensions size( ) = 0;
};

/// The packet decoder interface.
class ML_DECLSPEC packet_decoder
{
	public:
		/// Destructor
		virtual ~packet_decoder( ) { }

		/// Dictates which type can be decoded by this decoder
		virtual enum packet_id id( ) = 0;

		/// Decode the packet on the frame
		virtual olib::openimagelib::il::image_type_ptr decode( packet_type_ptr ) = 0;
};

/// Register a packet decoder
extern void ML_DECLSPEC packet_handler( packet_decoder_ptr );

/// Decode a packet on the frame
extern olib::openimagelib::il::image_type_ptr ML_DECLSPEC packet_decode( packet_type_ptr );

} } }

#endif

