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
#include <libavformat/url.h>
}

namespace ml = olib::openmedialib::ml;
namespace pl = olib::openpluginlib;
namespace il = olib::openimagelib::il;
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
		void setupHeaders(
			riff::wav::wave&     wave,
			riff::wav::fmt_base& fmt,
			riff::wav::ds64&     ds64,
			riff::wav::data&     data,
			boost::uint64_t      nbytes_of_samples,
			boost::uint64_t      headerlen,
			boost::uint64_t      nsamples);

		void vitalizeHeader();
		void closeFile();

		URLContext *file_;
		bool writeonly;
		std::wstring resource_;
		std::vector<unsigned char> conversion_buffer_;

		pcos::property prop_enabled_;
		pcos::property prop_count_;
		pcos::property prop_deferrable_;

		bool rf64; // size > 4gb

		int bytes_per_sample;
		int frequency;
		int channels;
		int accumulated_samples;
		int real_bytes_per_sample;

};

} } } }

