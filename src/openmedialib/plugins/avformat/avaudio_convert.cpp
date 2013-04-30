// Audio resampler and formatting via swresampler
//
// Copyright (C) Vizrt 2013


#include <openmedialib/plugins/avformat/avaudio_convert.hpp>
#include <opencorelib/cl/enforce_defines.hpp>


namespace olib { namespace openmedialib { namespace ml {

AVSampleFormat aml_id_to_AVSampleFormat( audio::identity id)
{
	switch( id )
	{
		case audio::pcm16_id:
			return AV_SAMPLE_FMT_S16;
		case audio::pcm24_id:
		case audio::pcm32_id:
			return AV_SAMPLE_FMT_S32;
		case audio::float_id:
			return AV_SAMPLE_FMT_FLT;
	}
	
	ARENFORCE_MSG( false, "invalid audio id" );
	return AV_SAMPLE_FMT_NONE; 
}

audio::identity AVSampleFormat_to_aml_id( AVSampleFormat fmt )
{
	switch( fmt )
	{
		case AV_SAMPLE_FMT_U8:
		case AV_SAMPLE_FMT_U8P:
		case AV_SAMPLE_FMT_S16:
		case AV_SAMPLE_FMT_S16P:
			return audio::pcm16_id;

		case AV_SAMPLE_FMT_S32:
		case AV_SAMPLE_FMT_S32P:
			return audio::pcm32_id;

		case AV_SAMPLE_FMT_DBL:
		case AV_SAMPLE_FMT_FLT:
		case AV_SAMPLE_FMT_FLTP:
		case AV_SAMPLE_FMT_DBLP:
			return audio::float_id;
		case AV_SAMPLE_FMT_NONE:
		case AV_SAMPLE_FMT_NB:
			break;

	}

	ARENFORCE_MSG( false, "invalid AV_SAMPLE_FMT" );
	return audio::float_id;
}

namespace
{

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

int output_samples( int in_samples , int frequency_in, int frequency_out )
{
	return av_rescale_rnd( in_samples, frequency_out, frequency_in, AV_ROUND_DOWN );
}

}




// namespace cl = olib::opencorelib;
// namespace pl = olib::openpluginlib;
// namespace il = olib::openimagelib::il;

// ########################################################################################


avaudio_resampler::avaudio_resampler(	int frequency_in, 
										int frequency_out, 
										int channels_in, 
										int channels_out, 
										boost::uint64_t layout_in,
										boost::uint64_t layout_out,
										AVSampleFormat format_in,
										AVSampleFormat format_out )
	: resampler_( swr_alloc( ), avaudio_resampler::destroy )
{
	av_opt_set_int( resampler_.get(), "in_channel_count", channels_in, 0 );
	av_opt_set_int( resampler_.get(), "out_channel_count", channels_out, 0 );
	av_opt_set_int( resampler_.get(), "in_channel_layout", layout_in, 0 );
	av_opt_set_int( resampler_.get(), "out_channel_layout", layout_out,  0 );
	av_opt_set_int( resampler_.get(), "in_sample_rate", frequency_in, 0 );
	av_opt_set_int( resampler_.get(), "out_sample_rate", frequency_out, 0 );
	av_opt_set_sample_fmt( resampler_.get(), "in_sample_fmt", format_in, 0 );
	av_opt_set_sample_fmt( resampler_.get(), "out_sample_fmt", format_out, 0 );

	int err = swr_init( resampler_.get() );
	ARENFORCE_MSG( 0 == err, "Error initializing swresample (%1%)" ) ( err );
}

avaudio_resampler::~avaudio_resampler()
{
}

// ########################################################################################

int avaudio_resampler::resample(	boost::uint8_t **out, 
									int out_samples, 
									const boost::uint8_t **in, 
									int in_samples)
{
	return swr_convert( resampler_.get(), out, out_samples, in, in_samples );
}

void avaudio_resampler::destroy( SwrContext *s )
{
	swr_free( &s );
}


// ########################################################################################
// ########################################################################################

avaudio_convert_to_aml::avaudio_convert_to_aml(	int frequency_in, 
												int frequency_out, 
												int channels_in, 
												int channels_out, 
												AVSampleFormat format_in,
												audio::identity format_out )
	: frequency_in_( frequency_in )
	, channels_in_( channels_in )
	, format_in_( format_in )
	, frequency_out_( frequency_out )
	, channels_out_( channels_out )
	, format_out_( format_out )
	, resampler_ ( frequency_in_, frequency_out_, channels_in_, channels_out_, channels_to_layout( channels_in_ ), 
				   channels_to_layout( channels_out_ ), format_in_ , aml_id_to_AVSampleFormat( format_out_ ) )
{
}


avaudio_convert_to_aml::~avaudio_convert_to_aml( )
{
}

// ########################################################################################

audio_type_ptr avaudio_convert_to_aml::convert( const boost::uint8_t **src, int samples )
{
	int out_samples = output_samples( samples, frequency_in_, frequency_out_ );

	ml::audio_type_ptr result = ml::audio::allocate( format_out_ , frequency_out_, channels_out_, out_samples, false );

	boost::uint8_t *output = ( boost::uint8_t * )result->pointer( );

	resampler_.resample( &output, out_samples, src, samples );

	return result;
}

audio_type_ptr avaudio_convert_to_aml::convert_with_offset( const boost::uint8_t **src, int samples, int offset_samples )
{
	std::vector<const boost::uint8_t*> offset_src( channels_in_ );

	if ( av_sample_fmt_is_planar( format_in_ ) )
	{
		// packed. increase each plane's pointer by offset * bps
		for ( int i = 0; i < channels_in_; ++i )
		{
			offset_src[ i ] = src[ i ] + offset_samples * av_get_bytes_per_sample( format_in_ );
		}
	}
	else
	{
		// interleaved. the pointer by offset*channels*bps
		offset_src[ 0 ] = src[ 0 ] + offset_samples * av_get_bytes_per_sample( format_in_ ) * channels_in_;
	}

	int out_samples = output_samples( samples, frequency_in_, frequency_out_ );

	ml::audio_type_ptr result = ml::audio::allocate( format_out_, frequency_out_, channels_out_, out_samples, false );

	boost::uint8_t *output = ( boost::uint8_t * )result->pointer( );

	resampler_.resample( &output, out_samples, &offset_src[0], samples );

	return result;
}

audio_type_ptr avaudio_convert_to_aml::convert( const audio_type_ptr& audio )
{
	const boost::uint8_t *buf = ( const boost::uint8_t * )audio->pointer( );

	return this->convert( &buf, audio->samples( ) );
}

audio::identity avaudio_convert_to_aml::get_out_format() const
{
	return format_out_;
}

AVSampleFormat avaudio_convert_to_aml::get_in_format( ) const
{
	return format_in_;
}

int avaudio_convert_to_aml::get_in_frequency( ) const
{
	return frequency_in_;
}

int avaudio_convert_to_aml::get_in_channels( ) const
{
	return channels_in_;
}

// ########################################################################################
// ########################################################################################

avaudio_convert_to_AVFrame::avaudio_convert_to_AVFrame(	int frequency_in, 
														int frequency_out, 
														int channels_in, 
														int channels_out, 
														AVSampleFormat format_in,
														AVSampleFormat format_out )
	: frequency_in_( frequency_in )
	, channels_in_( channels_in )
	, format_in_( format_in )
	, frequency_out_( frequency_out )
	, channels_out_( channels_out )
	, format_out_( format_out )
	, resampler_( frequency_in_, frequency_out_, channels_in_, channels_out_, channels_to_layout( channels_in_ ),
					channels_to_layout( channels_out_ ), format_in_, format_out_)
{
}

avaudio_convert_to_AVFrame::~avaudio_convert_to_AVFrame( )
{
}

// ########################################################################################

boost::shared_ptr< AVFrame > avaudio_convert_to_AVFrame::convert( const boost::uint8_t **src, int samples )
{
	boost::shared_ptr< AVFrame > result = boost::shared_ptr< AVFrame >( avcodec_alloc_frame( ), avaudio_convert_to_AVFrame::destroy_AVFrame );
	result->nb_samples = output_samples( samples, frequency_in_, frequency_out_ );
	int size = av_samples_get_buffer_size( NULL, channels_out_, result->nb_samples, format_out_, 0 );
	boost::uint8_t *buffer = ( boost::uint8_t * )av_malloc( size );
	avcodec_fill_audio_frame( result.get(), channels_out_, format_out_, buffer, size, 0 );
	resampler_.resample( result->data, result->nb_samples, src, samples );
	return result;
}

boost::shared_ptr< AVFrame > avaudio_convert_to_AVFrame::convert( const audio_type_ptr& audio )
{
	if ( audio )
	{
		const boost::uint8_t *buf = ( const boost::uint8_t * )audio->pointer( );

		return convert( &buf, audio->samples( ) );
	}
	return boost::shared_ptr< AVFrame >( );
}

void avaudio_convert_to_AVFrame::destroy_AVFrame( AVFrame* f )
{
	av_freep( f->data );
	avcodec_free_frame( &f );
}

// ########################################################################################

} } }

