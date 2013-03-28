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

namespace olib { namespace openmedialib { namespace ml {

namespace cl = olib::opencorelib;
namespace pl = olib::openpluginlib;
namespace il = olib::openimagelib::il;

class avaudio_filter
{
	public:
		avaudio_filter( ml::audio_type_ptr input = ml::audio_type_ptr( ), bool point_0 = false )
		: resampler_( swr_alloc( ) )
		, frequency_in_( 0 )
		, channels_in_( 0 )
		, format_in_( AV_SAMPLE_FMT_NONE )
		, layout_in_( 0 )
		, frequency_out_( 0 )
		, channels_out_( 0 )
		, format_out_( AV_SAMPLE_FMT_NONE )
		, layout_out_( 0 )
		{
			set_passthrough( input, point_0 );
		}

		avaudio_filter( AVCodecContext *codec, bool point_0 = false )
		: resampler_( swr_alloc( ) )
		, frequency_in_( 0 )
		, channels_in_( 0 )
		, format_in_( AV_SAMPLE_FMT_NONE )
		, layout_in_( 0 )
		, frequency_out_( 0 )
		, channels_out_( 0 )
		, format_out_( AV_SAMPLE_FMT_NONE )
		, layout_out_( 0 )
		{
			set_passthrough( codec, point_0 );
		}

		~avaudio_filter( )
		{
			swr_free( &resampler_ );
		}

		void set_passthrough( ml::audio_type_ptr input, bool point_0 = false )
		{
			if ( input )
			{
				frequency_in_ = input->frequency( );
				channels_in_ = input->channels( );
				format_in_ = aml_to_avformat( input->id( ) );
				layout_in_ = channels_to_layout( channels_in_, point_0 );
				set_passthrough( );
			}
		}

		bool changed( ml::audio_type_ptr input, bool point_0 = false )
		{
			return input->frequency( ) != frequency_in_ || 
				   input->channels( ) != channels_in_ || 
				   aml_to_avformat( input->id( ) ) != format_in_ || 
				   channels_to_layout( input->channels( ), point_0 ) != layout_in_;
		}

		void set_passthrough( AVCodecContext *codec, bool point_0 = false )
		{
			if ( codec )
			{
				frequency_in_ = codec->sample_rate;
				channels_in_ = codec->channels;
				format_in_ = codec->sample_fmt;
				layout_in_ = point_0 || codec->channel_layout == 0 ? channels_to_layout( channels_in_, point_0 ) : codec->channel_layout;
				set_passthrough( );
			}
		}

		void set_passthrough( int frequency, int channels, ml::audio::identity id, bool point_0 = false )
		{
			frequency_in_ = frequency;
			channels_in_ = channels;
			format_in_ = aml_to_avformat( id );
			layout_in_ = channels_to_layout( channels_in_, point_0 );
			set_passthrough( );
		}

		void set_passthrough( )
		{
			frequency_out_ = frequency_in_;
			channels_out_ = channels_in_;
			format_out_ = format_in_;
			layout_out_ = layout_in_;
			setup( );
		}

		void set_output( AVCodecContext *codec, bool point_0 = false )
		{
			frequency_out_ = codec->sample_rate;
			channels_out_ = codec->channels;
			format_out_ = codec->sample_fmt;
			layout_out_ = channels_to_layout( channels_out_, point_0 );
			setup( );
		}

		void set_frequency( int frequency )
		{
			frequency_out_ = frequency == -1 ? frequency_in_ : frequency;
			setup( );
		}

		void set_channels( int channels, bool point_0 = false )
		{
			channels_out_ = channels == -1 ? channels_in_ : channels;
			layout_out_ = channels_to_layout( channels_out_, point_0 );
			setup( );
		}

		void set_format( AVSampleFormat format )
		{
			format_out_ = format;
			setup( );
		}

		void set_id( ml::audio::identity id )
		{
			format_out_ = aml_to_avformat( id );
			setup( );
		}

		void set_aml_format( )
		{
			switch( format_in_ )
			{
				case AV_SAMPLE_FMT_U8:
				case AV_SAMPLE_FMT_U8P:
				case AV_SAMPLE_FMT_S16:
				case AV_SAMPLE_FMT_S16P:
					format_out_ = AV_SAMPLE_FMT_S16;
					break;

				case AV_SAMPLE_FMT_S32:
				case AV_SAMPLE_FMT_S32P:
					format_out_ = AV_SAMPLE_FMT_S32;
					break;

				case AV_SAMPLE_FMT_DBL:
				case AV_SAMPLE_FMT_FLT:
				case AV_SAMPLE_FMT_FLTP:
				case AV_SAMPLE_FMT_DBLP:
					format_out_ = AV_SAMPLE_FMT_FLT;
					break;

				case AV_SAMPLE_FMT_NONE:
				case AV_SAMPLE_FMT_NB:
					break;
			}
			setup( );
		}

		int bps( )
		{
			switch( format_out_ )
			{
				case AV_SAMPLE_FMT_U8:
				case AV_SAMPLE_FMT_U8P:
					return sizeof( boost::int8_t );

				case AV_SAMPLE_FMT_S16:
				case AV_SAMPLE_FMT_S16P:
					return sizeof( boost::int16_t );

				case AV_SAMPLE_FMT_S32:
				case AV_SAMPLE_FMT_S32P:
					return sizeof( boost::int32_t );

				case AV_SAMPLE_FMT_FLT:
				case AV_SAMPLE_FMT_FLTP:
					return sizeof( float );

				case AV_SAMPLE_FMT_DBL:
				case AV_SAMPLE_FMT_DBLP:
					return sizeof( double );

				case AV_SAMPLE_FMT_NONE:
				case AV_SAMPLE_FMT_NB:
					return 0;
			}

			return 0;
		}

		const std::wstring af( )
		{
			switch( format_out_ )
			{
				case AV_SAMPLE_FMT_S16:
					return ml::audio::FORMAT_PCM16;

				case AV_SAMPLE_FMT_S32:
					return ml::audio::FORMAT_PCM32;

				case AV_SAMPLE_FMT_FLT:
					return ml::audio::FORMAT_FLOAT;

				default:
					break;
			}

			return L"";
		}

		ml::audio_type_ptr convert( ml::audio_type_ptr audio )
		{
			ml::audio_type_ptr temp = aml_to_avaudio( audio );
			const boost::uint8_t *buf = ( const boost::uint8_t * )temp->pointer( );
			return ml::audio::coerce( audio->af( ), convert( &buf, temp->samples( ) ) );
		}

		ml::audio_type_ptr convert( const boost::uint8_t **buf, int samples )
		{
			int out_samples = output_samples( samples );
			ml::audio_type_ptr result = avaudio_to_aml( format_out_, frequency_out_, channels_out_, out_samples );
			boost::uint8_t *output = ( boost::uint8_t * )result->pointer( );
			out_samples = swr_convert( resampler_, &output, out_samples, buf, samples );
			return result;
		}

		AVFrame *convert( ml::audio_type_ptr audio, int &size )
		{
			if ( audio )
			{
				ml::audio_type_ptr temp = aml_to_avaudio( audio );
				const boost::uint8_t *buf = ( const boost::uint8_t * )temp->pointer( );
				return convert( &buf, audio->samples( ), size );
			}
			return 0;
		}

		AVFrame *convert( const boost::uint8_t **buf, int samples, int &size )
		{
			AVFrame *result = avcodec_alloc_frame( );
			result->nb_samples = output_samples( samples );
			size = av_samples_get_buffer_size( NULL, channels_out_, result->nb_samples, format_out_, 0 );
			boost::uint8_t *buffer = ( boost::uint8_t * )av_malloc( size );
			avcodec_fill_audio_frame( result, channels_out_, format_out_, buffer, size, 0 );
			result->extended_data = ( boost::uint8_t ** )buffer;
			swr_convert( resampler_, result->data, result->nb_samples, buf, samples );
			return result;
		}

	private:
		void setup( )
		{
			av_opt_set_int( resampler_, "in_channel_count", channels_in_, 0 );
			av_opt_set_int( resampler_, "out_channel_count", channels_out_, 0 );
			av_opt_set_int( resampler_, "in_channel_layout", layout_in_, 0 );
			av_opt_set_int( resampler_, "out_channel_layout", layout_out_,  0 );
			av_opt_set_int( resampler_, "in_sample_rate", frequency_in_, 0 );
			av_opt_set_int( resampler_, "out_sample_rate", frequency_out_, 0 );
			av_opt_set_sample_fmt( resampler_, "in_sample_fmt", format_in_, 0 );
			av_opt_set_sample_fmt( resampler_, "out_sample_fmt", format_out_, 0 );
			swr_init( resampler_ );
		}

		int output_samples( int samples )
		{
			int delay = swr_get_delay( resampler_, frequency_in_ ) + samples;
			delay = samples;
			return av_rescale_rnd( delay, frequency_out_, frequency_in_, AV_ROUND_DOWN );
		}

		AVSampleFormat aml_to_avformat( ml::audio::identity id )
		{
			AVSampleFormat format = AV_SAMPLE_FMT_NONE;

			switch( id )
			{
				case ml::audio::pcm8_id:
				case ml::audio::pcm16_id:
					format = AV_SAMPLE_FMT_S16;
					break;

				case ml::audio::pcm24_id:
				case ml::audio::pcm32_id:
					format = AV_SAMPLE_FMT_S32;
					break;

				case ml::audio::float_id:
					format = AV_SAMPLE_FMT_FLT;
					break;
			}

			return format;
		}

		ml::audio_type_ptr aml_to_avaudio( ml::audio_type_ptr audio )
		{
			ml::audio_type_ptr result = audio;

			switch( audio->id( ) )
			{
				case ml::audio::pcm8_id:
					result = ml::audio::coerce( ml::audio::FORMAT_PCM16, audio );
					break;

				case ml::audio::pcm24_id:
					result = ml::audio::coerce( ml::audio::FORMAT_PCM32, audio );
					break;

				case ml::audio::pcm16_id:
				case ml::audio::pcm32_id:
				case ml::audio::float_id:
					break;
			}

			return result;
		}

		ml::audio_type_ptr avaudio_to_aml( AVSampleFormat format, int frequency, int channels, int samples )
		{
			ml::audio_type_ptr result;

			switch( format )
			{
				case AV_SAMPLE_FMT_S16:
					result = ml::audio::allocate( ml::audio::FORMAT_PCM16, frequency, channels, samples, false );
					break;

				case AV_SAMPLE_FMT_S32:
					result = ml::audio::allocate( ml::audio::FORMAT_PCM32, frequency, channels, samples, false );
					break;

				case AV_SAMPLE_FMT_FLT:
					result = ml::audio::allocate( ml::audio::FORMAT_FLOAT, frequency, channels, samples, false );
					break;

				default:
					break;
			}

			return result;
		}

		boost::uint64_t channels_to_layout( int channels, bool point_0 = false )
		{
			boost::uint64_t layout = AV_CH_LAYOUT_NATIVE;

			switch( channels )
			{
				case 1:
					layout = AV_CH_LAYOUT_MONO;
					break;
				case 2:
					layout = AV_CH_LAYOUT_STEREO;
					break;
				case 3:
					layout = !point_0 ? AV_CH_LAYOUT_2POINT1 : AV_CH_LAYOUT_SURROUND;
					break;
				case 4:
					layout = !point_0 ? AV_CH_LAYOUT_3POINT1 : AV_CH_LAYOUT_4POINT0;
					break;
				case 5:
					layout = !point_0 ? AV_CH_LAYOUT_4POINT1 : AV_CH_LAYOUT_5POINT0;
					break;
				case 6:
					layout = !point_0 ? AV_CH_LAYOUT_5POINT1 : AV_CH_LAYOUT_6POINT0;
					break;
				case 7:
					layout = !point_0 ? AV_CH_LAYOUT_6POINT1 : AV_CH_LAYOUT_7POINT0;
					break;
				case 8:
					layout = !point_0 ? AV_CH_LAYOUT_7POINT1 : AV_CH_LAYOUT_OCTAGONAL;
					break;
			}

			return layout;
		}

		SwrContext *resampler_;
		int frequency_in_;
		int channels_in_;
		AVSampleFormat format_in_;
		boost::uint64_t layout_in_;
		int frequency_out_;
		int channels_out_;
		AVSampleFormat format_out_;
		boost::uint64_t layout_out_;
};

} } }

#endif
