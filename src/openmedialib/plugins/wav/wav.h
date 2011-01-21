
#ifndef WAV_H_
#define WAV_H_

namespace aml { namespace openmedialib { namespace wav {
#pragma pack(push, 1)

struct block {
	char type[4]; // 'WAVE', 'RF64', 'fmt ', 'data', ...
	uint32_t size;
};

struct riff : block {
	char riff_type[4]; // 'WAVE'
};

struct junk : block {
	char dummy[4];
};

struct fmt_base : block {
	uint16_t format_type;
	uint16_t channel_count;
	uint32_t sample_rate;
	uint32_t bytes_per_second;
	uint16_t block_alignment;
	uint16_t bits_per_sample;
	uint16_t cb_size;
};

struct fmt_generic : fmt_base {
	char extra_data[22];
};

struct data : block {
	char data[0];
};

struct chunksize64 : block { // 'big1'
	uint32_t size_high;
};

struct ds64 : block {
	uint32_t riff_size_low;
	uint32_t riff_size_high;
	uint32_t data_size_low;
	uint32_t data_size_high;
	uint32_t sample_count_low;
	uint32_t sample_count_high;
	uint32_t table_length;
	chunksize64 table[0];
};

struct guid {
	uint32_t data1;
	uint16_t data2;
	uint16_t data3;
	uint32_t data4;
	uint32_t data5;
};

struct fmt_format_extensible : fmt_base { // 'fmt ' when FORMAT_EXTENSIBLE
	uint16_t valid_bits_per_sample;
	uint32_t channel_mask;
	guid sub_format;
};

struct cue_point {
	uint32_t identifier;
	uint32_t position;
	char data_chunk_id[4];
	uint32_t chunk_start;
	uint32_t block_start;
	uint32_t sample_offset;
};

struct cue_chunk : block { // 'cue '
	uint32_t cue_point_count;
	cue_point cue_points[0];
};

struct list_chunk : block { // 'list'
	char type_id[4]; // 'adtl' associated data list
};

struct label_chunk : block { // 'labl'
	uint32_t identifier; // 'adtl' associated data list
	char text[0]; // null terminated string
};

struct marker_entry {
	uint32_t flags;
	uint32_t sample_offset_low;
	uint32_t sample_offset_high;
	uint32_t byte_offset_low;
	uint32_t byte_offset_high;
	uint32_t intra_sample_offset_high;
	uint32_t intra_sample_offset_low;
	char label_text[256];
	uint32_t label_chunk_identifier;
	guid vendor_and_product;
	uint32_t user_data_1;
	uint32_t user_data_2;
	uint32_t user_data_3;
	uint32_t user_data_4;
};

struct marker_chunk : block { // 'r64m'
	marker_entry markers[0];
};


#pragma pack(pop)
} } }

#endif // WAV_H_

