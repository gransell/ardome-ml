
#include "store_wav.hpp"

#include <openmedialib/ml/audio.hpp>
#include <openmedialib/ml/packet.hpp>

namespace olib { namespace openmedialib { namespace ml { namespace wav {

store_wav::store_wav(const pl::wstring &resource)
	: ml::store_type()
	, initialized_(false)
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
	, samples(0)
	, real_bytes_per_sample(0)

{
std::cerr << "wav: store_wav::store_wav(" << pl::to_string(resource_) << ")" << std::endl;
	properties( ).append( prop_enabled_ = 1 );
	properties( ).append( prop_count_ = 1 );
	properties( ).append( prop_deferrable_ = 1 );
}

store_wav::~store_wav()
{
std::cerr << "wav: store_wav::~store_wav()" << std::endl;
	closeFile();
}

bool store_wav::init()
{
std::cerr << "wav: store_wav::init()" << std::endl;
	return true;
}

void store_wav::initializeFirstFrame(ml::frame_type_ptr frame)
{
std::cerr << "wav: store_wav::initializeFirstFrame()" << std::endl;
	std::string resource = pl::to_string(resource_);
	if (resource.find("wav:") == 0) {
		resource = resource.substr(4);
		size_t ext = resource.rfind('.');
		if (ext == std::string::npos || ext < resource.size() - 4)
			// Append a ".wav" extension
			resource += ".wav";
	}

std::cerr << "wav: filename will be: " << resource << std::endl;

	olib::openmedialib::ml::stream_type_ptr s(frame->get_stream());
	ml::audio_type_ptr audio(frame->get_audio());

	ml::audio::identity ident = audio->id();

	bytes_per_sample      = audio->sample_size();
	frequency             = audio->frequency();
	channels              = audio->channels();
	samples               = audio->samples();
	real_bytes_per_sample = bytes_per_sample;

	if (ident == ml::audio::pcm8_id)
		real_bytes_per_sample = 1;
	else if (ident == ml::audio::pcm16_id)
		real_bytes_per_sample = 2;
	else if (ident == ml::audio::pcm24_id)
		real_bytes_per_sample = 3;
	else if (ident == ml::audio::pcm32_id ||
	         ident == ml::audio::float_id)
		real_bytes_per_sample = 4;

	riff::wav::wave w;
	riff::wav::fmt_base fmt;
	riff::wav::ds64 ds64;
	riff::wav::as_junk<riff::wav::ds64> junk; // either this or ds64
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

	int count = -1;
	try {
		//count = property("count").value<int>();
	} catch (...) {}

	w.size = 4 // 'WAVE'
	       + sizeof(fmt)
	       + sizeof(junk)
	    // + sizeof(bext)
	       + sizeof(data)
	       + 0; // data bytes; we don't know

	writeonly = (count >= 0);

	if (!file_) {
		file_ = fopen(resource.c_str(), writeonly ? "wb" : "w+b");
	}

	if (count == 0) {
		// We don't know total length, will use junk
		// block to allocate space for a ds64 block if
		// needed when reconstructing the header.

	} else {
		// We know length hence can set everything

		w.size += samples * fmt.block_alignment;

		if (rf64) {
			w.setType("RF64");

			//ds64. ... = ;
		} else {
			data.size = samples * fmt.block_alignment;
		}

		;
	}

#			define Write(block) \
		fwrite(&block, sizeof(block), 1, file_)

	Write(w);
	Write(fmt);
	rf64 ? Write(ds64) : Write(junk);
	// Write(bext);
	Write(data);
}

bool store_wav::push(ml::frame_type_ptr frame)
{
std::cerr << "wav: store_wav::push()" << std::endl;

	if (!frame) {
		// What?
		std::cerr << "store_wav::push(): frame is null?" << std::endl;
		return false;
	}

	if (!initialized_) {
		// This is the first frame, parse stream and
		// write header.
		initializeFirstFrame(frame);
		initialized_ = true;
	}

	ml::audio_type_ptr audio(frame->get_audio());

	// Add total number of parsed samples
	samples += audio->samples();

std::cerr << "wav: store_wav::push(): samples=" << audio->samples() << std::endl;

	if (real_bytes_per_sample != bytes_per_sample) {
		// This happens only for 24 bits per second, stored as 4 bytes
		if (bytes_per_sample != 4) {
			std::cerr << "store_wav::push(): Format not supported" << std::endl;
			return false;
			//throw;
		}

		uint32_t* p = (uint32_t*)audio->pointer();
		uint32_t* end = p + (audio->size() / 4);
		--p;
		while (++p < end) {
			le<uint32_t> sample(*p);
			fwrite(&sample, 3, 1, file_);
		}
	} else
		// Save the bytes just as they are
		fwrite(audio->pointer(), 1, audio->size(), file_);

	return true;

	if ( frame && prop_enabled_.value< int >( ) )
	{
/*
		std::cout << "--------------------------------------------------------------------------------" << std::endl;
		std::deque< ml::frame_type_ptr > queue = ml::frame_type::unfold( frame );
		for( std::deque< ml::frame_type_ptr >::iterator iter = queue.begin( ); iter != queue.end( ); iter ++ )
		{
			frame_report_basic( *iter );
			frame_report_image( *iter );
			frame_report_alpha( *iter );
			frame_report_audio( *iter );
			frame_report_props( *iter );
		}
*/
	}
	else if ( prop_enabled_.value< int >( ) )
	{
		std::cerr << "wav: No frame received.\n" << std::endl;
	}
	prop_count_.set< int >( prop_count_.value< int >( ) - 1 );
	return prop_count_.value< int >( ) != 0;
}

ml::frame_type_ptr store_wav::flush()
{
std::cerr << "wav: store_wav::flush()" << std::endl;
	return ml::frame_type_ptr( );
}

void store_wav::complete()
{
std::cerr << "wav: store_wav::complete()" << std::endl;
	closeFile();
}

// Rewinds the file pointer, goes through the header and rewrites the
// necessary parts.
void store_wav::vitalizeHeader() {
std::cerr << "wav: store_wav::vitalizeHeader()" << std::endl;

	if (writeonly)
		// File is opened for streaming output, we aren't allowed to
		// rewind it.
		return;

	if (fseek(file_, 0, SEEK_END))
		return;

	long filelen = ftell(file_);
	rewind(file_);

	size_t offset = 0;
	std::vector<uint8_t> buf(4096);

	if (!fread(&buf[0], 1, buf.size(), file_))
		return;

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

	if (!data || !fmt)
		return;

	// Header size
	long headerlen = ((uint8_t*)data - &buf[0]) - sizeof(*data);

	// Number of samples
	long nsamples = filelen - headerlen;

	// Number of bytes to store all samples
	long nbytes = nsamples * fmt->block_alignment;

	rf64 = (nbytes + headerlen > 0xffffffffUL);

#define Save(block) \
	do { \
		fseek(file_, (uint8_t*)block - &buf[0], SEEK_SET); \
		fwrite(block, sizeof(*block), 1, file_); \
	} while(0)

	if (!rf64) {
		wave->size = nbytes + headerlen - 8;
		Save(wave);

		data->size = nbytes;
		Save(data);
	}
}

void store_wav::closeFile() {
std::cerr << "wav: store_wav::closeFile()" << std::endl;
	if (file_) {
		if (samples) {
			// We have successfully saved samples, but not 
			vitalizeHeader();
		}

		fclose(file_);
		file_ = NULL;
	}
}


} } } }

