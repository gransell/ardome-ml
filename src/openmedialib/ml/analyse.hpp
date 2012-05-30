// ml - A media library representation.

// Copyright (C) 2011 Vizrt
// Released under the LGPL.

#ifndef ML_ANALYSE_
#define ML_ANALYSE_

#include <openmedialib/ml/openmedialib_plugin.hpp>
#include <openmedialib/ml/packet.hpp>

namespace olib { namespace openmedialib { namespace ml {

/// Base analyse_type which codec analysers should implement.

class ML_DECLSPEC analyse_type
{
	public:
		// Common keys which may be provided in the analysis collect method
		static olib::openpluginlib::pcos::key key_fps_num_;
		static olib::openpluginlib::pcos::key key_fps_den_;
		static olib::openpluginlib::pcos::key key_packet_size_;
		static olib::openpluginlib::pcos::key key_width_;
		static olib::openpluginlib::pcos::key key_height_;
		static olib::openpluginlib::pcos::key key_pf_;
		static olib::openpluginlib::pcos::key key_field_order_;
		static olib::openpluginlib::pcos::key key_sar_num_;
		static olib::openpluginlib::pcos::key key_sar_den_;
		static olib::openpluginlib::pcos::key key_picture_coding_type_;
		static olib::openpluginlib::pcos::key key_bit_rate_value_;
		static olib::openpluginlib::pcos::key key_temporal_offset_;
		static olib::openpluginlib::pcos::key key_temporal_stream_;
		static olib::openpluginlib::pcos::key key_temporal_reference_;

		// Mpeg2 specific key values
		static olib::openpluginlib::pcos::key key_vbv_delay_;
		static olib::openpluginlib::pcos::key key_closed_gop_;
		static olib::openpluginlib::pcos::key key_broken_link_;
		static olib::openpluginlib::pcos::key key_frame_rate_code_;
		static olib::openpluginlib::pcos::key key_vbv_buffer_size_;
		static olib::openpluginlib::pcos::key key_chroma_format_;
		static olib::openpluginlib::pcos::key key_top_field_first_;
		static olib::openpluginlib::pcos::key key_frame_pred_frame_dct_;
		static olib::openpluginlib::pcos::key key_progressive_frame_;
		static olib::openpluginlib::pcos::key key_aspect_ratio_information_;

		// Convenience method for analysing a prebuilt stream type object
		bool analyse( stream_type_ptr stream );

		// Determines if the codec id matches the analyser
		virtual bool valid( const std::string &codec ) = 0;

		// Analyses the packet and extracts meta data
		virtual bool analysis( boost::uint8_t *data, const size_t size ) = 0;

		// Passes extracted meata data to the properties object (properties are 
		// provided on a per codec basis)
		virtual bool collect( olib::openpluginlib::pcos::property_container &properties ) = 0;

	protected:
		// Helper function for obtaining int values from bit offsets in packets
		int get_bits( boost::uint8_t *p, int bit_offset, int bit_count );
};

// Shared pointer for analyse instances
typedef boost::shared_ptr< analyse_type > analyse_ptr;

// Create an analyse object for the given codec
analyse_ptr ML_DECLSPEC analyse_factory( const std::string &codec );

// Create an analyse object for the stream
analyse_ptr ML_DECLSPEC analyse_factory( const stream_type_ptr stream );

} } }

#endif
