// Audio resampler and formatting via swresampler
//
// Copyright (C) Vizrt 2013


#include <openmedialib/plugins/avformat/avaudio_base.hpp>
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

	}

}

int aml_id_to_bytes_per_sample( audio::identity id )
{
	switch ( id )
	{
		case audio::pcm16_id:
			return sizeof( boost::int16_t );

		case audio::pcm32_id:
			return sizeof( boost::int32_t );

		case audio::float_id:
			return sizeof( float );
	}
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

audio::identity aml_id_to_AVcompatible_aml_id( audio::identity id)
{
	return AVSampleFormat_to_aml_id( aml_id_to_AVSampleFormat( id ) );
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
	: resampler_( swr_alloc( ) )
{
	av_opt_set_int( resampler_, "in_channel_count", channels_in, 0 );
	av_opt_set_int( resampler_, "out_channel_count", channels_out, 0 );
	av_opt_set_int( resampler_, "in_channel_layout", layout_in, 0 );
	av_opt_set_int( resampler_, "out_channel_layout", layout_out,  0 );
	av_opt_set_int( resampler_, "in_sample_rate", frequency_in, 0 );
	av_opt_set_int( resampler_, "out_sample_rate", frequency_out, 0 );
	av_opt_set_sample_fmt( resampler_, "in_sample_fmt", format_in, 0 );
	av_opt_set_sample_fmt( resampler_, "out_sample_fmt", format_out, 0 );

	swr_init( resampler_ );
}

avaudio_resampler::~avaudio_resampler()
{
	swr_free( &resampler_ );
}

// ########################################################################################

int avaudio_resampler::resample(	boost::uint8_t **out, 
									int out_samples, 
									const boost::uint8_t **in, 
									int in_samples)
{
	return swr_convert( resampler_, out, out_samples, in, in_samples );
}



// ########################################################################################
// ########################################################################################

avaudio_convert_to_aml::avaudio_convert_to_aml(	int frequency_in, 
												int frequency_out, 
												int channels_in, 
												int channels_out, 
												AVSampleFormat format_in,
												audio::identity format_out )
	: resampler_( 0 )
	, frequency_in_( frequency_in )
	, channels_in_( channels_in )
	, format_in_( format_in )
	, frequency_out_( frequency_out )
	, channels_out_( channels_out )
	, format_out_( format_out )
{
	resampler_ = new avaudio_resampler( frequency_in_,
										frequency_out_, 
										channels_in_,
										channels_out_,
										channels_to_layout( channels_in_ ),
										channels_to_layout( channels_out_ ),
										format_in_ , 
										aml_id_to_AVSampleFormat( format_out_ ) );
}


avaudio_convert_to_aml::~avaudio_convert_to_aml( )
{
	if ( resampler_ )
	{
		delete resampler_;
	}
}

// ########################################################################################

audio_type_ptr avaudio_convert_to_aml::convert( const boost::uint8_t **src, int samples )
{
	int out_size;
	return this->convert( src, samples, out_size );
}

audio_type_ptr avaudio_convert_to_aml::convert( const boost::uint8_t **src, int samples, int &out_size )
{
	int out_samples = output_samples( samples, frequency_in_, frequency_out_ );

	out_size = out_samples * channels_out_ * aml_id_to_bytes_per_sample( format_out_ );

	ml::audio_type_ptr result = ml::audio::allocate( aml_id_to_AVcompatible_aml_id( format_out_ ), frequency_out_, channels_out_, out_samples, false );

	boost::uint8_t *output = ( boost::uint8_t * )result->pointer( );

	resampler_->resample( &output, out_samples, src, samples );

	return ml::audio::coerce( format_out_, result );
}

audio_type_ptr avaudio_convert_to_aml::convert_with_offset( const boost::uint8_t **src, int samples, int offset_samples )
{
//     const boost::uint8_t **offset_src;
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

	ml::audio_type_ptr result = ml::audio::allocate( aml_id_to_AVcompatible_aml_id( format_out_ ), frequency_out_, channels_out_, out_samples, false );

	boost::uint8_t *output = ( boost::uint8_t * )result->pointer( );

	resampler_->resample( &output, out_samples, &offset_src[0], samples );

	return ml::audio::coerce( format_out_, result );
}

audio_type_ptr avaudio_convert_to_aml::convert( const audio_type_ptr& audio )
{
	ml::audio_type_ptr temp = ml::audio::coerce( aml_id_to_AVcompatible_aml_id( audio->id( ) ), audio );

	const boost::uint8_t *buf = ( const boost::uint8_t * )temp->pointer( );

	return this->convert( &buf, temp->samples( ) );
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
	: resampler_( 0 )
	, frequency_in_( frequency_in )
	, channels_in_( channels_in )
	, format_in_( format_in )
	, frequency_out_( frequency_out )
	, channels_out_( channels_out )
	, format_out_( format_out )
{
	resampler_ = new avaudio_resampler( frequency_in_,
										frequency_out_, 
										channels_in_,
										channels_out_,
										channels_to_layout( channels_in_ ),
										channels_to_layout( channels_out_ ),
										format_in_,
										format_out_);
}

avaudio_convert_to_AVFrame::~avaudio_convert_to_AVFrame( )
{
	if ( resampler_ )
	{
		delete resampler_;
	}
}

// ########################################################################################

AVFrame *avaudio_convert_to_AVFrame::convert( const boost::uint8_t **src, int samples )
{
	AVFrame *result = avcodec_alloc_frame( );
	result->nb_samples = output_samples( samples, frequency_in_, frequency_out_ );
	int size = av_samples_get_buffer_size( NULL, channels_out_, result->nb_samples, format_out_, 0 );
	boost::uint8_t *buffer = ( boost::uint8_t * )av_malloc( size );
	avcodec_fill_audio_frame( result, channels_out_, format_out_, buffer, size, 0 );
	result->extended_data = ( boost::uint8_t ** )buffer;
	resampler_->resample( result->data, result->nb_samples, src, samples );
	return result;
}

AVFrame *avaudio_convert_to_AVFrame::convert( const audio_type_ptr& audio )
{
	if ( audio )
	{
		ml::audio_type_ptr temp = ml::audio::coerce( aml_id_to_AVcompatible_aml_id( audio->id( ) ), audio );
		const boost::uint8_t *buf = ( const boost::uint8_t * )temp->pointer( );

		return convert( &buf, temp->samples( ) );
	}
	return 0;
}

// ########################################################################################

} } }

