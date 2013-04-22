// Audio resampler and formatting via swresampler
//
// Copyright (C) Vizrt 2013

#ifndef AML_AVFORMAT_AVAUDIO
#define AML_AVFORMAT_AVAUDIO

#include <openmedialib/ml/openmedialib_plugin.hpp>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
#include <libavutil/opt.h>
}



// ########################################################################################




namespace olib { namespace openmedialib { namespace ml {

namespace cl = olib::opencorelib;
namespace pl = olib::openpluginlib;
namespace il = olib::openimagelib::il;

class avaudio_resampler
{
	public:

		avaudio_resampler(	int frequency_in, 
							int frequency_out, 
							int channels_in, 
							int channels_out, 
							boost::uint64_t layout_in,
							boost::uint64_t layout_out,
							AVSampleFormat format_in,
							AVSampleFormat format_out );

		~avaudio_resampler();

	private:

		avaudio_resampler( const avaudio_resampler& );
		avaudio_resampler& operator=( const avaudio_resampler& );

	public:

		int resample( boost::uint8_t **out, 
					  int out_count, 
					  const boost::uint8_t **in, 
					  int in_count);


	private:

		SwrContext *resampler_;
};


// ########################################################################################

class avaudio_convert_to_aml
{
	public:

		avaudio_convert_to_aml(	int frequency_in, 
								int frequency_out, 
								int channels_in, 
								int channels_out, 
								AVSampleFormat format_in,
								audio::identity format_out ) ;

		~avaudio_convert_to_aml( );

	private:

		avaudio_convert_to_aml( const avaudio_convert_to_aml& );
		avaudio_convert_to_aml& operator=( const avaudio_convert_to_aml& );

	public:

		audio_type_ptr convert( const boost::uint8_t **src, int samples, int& out_size);
		audio_type_ptr convert( const boost::uint8_t **src, int samples );
		audio_type_ptr convert_with_offset( const boost::uint8_t **src, int samples, int offset_samples );
		audio_type_ptr convert( const audio_type_ptr& audio );

		audio::identity get_out_format( ) const;

		AVSampleFormat get_in_format( ) const;
		int get_in_frequency( ) const;
		int get_in_channels( ) const;

	private:

		avaudio_resampler *resampler_;

		int frequency_in_; 
		int channels_in_; 
		AVSampleFormat format_in_;
		int frequency_out_; 
		int channels_out_; 
		audio::identity format_out_;

};

// ########################################################################################

class avaudio_convert_to_AVFrame
{
	public:

		avaudio_convert_to_AVFrame(	int frequency_in, 
									int frequency_out, 
									int channels_in, 
									int channels_out, 
									AVSampleFormat format_in,
									AVSampleFormat format_out );

		~avaudio_convert_to_AVFrame( );

	private:

		avaudio_convert_to_AVFrame( const avaudio_convert_to_AVFrame& );
		avaudio_convert_to_AVFrame& operator=( const avaudio_convert_to_AVFrame& );

	public:

		AVFrame *convert( const boost::uint8_t **src, int samples );
		AVFrame *convert( const audio_type_ptr& audio );

	private:

		avaudio_resampler *resampler_;

		int frequency_in_; 
		int channels_in_; 
		AVSampleFormat format_in_;
		int frequency_out_; 
		int channels_out_; 
		AVSampleFormat format_out_;

};

} } }

// ########################################################################################




#endif
