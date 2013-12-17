// wav store - WAVE RIFF store
//
// Copyright (C) 2011 Ardendo
// Released under the LGPL.
//
// #store:wav:
//
// Stores the pushed frames into a wav file.

#include <openmedialib/ml/openmedialib_plugin.hpp>
#include <openmedialib/ml/types.hpp>

#include <iostream>

#include "wav.h"

extern "C" {
#include <libavformat/avio.h>
}

namespace ml = olib::openmedialib::ml;
namespace pl = olib::openpluginlib;

namespace cl = olib::opencorelib;
namespace pcos = olib::openpluginlib::pcos;

namespace olib { namespace openmedialib { namespace ml { namespace wav {

class ML_PLUGIN_DECLSPEC store_wav : public ml::store_type
{
	public:
		store_wav(const std::wstring &resource);
		/*virtual*/ ~store_wav();

		/*virtual*/ bool init();

		void initializeFirstFrame(ml::frame_type_ptr frame);

		/*virtual*/ bool push(ml::frame_type_ptr frame);

		/*virtual*/ ml::frame_type_ptr flush();

		/*virtual*/ void complete();

	protected:
		static void setupHeaders(
			riff::wav::wave& wave,
			riff::wav::fmt_base& fmt,
			riff::wav::ds64& ds64,
			riff::wav::data& data,
			boost::uint64_t num_samples,
			int real_bytes_per_sample,
			int channels);

		void vitalizeHeader();
		int openFile(AVIOContext **out_context, int avio_flags);
		void closeFile();

		AVIOContext *file_;
		std::wstring resource_;
		std::vector<boost::uint8_t> conversion_buffer_;

		pcos::property prop_enabled_;
		pcos::property prop_deferrable_;

		riff::wav::wave wave_block_;
		riff::wav::fmt_base fmt_block_;
		riff::wav::ds64 ds64_block_; // Will be transformed into JUNK if necessary
		riff::wav::data data_block_;

		int bytes_per_sample_; //4 for 24-bit audio
		int real_bytes_per_sample_; //3 for 24-bit audio
		int frequency_;
		int channels_;

		boost::uint64_t accumulated_samples_;
};

} } } }

