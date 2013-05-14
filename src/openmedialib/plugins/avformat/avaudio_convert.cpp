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
	, frequency_in_( frequency_in )
	, channels_in_( channels_in )
	, format_in_( format_in )
	, frequency_out_( frequency_out )
	, channels_out_( channels_out )
	, format_out_( format_out )
{
	int err;

	ARENFORCE_MSG( 0 == ( err = av_opt_set_int( resampler_.get(), "in_channel_count", channels_in, 0 ) ), 
				   "Error (%1%) setting in_channel_count to %2%" ) ( err ) ( channels_in );
	ARENFORCE_MSG( 0 == ( err = av_opt_set_int( resampler_.get(), "out_channel_count", channels_out, 0 ) ), 
				   "Error (%1%) setting out_channel_count to %2%" ) ( err ) ( channels_out );
	ARENFORCE_MSG( 0 == ( err = av_opt_set_int( resampler_.get(), "in_channel_layout", layout_in, 0 ) ), 
				   "Error (%1%) setting in_channel_layout to %2%" ) ( err ) ( layout_in );
	ARENFORCE_MSG( 0 == ( err = av_opt_set_int( resampler_.get(), "out_channel_layout", layout_out, 0 ) ), 
				   "Error (%1%) setting out_channel_layout to %2%" ) ( err ) ( layout_out );
	ARENFORCE_MSG( 0 == ( err = av_opt_set_int( resampler_.get(), "in_sample_rate", frequency_in, 0 ) ), 
				   "Error (%1%) setting in_sample_rate to %2%" ) ( err ) ( frequency_in );
	ARENFORCE_MSG( 0 == ( err = av_opt_set_int( resampler_.get(), "out_sample_rate", frequency_out, 0 ) ), 
				   "Error (%1%) setting out_sample_rate to %2%" ) ( err ) ( frequency_out );
	ARENFORCE_MSG( 0 == ( err = av_opt_set_sample_fmt( resampler_.get(), "in_sample_fmt", format_in, 0 ) ), 
				   "Error (%1%) setting in_sample_fmt to %2%" ) ( err ) ( format_in );
	ARENFORCE_MSG( 0 == ( err = av_opt_set_sample_fmt( resampler_.get(), "out_sample_fmt", format_out, 0 ) ), 
				   "Error (%1%) setting out_sample_fmt to %2%" ) ( err ) ( format_out );

	ARENFORCE_MSG( 0 == ( err = swr_init( resampler_.get() ) ), "Error initializing swresample (%1%)" ) ( err );

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

int avaudio_resampler::resample(	boost::uint8_t *out, 
									int out_samples, 
									const boost::uint8_t *in, 
									int in_samples)
{
	return this->resample( &out, out_samples, &in, in_samples );
}

int avaudio_resampler::drain( boost::uint8_t **out, int out_samples )
{
	return swr_convert( resampler_.get(), out, out_samples, 0, 0 );
}

int avaudio_resampler::drain( boost::uint8_t *out, int out_samples )
{
	return this->drain( &out, out_samples );
}

int avaudio_resampler::drain( int num_samples_to_drain )
{
	boost::uint8_t* tmpbuf = new boost::uint8_t[ num_samples_to_drain*channels_out_*av_get_bytes_per_sample( format_out_ ) ];
	int return_val = this->drain( &tmpbuf[0], num_samples_to_drain );
	delete [] tmpbuf;
	return return_val;

}

void avaudio_resampler::destroy( SwrContext *s )
{
	swr_free( &s );
}

int avaudio_resampler::output_samples( int in_samples ) const
{
	return avaudio_resampler::output_samples( in_samples, frequency_in_, frequency_out_ );
}

int avaudio_resampler::output_samples( int in_samples , int frequency_in, int frequency_out )
{
	return av_rescale_rnd( in_samples, frequency_out, frequency_in, AV_ROUND_DOWN );
}

int avaudio_resampler::input_samples( int out_samples ) const
{
	return avaudio_resampler::input_samples( out_samples, frequency_in_, frequency_out_ );
}

int avaudio_resampler::input_samples( int out_samples , int frequency_in, int frequency_out )
{
	return av_rescale_rnd( out_samples, frequency_in, frequency_out, AV_ROUND_DOWN );
}

AVSampleFormat avaudio_resampler::get_format_in( ) const
{
	return format_in_;
}

AVSampleFormat avaudio_resampler::get_format_out( ) const
{
	return format_out_;
}

int avaudio_resampler::get_frequency_in( ) const
{
	return frequency_in_;
}

int avaudio_resampler::get_frequency_out( ) const
{
	return frequency_out_;
}

int avaudio_resampler::get_channels_in( ) const
{
	return channels_in_;
}

int avaudio_resampler::get_channels_out( ) const
{
	return channels_out_;
}

boost::uint64_t avaudio_resampler::channels_to_layout( int channels, bool point_0 )
{
	boost::uint64_t layout = av_get_default_channel_layout( channels );

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




// ########################################################################################
// ########################################################################################

avaudio_convert_to_aml::avaudio_convert_to_aml(	int frequency_in, 
												int frequency_out, 
												int channels_in, 
												int channels_out, 
												AVSampleFormat format_in,
												audio::identity format_out )
	: format_out_( format_out )
	, resampler_ ( frequency_in, 
				   frequency_out, 
				   channels_in, 
				   channels_out, 
				   avaudio_resampler::channels_to_layout( channels_in ), 
				   avaudio_resampler::channels_to_layout( channels_out ), 
				   format_in, 
				   aml_id_to_AVSampleFormat( format_out ) )
{
}


avaudio_convert_to_aml::~avaudio_convert_to_aml( )
{
}

// ########################################################################################

audio_type_ptr avaudio_convert_to_aml::convert( const boost::uint8_t **src, int samples )
{
	int out_samples = resampler_.output_samples( samples );

	ml::audio_type_ptr result = ml::audio::allocate( format_out_ , 
													 resampler_.get_frequency_out( ), 
													 resampler_.get_channels_out( ), 
													 out_samples, 
													 false );

	boost::uint8_t *output = ( boost::uint8_t * )result->pointer( );

	resampler_.resample( &output, out_samples, src, samples );

	return result;
}

audio_type_ptr avaudio_convert_to_aml::convert_with_offset( const boost::uint8_t **src, int samples, int offset_samples )
{
	std::vector<const boost::uint8_t*> offset_src( resampler_.get_channels_in( ) );

	if ( av_sample_fmt_is_planar( resampler_.get_format_in( ) ) )
	{
		// packed. increase each plane's pointer by offset * bps
		for ( int i = 0; i < resampler_.get_channels_in( ); ++i )
		{
			offset_src[ i ] = src[ i ] + offset_samples * av_get_bytes_per_sample( resampler_.get_format_in( ) );
		}
	}
	else
	{
		// interleaved. the pointer by offset*channels*bps
		offset_src[ 0 ] = src[ 0 ] + offset_samples * av_get_bytes_per_sample( resampler_.get_format_in( ) ) * resampler_.get_channels_in( );
	}

	int out_samples = resampler_.output_samples( samples );

	ml::audio_type_ptr result = ml::audio::allocate( format_out_, 
													 resampler_.get_frequency_out( ), 
													 resampler_.get_channels_out( ), 
													 out_samples, 
													 false );

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

// ########################################################################################
// ########################################################################################

avaudio_convert_to_AVFrame::avaudio_convert_to_AVFrame(	int frequency_in, 
														int frequency_out, 
														int channels_in, 
														int channels_out, 
														AVSampleFormat format_in,
														AVSampleFormat format_out )
	: resampler_(	frequency_in, 
					frequency_out, 
					channels_in, 
					channels_out, 
					avaudio_resampler::channels_to_layout( channels_in ),
					avaudio_resampler::channels_to_layout( channels_out ), 
					format_in, 
					format_out)
{
}

avaudio_convert_to_AVFrame::~avaudio_convert_to_AVFrame( )
{
}

// ########################################################################################

boost::shared_ptr< AVFrame > avaudio_convert_to_AVFrame::convert( const boost::uint8_t **src, int samples )
{
	boost::shared_ptr< AVFrame > result = 
		boost::shared_ptr< AVFrame >( avcodec_alloc_frame( ), 
									  avaudio_convert_to_AVFrame::destroy_AVFrame );

	result->nb_samples = resampler_.output_samples( samples );
	
	int size = av_samples_get_buffer_size( NULL, 
										   resampler_.get_channels_out( ), 
										   result->nb_samples, 
										   resampler_.get_format_out( ), 
										   0 );

	boost::uint8_t *buffer = ( boost::uint8_t * )av_malloc( size );

	avcodec_fill_audio_frame( result.get(), 
							  resampler_.get_channels_out( ), 
							  resampler_.get_format_out( ), 
							  buffer, 
							  size, 
							  0 );

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

