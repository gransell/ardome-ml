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
	dv25,
	dv50,
	dv100,
	imx,
	png,
	jpeg
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

		// Colourspace
		virtual olib::openpluginlib::wstring pf( ) = 0;

		// Aspect ratio pair
		virtual fraction ar( ) = 0;

		// Field order
		virtual olib::openimagelib::il::field_order_flags field( ) = 0;

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

/// The packet encoder interface.
class ML_DECLSPEC packet_encoder
{
	public:
		/// Destructor
		virtual ~packet_encoder( ) { }

		/// Dictates which type can be decoded by this decoder
		virtual enum packet_id id( ) = 0;

		/// Decode the packet on the frame
		virtual packet_type_ptr encode( frame_type_ptr ) = 0;
};

/// Register a packet decoder
extern void ML_DECLSPEC packet_handler( packet_decoder_ptr decoder );
extern void ML_DECLSPEC packet_handler( packet_encoder_ptr encoder );

/// Decode a packet on the frame
extern olib::openimagelib::il::image_type_ptr ML_DECLSPEC packet_decode( packet_type_ptr );
extern olib::openimagelib::il::image_type_ptr ML_DECLSPEC packet_decode( frame_type_ptr );

// Encode a packet
extern packet_type_ptr ML_DECLSPEC packet_encode( frame_type_ptr packet, packet_id id );

} } }

#endif

