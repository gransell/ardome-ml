// ml - A media library representation.

// Copyright (C) 2011 Vizrt
// Released under the LGPL.

#ifndef ML_ANALYSE_MPEG2
#define ML_ANALYSE_MPEG2

#include <openmedialib/ml/openmedialib_plugin.hpp>
#include <openmedialib/ml/stream.hpp>
#include <openmedialib/ml/analyse.hpp>
#include <opencorelib/cl/enforce_defines.hpp>

namespace olib { namespace openmedialib { namespace ml {

namespace mpeg_start_code
{
	enum type
	{
		undefined = -1,
		picture_start = 0x00,
		user_data_start = 0xb2,
		sequence_header = 0xb3,
		sequence_error = 0xb4,
		extension_start = 0xb5,
		sequence_end = 0xb7,
		group_start = 0xb8			
	};
}

namespace mpeg_extension_id 
{
	enum type
	{
		sequence_extension = 1,
		sequence_display_extension = 2,
		quant_matrix_extension = 3,
		copyright_extension = 4,
		sequence_scalable_extension = 5,
		// 6 is reserved
		picture_display_extension = 7,
		picture_coding_extension = 8,
		picture_spatial_scalable_extension = 9,
		picture_temporal_scalable_extension = 10		
	};
}

////////////////////////////////////////////
// Sequence header structure	
typedef struct sequence_header
{
	bool found;
	int horizontal_size_value;			// 12 bits
	int vertical_size_value;			// 12 bits
	int aspect_ratio_information;		// 4 bits
	int frame_rate_code;				// 4 bits
	int bit_rate_value;					// 18 bits
	int marker_bit;						// 1 bits
	int vbv_buffer_size_value;			// 10 bits
	int constrained_parameters_flag;	// 1 bits
} sequence_header;

///////////////////////////////////////////////
// Sequence extension structure
typedef struct sequence_extension {
	bool found;
	int extension_start_code_identifier;	// 4 bits
	int profile_and_level_indication;		// 8 bits
	int progressive_sequence;				// 1 bit
	int chroma_format;						// 2 bits
	int horizontal_size_extension;			// 2 bits
	int vertical_size_extension;			// 2 bits
	int bit_rate_extension;					// 12 bits
	int marker_bit;							// 1 bits
	int vbv_buffer_size_extension;			// 8 bits
	int low_delay;							// 1 bits
	int frame_rate_extension_n;				// 2 bits
	int frame_rate_extension_d;				// 5 bits
} sequence_extension;

///////////////////////////////////////////
// GOP header structure
typedef struct gop_header
{
	bool found;
	int offset;			// Offset of the start of the gop header in the packet
	int time_code;		// 25 bits
	int closed_gop;		// 1 bit
	int broken_link;	// 1 bit
} gop_header;

typedef struct picture_header
{
	bool found;
	int temporal_reference;			// 10 bits
	int picture_coding_type;		// 3 bits
	int vbv_delay;					// 16 bits
	int full_pel_forward_vector;	// 1 bit
	int forward_f_code;				// 3 bits
	int full_pel_backward_vector;	// 1 bit
	int backward_f_code;			// 3 bits
} picture_header;

typedef struct picture_coding_extension
{
	bool found;
	int extension_start_code_identifier;	// 4 bits
	int f_code_0_0;							// 4 bits * forward horizontal *
	int f_code_0_1;							// 4 bits * forward vertical *
	int f_code_1_0;							// 4 bits * backward horizontal *
	int f_code_1_1;							// 4 bits * backward vertical *
	int intra_dc_precision;					// 2 bits
	int picture_structure;					// 2 bits
	int top_field_first;					// 1 bit
	int frame_pred_frame_dct;				// 1 bit
	int concealment_motion_vectors;			// 1 bit
	int q_scale_type;						// 1 bit
	int intra_vlc_format;					// 1 bit
	int alternate_scan;						// 1 bit
	int repeat_first_field;					// 1 bit
	int chroma_420_type;					// 1 bit
	int progressive_frame;					// 1 bit
	int composite_display_flag;				// 1 bit
	int v_axis;								// 1 bit
	int field_sequence;						// 3 bits
	int sub_carrier;						// 1 bit
	int burst_amplitude;					// 7 bits
	int sub_carrier_phase;					// 8 bits
} picture_coding_extension;

class ML_DECLSPEC analyse_mpeg2 : public analyse_type
{
	public:
		analyse_mpeg2( );
		virtual ~analyse_mpeg2( ) { }

		bool valid( const std::string &codec );

		bool analysis( boost::uint8_t *data, const size_t size );

		bool collect( olib::openpluginlib::pcos::property_container &properties );

	private:
		mpeg_start_code::type find_start( boost::uint8_t *&p, boost::uint8_t *end );
		void parse_sequence_header( boost::uint8_t *&buf );
		void parse_sequence_extension( boost::uint8_t *&buf );
		void parse_gop_header( boost::uint8_t *&buf );
		void parse_picture_header( boost::uint8_t *&buf );
		void parse_picture_coding_extension( boost::uint8_t *&buf );
    	
		/// Hold the last sequence header we read
		sequence_header seq_hdr_;
		/// Hold the last sequence extension information
		sequence_extension seq_ext_;
		/// Hold the last parsed gop header
		gop_header gop_hdr_;
		/// Hold the last picture header
		picture_header pict_hdr_;
		/// Hold the last extensions to picture header
		picture_coding_extension pict_ext_;
};

} } }

#endif
