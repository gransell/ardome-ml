
#include "store_wav.hpp"

#include <opencorelib/cl/logger.hpp>
#include <opencorelib/cl/enforce.hpp>
#include <opencorelib/cl/enforce_defines.hpp>
#include <openmedialib/ml/audio.hpp>
#include <openmedialib/ml/packet.hpp>

namespace olib { namespace openmedialib { namespace ml { namespace wav {

using boost::uint8_t;
using boost::uint16_t;
using boost::uint32_t;
using boost::uint64_t;

store_wav::store_wav(const pl::wstring &resource)
	: ml::store_type()
	, file_(NULL)
	, writeonly(true)
	, resource_(resource)
	, prop_enabled_( pcos::key::from_string( "enabled" ) )
	, prop_count_( pcos::key::from_string( "count" ) )
	, prop_deferrable_( pcos::key::from_string( "deferrable" ) )

	, rf64(false)
	, bytes_per_sample(0)
	, frequency(0)
	, channels(0)
	, accumulated_samples(0)
	, real_bytes_per_sample(0)

{
	std::string resource_name = pl::to_string(resource_);
	ARLOG_DEBUG("store_wav::store_wav(\"%s\")")(resource_name.c_str());

	properties( ).append( prop_enabled_ = 1 );
	properties( ).append( prop_count_ = 1 );
	properties( ).append( prop_deferrable_ = 1 );
}

store_wav::~store_wav()
{
	ARLOG_DEBUG3("store_wav::~store_wav()");
	closeFile();
}

bool store_wav::init()
{
	ARLOG_DEBUG2("store_wav::init()");
	return true;
}

void store_wav::initializeFirstFrame(ml::frame_type_ptr frame)
{
	ARLOG_DEBUG2("store_wav::initializeFirstFrame()");

	std::string resource = pl::to_string(resource_);
	if (resource.find("wav:") == 0) {
		resource = resource.substr(4);
		size_t ext = resource.rfind('.');
		if (ext == std::string::npos || ext < resource.size() - 5)
			// Append a ".wav" extension
			if (false) // Apply if wanted
				resource += ".wav";
	}

	olib::openmedialib::ml::stream_type_ptr s(frame->get_stream());
	ml::audio_type_ptr audio(frame->get_audio());

	ml::audio::identity ident = audio->id();

	bytes_per_sample      = audio->sample_size();
	frequency             = audio->frequency();
	channels              = audio->channels();
	// This is just the number of samples within this frame_type_ptr
	// samples               = audio->samples();
	real_bytes_per_sample = bytes_per_sample;

	switch (ident) {
		case ml::audio::pcm8_id:
			real_bytes_per_sample = 1;
		case ml::audio::pcm16_id:
			real_bytes_per_sample = 2;
		case ml::audio::pcm24_id:
			real_bytes_per_sample = 3;
		case ml::audio::pcm32_id:
		case ml::audio::float_id:
			real_bytes_per_sample = 4;
	};

	// Allocate headers, we'll write them down to file as they are

	riff::wav::wave w;
	riff::wav::fmt_base fmt;
	riff::wav::ds64 ds64; // Will be transformed into JUNK if necessary
	riff::wav::bwf::bext_chunk bext;
	riff::wav::data data;

	fmt.format_type      = ident == ml::audio::float_id
		? WAVE_FORMAT_IEEE_FLOAT
		: WAVE_FORMAT_PCM;
	fmt.channel_count    = channels;
	fmt.sample_rate      = frequency;
	fmt.bytes_per_second = real_bytes_per_sample * channels * frequency;
	fmt.block_alignment  = real_bytes_per_sample * channels;
	fmt.bits_per_sample  = real_bytes_per_sample * 8;

	// Figure out the total clip length. If we can do this, we can write a
	// proper header right now. Otherwise we'll need to fix the header
	// afterwords.

	int samples = 0;
	int count = -1;
	try {
		//count = property("count").value<int>();
	} catch (...) {}

	uint64_t header_size =
		  sizeof(w)
		+ sizeof(fmt)
		+ sizeof(ds64)
		// + sizeof(bext)
		+ sizeof(data)
		+ 0; // data bytes; we don't know

	writeonly = (samples >= 0);

	if (!file_) {
		file_ = fopen(resource.c_str(), writeonly ? "wb" : "w+b");
	}

	if (samples == 0) {
		// We don't know total length, will use junk
		// block to allocate space for a ds64 block if
		// needed when reconstructing the header.

	} else {
		// We know length hence can set everything

		;
	}

	setupHeaders(w, fmt, ds64, data,
		samples * (uint64_t)fmt.block_alignment,
		header_size,
		samples);

#	define Write(block) \
		fwrite(&block, sizeof(block), 1, file_)

	Write(w);
	Write(fmt);
	Write(ds64);
	// Write(bext);
	Write(data);
}

bool store_wav::push(ml::frame_type_ptr frame)
{
	ARLOG_DEBUG5("store_wav::push()");
	ARENFORCE_MSG( frame, "store_wav::push(): Frame is NULL?!" );

	ml::audio_type_ptr audio(frame->get_audio());

	// Add total number of parsed samples
	accumulated_samples += audio->samples();

	ARLOG_DEBUG6("store_wav::push(): %d audio samples in this frame")(audio->samples());

	if (real_bytes_per_sample != bytes_per_sample) {
		// This happens only for 24 bits per second, stored as 4 bytes
		ARENFORCE_MSG( bytes_per_sample == 4, "Cannot understand audio sample format" );

		uint32_t* p = (uint32_t*)audio->pointer();
		uint32_t* end = p + (audio->size() / 4);
		--p;
		while (++p < end) {
			le<uint32_t> sample(*p);
			fwrite(&sample, 3, 1, file_);
		}
	} else
		// Save the bytes just as they are
		// NOTE: This requires the internal audio to be little endian!
		fwrite(audio->pointer(), 1, audio->size(), file_);

	return true;
}

ml::frame_type_ptr store_wav::flush()
{
	ARLOG_DEBUG6("store_wav::flush()");
	return ml::frame_type_ptr( );
}

void store_wav::complete()
{
	ARLOG_DEBUG2("store_wav::complete()");
	closeFile();
}

// Rewinds the file pointer, goes through the header and rewrites the
// necessary parts.
void store_wav::vitalizeHeader() {
	ARLOG_DEBUG("store_wav::vitalizeHeader()");

	if (writeonly)
		// File is opened for streaming output, we aren't allowed to
		// rewind it.
		return;

	ARENFORCE_MSG( fseek(file_, 0, SEEK_END) , "Could not seek in file" );

	long s_filelen = ftell(file_);
	ARENFORCE_MSG( s_filelen != -1 , "Could not get file size" );

	uint64_t filelen = (uint64_t)s_filelen;
	rewind(file_);

	size_t offset = 0;
	std::vector<uint8_t> buf(4096);

	ARENFORCE_MSG( fread(&buf[0], 1, buf.size(), file_) > 0 , "Could not read file" );

	riff::wav::block* blk = NULL;
	riff::wav::wave* wave = NULL;
	riff::wav::fmt_base* fmt = NULL;
	riff::wav::ds64* ds64 = NULL;
	riff::wav::bwf::bext_chunk* bext = NULL;
	riff::wav::data* data = NULL;

	blk = wave = (riff::wav::wave*)&buf[0];

	if (!(*wave == "RIFF" || *wave == "RF64"))
		return;

	offset = 12;

	while (offset < buf.size()) {
		blk = (riff::wav::block*)&buf[offset];

		if (*blk == "fmt ") {

			std::cout << "wav::: found fmt" << std::endl;
			fmt = (riff::wav::fmt_base*)blk;

		} else if (*blk == "ds64") {

			std::cout << "wav::: found ds64" << std::endl;
			ds64 = (riff::wav::ds64*)blk;

		} else if (*blk == "JUNK" &&
		           !ds64 &&
		           blk->size >= sizeof(riff::wav::ds64) - sizeof(riff::wav::block)) {

			std::cout << "wav::: found JUNK -> ds64" << std::endl;
			ds64 = (riff::wav::ds64*)blk;

		} else if (*blk == "bext") {

			std::cout << "wav::: found bext" << std::endl;
			bext = (riff::wav::bwf::bext_chunk*)blk;

		} else if (*blk == "data") {

			std::cout << "wav::: found data" << std::endl;
			data = (riff::wav::data*)blk;

		}

		offset += blk->size + 8;
	}

	ARENFORCE_MSG( wave && fmt && ds64 && data , "Could not find all necessary blocks in file" );

	// Header size
	uint64_t headerlen = ((uint8_t*)data - &buf[0]) - sizeof(*data);

	ARENFORCE_MSG( filelen >= headerlen , "File is broken" );

	// Number of samples
	uint64_t nbytes = filelen - headerlen;

	setupHeaders(*wave, *fmt, *ds64, *data, nbytes, headerlen, accumulated_samples);

#define Save(block) \
	do { \
		fseek(file_, (uint8_t*)block - &buf[0], SEEK_SET); \
		fwrite(block, sizeof(*block), 1, file_); \
	} while(0)

	Save(wave);
	Save(fmt);
	Save(ds64);
	Save(data);
}

static uint32_t getLo(uint64_t val) {
	return (uint32_t)(val && 0xffffffffUL);
}
static uint32_t getHi(uint64_t val) {
	return (uint32_t)((val >> 32) && 0xffffffffUL);
}

void store_wav::setupHeaders(
		riff::wav::wave& wave,
		riff::wav::fmt_base& fmt,
		riff::wav::ds64& ds64,
		riff::wav::data& data,
		uint64_t nbytes_of_samples,
		uint64_t headerlen,
		uint64_t nsamples) {

	uint64_t size = nbytes_of_samples + headerlen;
	rf64 = size > 0xffffffffUL;

	wave.setType(rf64 ? "RF64" : "RIFF");
	ds64.setType(rf64 ? "ds64" : "JUNK");

	if (rf64) {
		wave.size = 0xffffffff;
		data.size = 0xffffffff;

		ds64.riff_size_low     = getLo(size - 8);
		ds64.riff_size_high    = getHi(size - 8);
		ds64.data_size_low     = getLo(nbytes_of_samples);
		ds64.data_size_high    = getHi(nbytes_of_samples);
		ds64.sample_count_low  = getLo(nsamples);
		ds64.sample_count_high = getHi(nsamples);
		ds64.table_length = 0;
	} else {
		wave.size = size - 8;
		data.size = nbytes_of_samples;
	}
}

void store_wav::closeFile() {
	ARLOG_DEBUG3("store_wav::closeFile()");

	if (file_) {
		if (accumulated_samples) {
			// We have successfully saved samples, but perhaps not
			// a valid header. Rewrite if necessary.
			vitalizeHeader();
		}

		fclose(file_);
		file_ = NULL;
	}
}


} } } }

