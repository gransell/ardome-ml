
#include "store_wav.hpp"

#include <opencorelib/cl/logger.hpp>
#include <opencorelib/cl/enforce.hpp>
#include <opencorelib/cl/enforce_defines.hpp>
#include <openmedialib/ml/audio.hpp>
#include <openmedialib/ml/stream.hpp>
#include <openmedialib/ml/io.hpp>

namespace olib { namespace openmedialib { namespace ml { namespace wav {

using boost::uint8_t;
using boost::uint16_t;
using boost::uint32_t;
using boost::uint64_t;

#define le olib::opencorelib::endian::little

#define WriteBlock(block) \
	avio_write(file_, reinterpret_cast<uint8_t *>(&block), sizeof(block))

store_wav::store_wav(const std::wstring &resource)
	: ml::store_type()
	, file_(NULL)
	, resource_(resource)
	, prop_enabled_(pcos::key::from_string("enabled"))
	, prop_deferrable_(pcos::key::from_string("deferrable"))
	, bytes_per_sample_(0)
	, real_bytes_per_sample_(0)
	, frequency_(0)
	, channels_(0)
	, accumulated_samples_(0)
{
	std::string resource_name = olib::opencorelib::str_util::to_string(resource_);
	ARLOG_DEBUG("store_wav::store_wav(\"%1%\")")(resource_);

	properties().append(prop_enabled_ = 1);
	properties().append(prop_deferrable_ = 1);
}

store_wav::~store_wav()
{
	ARLOG_DEBUG3("store_wav::~store_wav()");
}

bool store_wav::init()
{
	ARLOG_DEBUG2("store_wav::init()");
	return true;
}

int store_wav::openFile(AVIOContext **out_context, int avio_flags)
{
	std::string resource = olib::opencorelib::str_util::to_string(resource_);

	//Remove "wav:" prefix if present
	if (resource.find("wav:") == 0)
		resource = resource.substr(4);

	//If the URI uses ftp, then add the aml: protocol so that
	//the generic connector system will be used.
	if(resource.find("ftp:") == 0)
		resource = "aml:" + resource;

	return io::open_file(out_context, resource.c_str(), avio_flags);
}

void store_wav::initializeFirstFrame(ml::frame_type_ptr frame)
{
	ARLOG_DEBUG2("store_wav::initializeFirstFrame()");

	olib::openmedialib::ml::stream_type_ptr s(frame->get_stream());
	ml::audio_type_ptr audio(frame->get_audio());

	ml::audio::identity ident = audio->id();

	bytes_per_sample_ = audio->sample_storage_size();
	frequency_ = audio->frequency();
	channels_ = audio->channels();
	real_bytes_per_sample_ = bytes_per_sample_;

	switch (ident) {
		case ml::audio::pcm16_id:
			real_bytes_per_sample_ = 2; break;
		case ml::audio::pcm24_id:
			real_bytes_per_sample_ = 3; break;
		case ml::audio::pcm32_id:
		case ml::audio::float_id:
			real_bytes_per_sample_ = 4; break;
	};

	// Allocate headers, we'll write them down to file as they are

	fmt_block_.format_type      = ident == ml::audio::float_id
		? WAVE_FORMAT_IEEE_FLOAT
		: WAVE_FORMAT_PCM;
	fmt_block_.channel_count    = channels_;
	fmt_block_.sample_rate      = frequency_;
	fmt_block_.bytes_per_second = real_bytes_per_sample_ * channels_ * frequency_;
	fmt_block_.block_alignment  = real_bytes_per_sample_ * channels_;
	fmt_block_.bits_per_sample  = real_bytes_per_sample_ * 8;

	if (!file_) {
		int error = openFile(&file_, AVIO_FLAG_WRITE);
		ARENFORCE_MSG(file_ && !error, "Failed to open %1% for writing. error = %2%")(resource_)(error);
	}

	//We don't know the future number of samples at this point, so send 0
	setupHeaders(wave_block_, fmt_block_, ds64_block_, data_block_, 0, real_bytes_per_sample_, channels_);

	WriteBlock(wave_block_);
	WriteBlock(fmt_block_);
	WriteBlock(ds64_block_);
	WriteBlock(data_block_);

	avio_flush(file_);
	ARENFORCE(file_->error == 0)(file_->error);
}

// Creates one large array of endian::little<T>'s which are filled with the
// values from p and then stored as is down to file in one chunk.
template<typename T>
inline void saveRangeFast(AVIOContext* file, T* p, size_t nblocks) {
	boost::scoped_array< le<T> > blocks(new le<T>[nblocks]);
	size_t i = 0;
	while (i < nblocks)
		blocks[i++] = *p++;

	avio_write(file, (unsigned char *) blocks.get(), sizeof(T) * nblocks);
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
	ARENFORCE_MSG(frame, "store_wav::push(): Frame pushed is NULL!");

	ml::audio_type_ptr audio(frame->get_audio());
	ARENFORCE_MSG(audio, "store_wav::push(): Audio in frame pushed is NULL!");

	// These are the values from the audio object
	int samples = audio->samples();
	int orig_samples = audio->original_samples();
	int size = audio->original_size();

	// Add total number of parsed samples
	accumulated_samples_ += orig_samples;

	ARLOG_DEBUG6("store_wav::push(): %d audio samples in this frame (%d valid ones)")(samples)(orig_samples);

#if BYTE_ORDER == LITTLE_ENDIAN
	if (real_bytes_per_sample_ == bytes_per_sample_)
	{
		// This is no doubt the fastest, just write the memory
		// chunk down to file in one go.
		avio_write(file_, (unsigned char *)audio->pointer(), size);

	}
	else
#endif // BYTE_ORDER == LITTLE_ENDIAN
	if (real_bytes_per_sample_ == 1)
	{
		saveRange<uint8_t,  1>(file_, audio->pointer(), size, conversion_buffer_);
	}
	else if (real_bytes_per_sample_ == 2)
	{
		saveRange<uint16_t, 2>(file_, audio->pointer(), size, conversion_buffer_);
	}
	else if (real_bytes_per_sample_ == 3)
	{
		saveRange<uint32_t, 3>(file_, audio->pointer(), size, conversion_buffer_);
	}
	else if (real_bytes_per_sample_ == 4)
	{
		saveRange<uint32_t, 4>(file_, audio->pointer(), size, conversion_buffer_);
	}

	avio_flush(file_);
	ARENFORCE(file_->error >= 0);

	return true;
}

ml::frame_type_ptr store_wav::flush()
{
	ARLOG_DEBUG6("store_wav::flush()");
	return ml::frame_type_ptr();
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

	//See if the context allows us to seek back to rewrite the WAV header
	const int seek_result = avio_seek(file_, 0, SEEK_SET);
	if(seek_result != 0)
	{
		ARLOG_WARN("Could not seek to the beginning of the file. Cannot rewrite WAV header")(seek_result)(resource_);
		return;
	}

	int64_t s_filelen = avio_size(file_);
	ARENFORCE_MSG(s_filelen != -1 , "Could not get file size");

	setupHeaders(wave_block_, fmt_block_, ds64_block_, data_block_, accumulated_samples_, real_bytes_per_sample_, channels_);

	WriteBlock(wave_block_);
	WriteBlock(fmt_block_);
	WriteBlock(ds64_block_);
	WriteBlock(data_block_);

	avio_flush(file_);
	ARENFORCE(file_->error >= 0)(file_->error);
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
		uint64_t num_samples,
		int real_bytes_per_sample,
		int channels)
{
	const uint32_t header_size = 
		sizeof(wave) + sizeof(fmt) + sizeof(ds64) + sizeof(data);

	const uint64_t num_bytes_of_samples = num_samples * real_bytes_per_sample * channels;

	const uint64_t size = num_bytes_of_samples + header_size;
	const bool rf64 = size > 0xffffffffUL;

	wave.setType(rf64 ? "RF64" : "RIFF");
	ds64.setType(rf64 ? "ds64" : "JUNK");

	if (rf64) {
		ARLOG_DEBUG3("store_wav::setupHeaders(): riffsize > 4GiB, writing RF64 headers");

		wave.size = 0xffffffff;
		data.size = 0xffffffff;

		ds64.riff_size_low     = getLo(size - 8);
		ds64.riff_size_high    = getHi(size - 8);
		ds64.data_size_low     = getLo(num_bytes_of_samples);
		ds64.data_size_high    = getHi(num_bytes_of_samples);
		ds64.sample_count_low  = getLo(num_samples);
		ds64.sample_count_high = getHi(num_samples);
		ds64.table_length = 0;
	} else {
		ARLOG_DEBUG3("store_wav::setupHeaders(): riffsize <= 4GiB, writing RIFF headers");

		wave.size = (uint32_t)(size - 8);
		data.size = (uint32_t)num_bytes_of_samples;
	}
}

void store_wav::closeFile() {
	ARLOG_DEBUG3("store_wav::closeFile()");

	if (file_) {
		if (accumulated_samples_ > 0) {
			// We have successfully saved samples, but perhaps not
			// a valid header. Rewrite if necessary.
			vitalizeHeader();
		}

		int close_error = io::close_file(file_);
		ARENFORCE_MSG(close_error == 0, "Got error when closing file: %1%")(close_error);
		file_ = NULL;
	}
}


} } } }

