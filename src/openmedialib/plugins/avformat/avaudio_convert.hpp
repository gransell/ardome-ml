// Audio resampler and formatting via swresampler
//
// Copyright (C) Vizrt 2013

#ifndef AML_AVFORMAT_AVAUDIO_CONVERT
#define AML_AVFORMAT_AVAUDIO_CONVERT

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

		static void destroy( SwrContext *s );

	public:

		// resample planar or non-planar audio. The array of pointers in/out point to each of the planes
		// returns number of resampled samples
		int resample( boost::uint8_t **out, 
					  int out_count, 
					  const boost::uint8_t **in, 
					  int in_count);

		// resample non-planar audio
		// returns number of resampled samples
		int resample( boost::uint8_t *out, 
					  int out_count, 
					  const boost::uint8_t *in, 
					  int in_count);

		// drain the remaining samples (max out_count) from the resampler
		// returns number of samples drained
		int drain( boost::uint8_t **out, int out_count ); // planar audio out
		int drain( boost::uint8_t *out, int out_count );  // non-planar audio out
		int drain( int num_samples_to_drain );

		// return the number of samples which would be generated if all the input_samples were resampled 
		int output_samples( int input_samples ) const;

		static int output_samples( int in_samples , int frequency_in, int frequency_out );

		// return the number of input samples needed to generate out_samples number of output_samples
		int input_samples( int out_samples ) const;

		static int input_samples( int out_samples , int frequency_in, int frequency_out );

		AVSampleFormat get_format_in( ) const;
		AVSampleFormat get_format_out( ) const;
		int get_frequency_in( ) const;
		int get_frequency_out( ) const;
		int get_channels_in( ) const;
		int get_channels_out( ) const;

		static boost::uint64_t channels_to_layout( int channels, bool point_0 = false );

	private:

		boost::shared_ptr< SwrContext > resampler_;

		int frequency_in_; 
		int channels_in_; 
		AVSampleFormat format_in_;
		int frequency_out_; 
		int channels_out_; 
		AVSampleFormat format_out_;
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

		audio_type_ptr convert( const boost::uint8_t **src, int samples );
		audio_type_ptr convert_with_offset( const boost::uint8_t **src, int samples, int offset_samples );
		audio_type_ptr convert( const audio_type_ptr& audio );

		audio::identity get_out_format( ) const;

	private:

		audio::identity format_out_;

		avaudio_resampler resampler_;
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

		boost::shared_ptr< AVFrame > convert( const boost::uint8_t **src, int samples );
		boost::shared_ptr< AVFrame > convert( const audio_type_ptr& audio );

		static void destroy_AVFrame( AVFrame* f);

	private:

		avaudio_resampler resampler_;

};

} } }

// ########################################################################################




#endif
