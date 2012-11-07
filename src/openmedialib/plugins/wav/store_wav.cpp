
#include "store_wav.hpp"

#include <opencorelib/cl/logger.hpp>
#include <opencorelib/cl/enforce.hpp>
#include <opencorelib/cl/enforce_defines.hpp>
#include <openmedialib/ml/audio.hpp>
#include <openmedialib/ml/stream.hpp>

namespace olib { namespace openmedialib { namespace ml { namespace wav {

using boost::uint8_t;
using boost::uint16_t;
using boost::uint32_t;
using boost::uint64_t;

#define le olib::opencorelib::endian::little

store_wav::store_wav(const std::wstring &resource)
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
	std::string resource_name = olib::opencorelib::str_util::to_string(resource_);
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

	std::string resource = olib::opencorelib::str_util::to_string(resource_);
	if (resource.find("wav:") == 0) {
		resource = resource.substr(4);
		size_t ext = resource.rfind('.');
		if (ext == std::string::npos || ext < resource.size() - 5)
			// Append a ".wav" extension
			if (false) // Apply if wanted
				resource += ".wav";
	}

	//If the URI uses ftp, then add the aml: protocol so that
	//the generic connector system will be used.
	if( resource.find( "ftp:" ) == 0 )
		resource = "aml:" + resource;


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
			real_bytes_per_sample = 1; break;
		case ml::audio::pcm16_id:
			real_bytes_per_sample = 2; break;
		case ml::audio::pcm24_id:
			real_bytes_per_sample = 3; break;
		case ml::audio::pcm32_id:
		case ml::audio::float_id:
			real_bytes_per_sample = 4; break;
	};

	// Allocate headers, we'll write them down to file as they are

	riff::wav::wave w;
	riff::wav::fmt_base fmt;
	riff::wav::ds64 ds64; // Will be transformed into JUNK if necessary
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
	// afterwards.

	int samples = 0;

	uint64_t header_size =
		  sizeof(w)
		+ sizeof(fmt)
		+ sizeof(ds64)
		+ sizeof(data)
		+ 0; // data bytes; we don't know

	writeonly = (samples > 0);

	if (!file_) {
		//Try to open with read-write access first
		//Know that READ_WRITE is a lie! The file will be write-only.
		int error = avio_open2( &file_, resource.c_str( ), AVIO_FLAG_READ_WRITE, 0, 0 );
		if( !file_ || error )
		{
			//If that failed, open with just write access.
			ARLOG_WARN( "Could not open %1% with read/write access. Attempting to open with just write. Will not be able to rewrite WAV header" )( resource );
			writeonly = true;
			error = avio_open2( &file_, resource.c_str( ), AVIO_FLAG_WRITE, 0, 0 );
			ARENFORCE_MSG( file_ && !error, "Failed to open %1% for writing. error = %3%" )( resource )( error );
		}
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
		avio_write( file_, ( unsigned char * )&block, sizeof(block) )

	Write(w);
	Write(fmt);
	Write(ds64);
	Write(data);

	avio_flush( file_ );
	ARENFORCE( file_->error >= 0 );
}

// Creates one large array of endian::little<T>'s which are filled with the
// values from p and then stored as is down to file in one chunk.
template<typename T>
inline void saveRangeFast(AVIOContext* file, T* p, size_t nblocks) {
	boost::scoped_array< le<T> > blocks( new le<T>[nblocks] );
	size_t i = 0;
	while (i < nblocks)
		blocks[i++] = *p++;

	avio_write(file, ( unsigned char * ) blocks.get(), sizeof(T) * nblocks);
}

// Goes through p sample by sample and saves them. This is necessary when the
// sample is isn't a power of 2.
template<typename T, size_t realsizeofT>
inline void saveRangeNormal(AVIOContext* file, T* p, size_t nblocks, std::vector<unsigned char> &conversion_buffer) {
	const size_t conversion_buffer_size = nblocks * realsizeofT;
	if (conversion_buffer.size() < conversion_buffer_size)
		conversion_buffer.resize(conversion_buffer_size);

	unsigned char *dst = &conversion_buffer[0];
	le<T> sample;
	for (size_t i = 0; i < nblocks; ++i) {
		sample = *p++;
		memcpy(dst, &sample, realsizeofT);
		dst += realsizeofT;
	}
	avio_write(file, &conversion_buffer[0], nblocks * realsizeofT);
}

template<typename T, size_t realsizeofT>
inline void saveRange(AVIOContext* file, void* array, size_t nbytes, std::vector<unsigned char> &conversion_buffer) {
	size_t nblocks = nbytes / sizeof(T);
	T* p = (T*)array;

	if (realsizeofT == sizeof(T))
		saveRangeFast<T>(file, p, nblocks);
	else
		saveRangeNormal<T, realsizeofT>(file, p, nblocks, conversion_buffer);
}

bool store_wav::push(ml::frame_type_ptr frame)
{
	ARLOG_DEBUG5("store_wav::push()");
	ARENFORCE_MSG( frame, "store_wav::push(): Frame is NULL?!" );

	ml::audio_type_ptr audio(frame->get_audio());

	// These are the values from the audio object
	int samples = audio->samples();
	int orig_samples = audio->original_samples();
	int size = audio->original_size();

	// Add total number of parsed samples
	accumulated_samples += orig_samples;

	ARLOG_DEBUG6("store_wav::push(): %d audio samples in this frame (%d valid ones)")(samples)(orig_samples);

#if BYTE_ORDER == LITTLE_ENDIAN
	if (real_bytes_per_sample == bytes_per_sample)
	{
		// This is no doubt the fastest, just write the memory
		// chunk down to file in one go.
		avio_write( file_, ( unsigned char * )audio->pointer(), size );

	}
	else
#endif // BYTE_ORDER == LITTLE_ENDIAN
	if (real_bytes_per_sample == 1)
	{
		saveRange<uint8_t,  1>(file_, audio->pointer(), size, conversion_buffer_);
	}
	else if (real_bytes_per_sample == 2)
	{
		saveRange<uint16_t, 2>(file_, audio->pointer(), size, conversion_buffer_);
	}
	else if (real_bytes_per_sample == 3)
	{
		saveRange<uint32_t, 3>(file_, audio->pointer(), size, conversion_buffer_);
	}
	else if (real_bytes_per_sample == 4)
	{
		saveRange<uint32_t, 4>(file_, audio->pointer(), size, conversion_buffer_);
	}

	avio_flush( file_ );
	ARENFORCE( file_->error >= 0 );

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

	if (writeonly) {
		// File is opened for streaming output, we aren't allowed to
		// rewind it.
		ARLOG_DEBUG("store_wav::vitalizeHeader(): "
			"In writeonly mode, will not reconstruct header");
		return;
	}

	// avio_flush( file_ );

	int64_t s_filelen = avio_size( file_ );
	ARENFORCE_MSG( s_filelen != -1 , "Could not get file size" );

	uint64_t filelen = (uint64_t)s_filelen;
	avio_seek( file_, 0, SEEK_SET );

	size_t offset = 0;
	std::vector<uint8_t> buf(4096);

	AVIOContext *tmp_file;
	ARENFORCE_MSG( avio_open2( &tmp_file, olib::opencorelib::str_util::to_string(resource_).c_str( ), AVIO_FLAG_READ, 0, 0 ) == 0, "Could not reopen file for reading");
	ARENFORCE_MSG( avio_read( tmp_file, &buf[0], buf.size( ) ) == buf.size( ) , "Could not read file" );
	ARENFORCE_MSG( tmp_file->error == 0, "Error when reading file" );
	avio_close( tmp_file );
	
	riff::wav::block* blk = NULL;
	riff::wav::wave* wave = NULL;
	riff::wav::fmt_base* fmt = NULL;
	riff::wav::ds64* ds64 = NULL;
	riff::wav::data* data = NULL;

	blk = wave = (riff::wav::wave*)&buf[0];

	ARENFORCE_MSG( *wave == "RIFF" || *wave == "RF64" , "Bad file header" );

	offset = 12;

	while (offset < buf.size()) {
		blk = (riff::wav::block*)&buf[offset];

		if (*blk == "fmt ") {

			ARLOG_DEBUG4("store_wav::vitalizeHeader(): Found fmt block");
			fmt = (riff::wav::fmt_base*)blk;

		} else if (*blk == "ds64") {

			ARLOG_DEBUG4("store_wav::vitalizeHeader(): Found ds64 block");
			ds64 = (riff::wav::ds64*)blk;

		} else if (*blk == "JUNK" &&
		           !ds64 &&
		           blk->size >= sizeof(riff::wav::ds64) - sizeof(riff::wav::block)) {

			ARLOG_DEBUG4("store_wav::vitalizeHeader(): Found JUNK block (perhaps converting into ds64)");
			ds64 = (riff::wav::ds64*)blk;

		} else if (*blk == "data") {

			ARLOG_DEBUG4("store_wav::vitalizeHeader(): Found data block");
			data = (riff::wav::data*)blk;

			//J#AMF-1554: Since the data length is not set, we cannot continue
			//parsing from this point. Since we don't know how much to jump
			//forward, we would start interpreting the sample data as headers.
			break;
		}

		offset += blk->size + 8;
	}

	ARENFORCE_MSG( wave && fmt && ds64 && data , "Could not find all necessary blocks in file" );

	// Header size
	uint64_t headerlen = ((uint8_t*)data - &buf[0]) + sizeof(*data);

	ARENFORCE_MSG( filelen >= headerlen , "File is broken" );

	// Number of samples
	uint64_t nbytes = filelen - headerlen;

	setupHeaders(*wave, *fmt, *ds64, *data, nbytes, headerlen, accumulated_samples);

#define Save(block) \
	do { \
		avio_seek( file_, (uint8_t*)block - &buf[0], SEEK_SET ); \
		avio_write( file_, (uint8_t *)block, sizeof(*block) ); \
	} while(0)

	Save(wave);
	Save(fmt);
	Save(ds64);
	Save(data);

	avio_flush( file_ );
	ARENFORCE( file_->error >= 0 );
}

static uint32_t getLo(uint64_t val) {
	return (uint32_t)(val & 0xffffffffUL);
}
static uint32_t getHi(uint64_t val) {
	return (uint32_t)((val >> 32) & 0xffffffffUL);
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
		ARLOG_DEBUG3("store_wav::setupHeaders(): riffsize > 4GiB, writing RF64 headers");

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
		ARLOG_DEBUG3("store_wav::setupHeaders(): riffsize <= 4GiB, writing RIFF headers");

		wave.size = (uint32_t)(size - 8);
		data.size = (uint32_t)nbytes_of_samples;
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

		avio_close(file_);
		file_ = NULL;
	}
}


} } } }

