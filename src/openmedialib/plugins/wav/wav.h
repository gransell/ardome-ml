
#ifndef WAV_H_
#define WAV_H_

#include <opencorelib/cl/endian.hpp>

namespace olib { namespace openmedialib { namespace riff { namespace wav {

#pragma pack(push, 1)

#ifdef WAVE_FORMAT_PCM
#	undef WAVE_FORMAT_PCM
#endif
#ifdef WAVE_FORMAT_IEEE_FLOAT
#	undef WAVE_FORMAT_IEEE_FLOAT
#endif

#define WAVE_FORMAT_PCM        0x0001 // PCM
#define WAVE_FORMAT_IEEE_FLOAT 0x0003 // IEEE float
#define WAVE_FORMAT_ALAW       0x0006 // 8-bit ITU-T G.711 A-law
#define WAVE_FORMAT_MULAW      0x0007 // 8-bit ITU-T G.711 µ-law
#define WAVE_FORMAT_MPEG       0x0050 // MPEG-1 Audio (audio only)
#define WAVE_FORMAT_EXTENSIBLE 0xFFFE // Determined by SubFormat

#define SPEAKER_FRONT_LEFT            0x00000001
#define SPEAKER_FRONT_RIGHT           0x00000002
#define SPEAKER_FRONT_CENTER          0x00000004
#define SPEAKER_LOW_FREQUENCY         0x00000008
#define SPEAKER_BACK_LEFT             0x00000010
#define SPEAKER_BACK_RIGHT            0x00000020
#define SPEAKER_FRONT_LEFT_OF_CENTER  0x00000040
#define SPEAKER_FRONT_RIGHT_OF_CENTER 0x00000080
#define SPEAKER_BACK_CENTER           0x00000100
#define SPEAKER_SIDE_LEFT             0x00000200
#define SPEAKER_SIDE_RIGHT            0x00000400
#define SPEAKER_TOP_CENTER            0x00000800
#define SPEAKER_TOP_FRONT_LEFT        0x00001000
#define SPEAKER_TOP_FRONT_CENTER      0x00002000
#define SPEAKER_TOP_FRONT_RIGHT       0x00004000
#define SPEAKER_TOP_BACK_LEFT         0x00008000
#define SPEAKER_TOP_BACK_CENTER       0x00010000
#define SPEAKER_TOP_BACK_RIGHT        0x00020000
#define SPEAKER_ALL                   0x80000000

#define SPEAKER_STEREO_LEFT           0x20000000
#define SPEAKER_STEREO_RIGHT          0x40000000

#define SPEAKER_CONTROLSAMPLE_1       0x08000000
#define SPEAKER_CONTROLSAMPLE_2       0x10000000

#define SPEAKER_BITSTREAM_1_LEFT      0x00800000
#define SPEAKER_BITSTREAM_1_RIGHT     0x01000000
#define SPEAKER_BITSTREAM_2_LEFT      0x02000000
#define SPEAKER_BITSTREAM_2_RIGHT     0x04000000

using boost::uint8_t;
using boost::uint16_t;
using boost::uint32_t;
using boost::uint64_t;

struct guid {
	opencorelib::endian::little<uint32_t> data1;
	opencorelib::endian::little<uint16_t> data2;
	opencorelib::endian::little<uint16_t> data3;
	opencorelib::endian::little<uint32_t> data4;
	opencorelib::endian::little<uint32_t> data5;
};

struct block {
	block(const char* _type = NULL, uint32_t _size = 0) {
		if (_type)
			setType(_type);
		size = _size;
	}
	void setType(const char* _type = NULL) {
		size_t n = _type ? strlen(_type) : 0;
		n = (n > 4) ? 4 : n;

		type[0] = type[1] = type[2] = type[3] = 0;
		for (size_t i = 0; i < n; ++i)
			type[i] = _type[i];
	}
	bool operator==(std::string _type) {
		return !strncmp(type, _type.c_str(), 4);
	}

	char type[4]; // 'RIFF', 'RF64', 'fmt ', 'data', ...
	opencorelib::endian::little<uint32_t> size;
};

struct wave : block { // 'RIFF' or 'RF64'
	wave(const char* type = "RIFF") : block(type) {
		memcpy(riff_type, "WAVE", 4);
	}

	char riff_type[4]; // 'WAVE'
};

struct junk : block {
	junk(uint32_t size) : block("JUNK", size) {}

	char dummy[0];
};

template<class T>
struct as_junk : block {
	as_junk() : block("JUNK", sizeof(T) - sizeof(block)) {}

	char dummy[sizeof(T) - sizeof(block)];
};

struct fmt_base : block {
	fmt_base(uint32_t size = 0)
	: block("fmt ", size + 16) {}

	opencorelib::endian::little<uint16_t> format_type;
	opencorelib::endian::little<uint16_t> channel_count;
	opencorelib::endian::little<uint32_t> sample_rate;
	opencorelib::endian::little<uint32_t> bytes_per_second;
	opencorelib::endian::little<uint16_t> block_alignment;
	opencorelib::endian::little<uint16_t> bits_per_sample;
};

struct fmt_generic : fmt_base {
	fmt_generic() : fmt_base(24), cb_size(22) {}

	opencorelib::endian::little<uint16_t> cb_size;

	char extra_data[22];
};

struct fmt_format_extensible : fmt_base { // 'fmt ' when FORMAT_EXTENSIBLE
	fmt_format_extensible() : fmt_base(24), cb_size(22) {}

	opencorelib::endian::little<uint16_t> cb_size;

	opencorelib::endian::little<uint16_t> valid_bits_per_sample;
	opencorelib::endian::little<uint32_t> channel_mask;
	guid sub_format;
};

struct chunksize64 : block { // 'big1'
	chunksize64(uint64_t size = 0)
	: block("big1", (uint32_t)(size & 0xffffffff))
	, size_high((uint32_t)(size >> 32)) {}

	opencorelib::endian::little<uint32_t> size_high;
};

struct ds64 : block {
	ds64() : block("ds64", 28) {}

	opencorelib::endian::little<uint32_t> riff_size_low;
	opencorelib::endian::little<uint32_t> riff_size_high;
	opencorelib::endian::little<uint32_t> data_size_low;
	opencorelib::endian::little<uint32_t> data_size_high;
	opencorelib::endian::little<uint32_t> sample_count_low;
	opencorelib::endian::little<uint32_t> sample_count_high;
	opencorelib::endian::little<uint32_t> table_length;
	chunksize64 table[0];
};

struct cue_point {
	opencorelib::endian::little<uint32_t> identifier;
	opencorelib::endian::little<uint32_t> position;
	char data_chunk_id[4];
	opencorelib::endian::little<uint32_t> chunk_start;
	opencorelib::endian::little<uint32_t> block_start;
	opencorelib::endian::little<uint32_t> sample_offset;
};

struct cue_chunk : block { // 'cue '
	cue_chunk(size_t num_cues = 0)
	: block("cue ", 4 + num_cues * sizeof(cue_point)) {}

	opencorelib::endian::little<uint32_t> cue_point_count;
	cue_point cue_points[0];
};

struct list_chunk : block { // 'list'
	list_chunk() : block("list", 4) {}

	char type_id[4]; // 'adtl' associated data list
};

struct label_chunk : block { // 'labl'
	label_chunk(const char* text = NULL)
	: block("labl", 4) {
		if (text)
			size += strlen(text) + 1;
	}

	opencorelib::endian::little<uint32_t> identifier; // 'adtl' associated data list
	char text[0]; // null terminated string
};

struct marker_entry {
	opencorelib::endian::little<uint32_t> flags;
	opencorelib::endian::little<uint32_t> sample_offset_low;
	opencorelib::endian::little<uint32_t> sample_offset_high;
	opencorelib::endian::little<uint32_t> byte_offset_low;
	opencorelib::endian::little<uint32_t> byte_offset_high;
	opencorelib::endian::little<uint32_t> intra_sample_offset_high;
	opencorelib::endian::little<uint32_t> intra_sample_offset_low;
	char label_text[256];
	opencorelib::endian::little<uint32_t> label_chunk_identifier;
	guid vendor_and_product;
	opencorelib::endian::little<uint32_t> user_data_1;
	opencorelib::endian::little<uint32_t> user_data_2;
	opencorelib::endian::little<uint32_t> user_data_3;
	opencorelib::endian::little<uint32_t> user_data_4;
};

struct marker_chunk : block { // 'r64m'
	marker_chunk(size_t num_markers = 0)
	: block("r64m", num_markers * sizeof(marker_entry)) {}

	marker_entry markers[0];
};

struct data : block {
	data(uint32_t size = 0xffffffff) : block("data", size) {}

	char pdata[0];
};

/*
 * Unsupported chunks (mostly BWAV chunks):
 * 'iXML': iXML chunk
 * 'qlty': Quality chunk
 * 'mext': MPEG audio extension chunk
 * 'levl': Peak Envelope chunk
 * 'axml': axml chunk
 */

namespace bwf {

struct bext_chunk : block { // 'bext', size == 752
	bext_chunk() : block("bext", 752) {}

	char description[256];      // ASCII: Description of the sound sequence
	char originator[32];        // ASCII: Name of the originator
	char originator_ref[32];    // ASCII: Reference of the originator
	char origination_date[10];  // ASCII: yyyy-mm-dd
	char origination_time[8];   // ASCII: hh-mm-ss
	opencorelib::endian::little<uint32_t> time_ref_low;  // First sample count since midnight
	opencorelib::endian::little<uint32_t> time_ref_high; // First sample count since midnight
	opencorelib::endian::little<uint16_t> Version;       // Version of BWF; unsigned binary number
	char umid[64];              // SMPTE UMID
	char reserved[190];         // Reserved for future use, set to '\0'
	char CodingHistory[150];    // ASCII: History coding
};

} // namespace bwf

#pragma pack(pop)

} } } }

#endif // WAV_H_

