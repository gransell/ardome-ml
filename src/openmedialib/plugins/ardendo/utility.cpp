// General purpose utility functionality
//
// Copyright (C) 2007 Ardendo
#include "precompiled_headers.hpp"
#include "amf_filter_plugin.hpp"
#include "utility.hpp"
#include <openmedialib/ml/packet.hpp>
#include <cmath>

#include <iostream>
#include <sstream>
using namespace std;

namespace aml { namespace openmedialib {

boost::int64_t parse_int64( const std::string &str, int base )
{
	char *end_of_array = 0;
	
#	ifdef WIN32
	boost::int64_t v = _strtoi64( &str[0], &end_of_array, base );
#	else
	boost::int64_t v = strtoll( &str[0], &end_of_array, base );
#	endif

	return v;
}

boost::uint64_t parse_uint64( const std::string &str, int base )
{
	char *end_of_array = 0;
	
#	ifdef WIN32
	boost::uint64_t v = _strtoui64( &str[0], &end_of_array, base );
#	else
	boost::uint64_t v = strtoull( &str[0], &end_of_array, base );
#	endif

	return v;
}

static const float min_short = float( ( std::numeric_limits< short >::min )( ) );
static const float max_short = float( ( std::numeric_limits< short >::max )( ) );

ml::audio_type_ptr change_pitch( ml::audio_type_ptr audio, int required )
{
	int channels = audio->channels( );
	int frequency = audio->frequency( );

	ml::audio_type_ptr output = ml::audio_type_ptr( new ml::audio_type( ml::pcm16_audio_type( frequency, channels, required ) ) );

	short *dst = ( short * )output->data( );
	short *src = ( short * )audio->data( );
	double ratio = double( audio->samples( ) ) / required;
	int upper = channels * ( audio->samples( ) - 1 );

	for ( int i = 0; i < required; i ++ )
	{
		for ( int c = 0; c < channels; c ++ )
		{
			int offset = channels * int( i * ratio ) + c;
			if ( offset < upper )
				*dst ++ = short( ( int( src[ offset ] ) + src[ offset + channels ] ) / 2 );
			else
				*dst ++ = src[ offset ];
		}
	}

	return output;
}

void apply_volume( ml::frame_type_ptr &result, double volume, double end )
{
	if ( result && result->get_audio( ) )
	{
		// Get the audio object
		ml::audio_type_ptr audio = result->get_audio( );

		// Extract samples, number of channels and data
		int samples = audio->samples( );
		int channels = audio->channels( );
		short *data = ( short * )audio->data( );

		// Assign initial volume and increment value (assume fixed for now)
		double increment = ( end - volume ) / samples;

		const double cutoff_to_fs_ratio	= 0.5;
		const double exponent = exp( -2.0 * M_PI * cutoff_to_fs_ratio );
		const double minus_exponent	= 1 - exponent;

		float sum = 0.0f;
		short clipped = 0;

		// Special case for first sample
		for ( int c = 0; c < channels; c ++ )
		{
			// Apply the volume
			sum = static_cast<float>( volume * *data );

			// Clip channels appropriately
			clipped = short( CLAMP( sum, min_short, max_short ) );

			// Low pass filter to counter any effects from clipping
			*data ++ = short( minus_exponent * clipped );
		}

		// Mangle the audio samples
		for( int index = 1; index < samples; index ++ )
		{
			volume += increment;

			for ( int c = 0; c < channels; c ++ )
			{
				// Apply the volume
				sum = static_cast<float>( volume * *data );

				// Clip channels appropriately
				clipped = short( CLAMP( sum, min_short, max_short ) );

				// Low pass filter to counter any effects from clipping
				*data = short( minus_exponent * clipped + exponent * *( data - channels ) );
				data ++;
			}
		}
	}
}

void extract_channel( ml::frame_type_ptr &result, int channel )
{
	ml::audio_type_ptr audio;

	ml::audio_type_ptr input = result->get_audio( );

	int position = result->get_position( );
	int frequency = 48000;
	int channels = 2;
	int samples = 0;

	if ( input )
	{
		frequency = input->frequency( );
		channels = input->channels( );
		samples = input->samples( );
	}
	else
	{
		int num = result->get_fps_num( );
		int den = result->get_fps_den( );
		samples = ml::audio_samples_for_frame( position, frequency, num, den );
	}

	audio = ml::audio_type_ptr(new ml::audio_type(ml::pcm16_audio_type( frequency, 1, samples )));

	if ( input && channel < channels )
	{
		short *src = reinterpret_cast< short * >( input->data( ) ) + channel;
		short *dst = reinterpret_cast< short * >( audio->data( ) );
		while( samples -- )
		{
			*dst ++ = *src;
			src += channels;
		}
	}
	else
	{
		memset( audio->data( ), 0, audio->size( ) );
	}

	result->set_audio( audio );
}

std::vector< double > extract_levels( ml::frame_type_ptr &result )
{
	ml::audio_type_ptr input = result->get_audio( );
	std::vector< double > levels;

	if ( input )
	{
		int channels = input->channels( );
		std::vector< int > sum( channels, 0 );
		int samples = input->samples( );
		short *src = reinterpret_cast< short * >( input->data( ) );

		while( samples -- )
			for ( int c = 0; c < channels; c++ )
				sum[ c ] = ( sum[ c ] + std::abs( *src ++ ) ) / 2;

		for ( int c = 0; c < channels; c++ )
			levels.push_back( double( sum[ c ] ) / max_short );
	}

	return levels;
}

void change_pitch( ml::frame_type_ptr &result, int required )
{
	result->set_audio( change_pitch( result->get_audio( ), required ) );
}

/**
 * Takes sound data from one input channel and spreads it to the output channels according to a
 * volume vector.  The sound is added to any sound that is allready present in the output channels.
 * \param result container for the output channels.
 * \param channel container for the single input channel.
 * \param volume how loud this input channel should be on each output channel (linear).
 * \param max_level the peak level from the input.
 */
bool mix_channel( ml::frame_type_ptr &result, ml::frame_type_ptr &channel, const std::vector< double > &volume, double &max_level, int mute )
{
	ml::audio_type_ptr output = result->get_audio( );
	ml::audio_type_ptr input = channel->get_audio( );

	// We'll report the max sample here
	max_level = 0.0;
	short max_sample = 0;

	// Shouldn't happen - precondition is that channel provides audio
	if ( input == 0 )
		return false;

	// Again, shouldn't happen - all inputs should be channel split
	if ( input->channels( ) != 1 )
	{
		input = ml::audio_channel_convert( input, 1 );
		if ( input == 0 )
			return false;
	}

	// Valid case - on first usage, there won't be an output audio, so create it to match the first channel input
	if ( output == 0 )
	{
		const int frequency = input->frequency( );
		const int samples = input->samples( );
		output = ml::audio_type_ptr( 
            new ml::audio_type( ml::pcm16_audio_type( frequency, 
                    static_cast<ml::pcm16_audio_type::size_type>(volume.size( )), samples ) ) );
		memset( output->data( ), 0, output->size( ) );
	}

	// Should never happen, but can accept if it does
	if ( size_t( output->channels( ) ) != volume.size( ) )
	{
        PL_LOG(pl::level::warning, "Channels doesn't match volumes in mix_channel");
		output = ml::audio_channel_convert( input, static_cast<int>(volume.size( )) );
		if ( output == 0 )
			return false;
	}

	// Do I reject or do a rough resample?
	// For now, we'll do a pitch shift
	if ( input->frequency( ) != output->frequency( ) )
	{
	}

	// In theory, this shouldn't happen, but just in case, force samples to match
	if ( input->samples( ) != output->samples( ) )
	{
		input = change_pitch( input, output->samples( ) );
	}

	// Extract samples, number of channels and data
	int samples = input->samples( );
	int channels = output->channels( );
	short *dst = ( short * )output->data( );
	short *src = ( short * )input->data( );

	const double cutoff_to_fs_ratio	= 0.5;
	const double exponent = exp( -2.0 * M_PI * cutoff_to_fs_ratio );
	const double minus_exponent	= 1 - exponent;

	double sum = 0.0;
	short clipped = 0;

	int solo_channel = -1;
	bool solo = true;

	for ( size_t index = 0; index < volume.size( ); index ++ )
	{
		if ( volume[ index ] != 0.0 )
		{
			if ( solo_channel == -1 )
				solo_channel = int( index );
			else
				solo = false;
		}
	}

	if ( !solo )
	{
		// Get the max sample value now
		max_sample = std::abs( *src ) > max_sample ? std::abs( *src ) : max_sample;
	
		// Special case for first sample
		for ( int c = 0; c < channels; c ++ )
		{
			if ( !mute && volume[ c ] != 0.0 )
			{
				// Apply the volume
				sum =  double( *dst + volume[ c ] * *src );
	
				// Clip channels appropriately
				clipped = short( CLAMP( sum, min_short, max_short ) );
	
				// Low pass filter to counter any effects from clipping
				*dst = short( minus_exponent * clipped + exponent * *dst );
			}

			dst ++;
		}
	
		// Mangle the audio samples
		for( int index = 1; index < samples; index ++ )
		{
			src ++;
	
			// Get the max sample value now
			max_sample = std::abs( *src ) > max_sample ? std::abs( *src ) : max_sample;
	
			for ( int c = 0; c < channels; c ++ )
			{
				if ( !mute && volume[ c ] != 0.0 )
				{
					// Apply the volume
					sum = double( *dst + volume[ c ] * *src );
	
					// Clip channels appropriately
					clipped = short( CLAMP( sum, min_short, max_short ) );
	
					// Low pass filter to counter any effects from clipping
					*dst = short( minus_exponent * clipped + exponent * *( dst - channels ) );
				}

				dst ++;
			}
		}
	}
	else if ( solo_channel >= 0 && solo_channel < channels )
	{
		double vol = volume[ solo_channel ];

		// Offset to first sample
		dst += solo_channel;

		// Get the max sample value now
		max_sample = std::abs( *src ) > max_sample ? std::abs( *src ) : max_sample;
	
		if ( !mute )
		{
			// Apply the volume
			sum =  double( *dst + vol * *src );
	
			// Clip channels appropriately
			clipped = short( CLAMP( sum, min_short, max_short ) );
	
			// Low pass filter to counter any effects from clipping
			*dst = short( minus_exponent * clipped + exponent * *dst );
		}
	
		// Mangle the audio samples
		for( int index = 1; index < samples; index ++ )
		{
			src ++;
			dst += channels;
	
			// Get the max sample value now
			max_sample = std::abs( *src ) > max_sample ? std::abs( *src ) : max_sample;
	
			if ( !mute )
			{
				// Apply the volume
				sum = double( *dst + vol * *src );
	
				// Clip channels appropriately
				clipped = short( CLAMP( sum, min_short, max_short ) );
	
				// Low pass filter to counter any effects from clipping
				*dst = short( minus_exponent * clipped + exponent * *( dst - channels ) );
			}
		}
	}

	result->set_audio( output );

	// Calculate the max level
	max_level = double( max_sample ) / 32768.0;

	return true;
}

static pl::pcos::key key_peaks_( pcos::key::from_string( "peaks" ) );

void join_peaks( ml::frame_type_ptr &result, std::vector< double > &max_levels )
{
	if ( max_levels.size( ) < size_t( result->get_audio( )->channels( ) ) )
	{
		for( size_t i = max_levels.size( ); i < size_t( result->get_audio( )->channels( ) ); i ++ )
			max_levels.push_back( double( 0.0 ) );
	}

	if ( !result->properties( ).get_property_with_key( key_peaks_ ).valid( ) )
	{
		pcos::property frame_peaks( key_peaks_ );
		result->properties( ).append( frame_peaks = max_levels );
	}
	else
	{
		pcos::property frame_peaks = result->properties( ).get_property_with_key( key_peaks_ );
		std::vector< double > current = frame_peaks.value< std::vector< double > >( );
		for ( std::vector< double >::iterator iter = max_levels.begin( ); iter != max_levels.end( ); iter ++ )
			current.push_back( *iter );
		frame_peaks = current;
	}
}

void join_peaks( ml::frame_type_ptr &result, ml::frame_type_ptr &input )
{
	if ( input->properties( ).get_property_with_key( key_peaks_ ).valid( ) )
	{
		std::vector< double > levels = input->properties( ).get_property_with_key( key_peaks_ ).value< std::vector< double > >( );
		join_peaks( result, levels );
	}
}

void copy_plane( il::image_type_ptr output, il::image_type_ptr input, size_t plane )
{
	boost::uint8_t *dst = output->data( plane );
	boost::uint8_t *src = input->data( plane );
	int w = output->width( plane );
	int h = output->height( plane );
	int dst_p = output->pitch( plane );
	int src_p = input->pitch( plane );

	while( h -- )
	{
		memcpy( dst, src, w );
		dst += dst_p;
		src += src_p;
	}
}

void fill_plane( il::image_type_ptr img, size_t plane, boost::uint8_t sample )
{
	boost::uint8_t *ptr = img->data( plane );
	int w = img->width( plane );
	int h = img->height( plane );
	int p = img->pitch( plane );

	while( h -- )
	{
		memset( ptr, sample, w );
		ptr += p;
	}
}

void report_frame( std::ostream &stream, const ml::frame_type_ptr &frame )
{
	int num, den;
	frame->get_fps( num, den );
	stream << "Frame Report" << endl << endl;
	stream << "Position    : " << frame->get_position( ) << endl;
	if ( frame->has_image( ) )
		stream << "Image Info  : pf = " << pl::to_string( frame->pf( ) ) << ", codec = " << frame->video_codec( ) << ", size = " << frame->width( ) << "x" << frame->height( ) << ", sar = " << frame->get_sar_num( ) << ":" << frame->get_sar_den( ) << std::endl;
	if ( frame->has_audio( ) )
		stream << "Audio Info  : codec = " << frame->audio_codec( ) << ", samples = " << frame->samples( ) << ", frequency = "  << frame->frequency( ) << ", channels = " << frame->channels( ) << std::endl;
	if ( frame->get_stream( ) )
		stream << "Has Stream  : Yes, pf = " << pl::to_string( frame->get_stream( )->pf( ) ) << " key = " << frame->get_stream( )->key( ) << ", position = " << frame->get_stream( )->position( ) << ", bitrate = " << frame->get_stream( )->bitrate( ) << endl;
	else
		stream << "Has Stream  : No" << endl;

	if ( frame->get_image( ) )
		stream << "Has Image   : Yes, position = " << frame->get_image( )->position( ) << endl;
	else
		stream << "Has Image   : No" << endl;

	if ( frame->get_audio( ) )
		stream << "Has Audio   : Yes, position = " << frame->get_audio( )->position( ) << endl;
	else
		stream << "Has Audio   : No" << endl;

	stream << "Has Alpha   : " << ( frame->get_alpha( ) ? "Yes" : "No" ) << endl;
	stream << "Frame Rate  : " << frame->fps( ) << " (" << num << ":" << den << ")" << endl;
	stream << endl;
}

void report_image( std::ostream &stream, const il::image_type_ptr &img, int num, int den )
{
	stream << "Image Report" << endl << endl;

	if ( img )
	{
		double ar = double( img->width( ) * num ) / ( img->height( ) * den );

		const char *type = "Progressive";
		if ( img->field_order( ) == il::top_field_first )
			type = "Interlaced (top field first)";
		else if ( img->field_order( ) == il::bottom_field_first )
			type = "Interlaced (bottom field first)";

		stream << "Colour Space: " << pl::to_string( img->pf( ) ) << endl;
		stream << "Aspect Ratio: " << ar << " (" << num << ":" << den << ")" << endl;
		stream << "Type        : " << type << endl;
		stream << "Planes      : " << img->plane_count( ) << endl;

		for ( int p = 0; p < img->plane_count( ); p ++ )
		{
			int w = img->width( p );
			int h = img->height( p );
			int s = img->pitch( p );
			stream << "Dimensions " << p << ": " << w << "x" << h << "@" << s << endl;
		}
	}
	else
	{
		stream << "None." << endl;
	}

	stream << endl;
}

void report_alpha( std::ostream &stream, const il::image_type_ptr img )
{
	stream << "Alpha Report" << endl << endl;

	if ( img )
	{
		stream << "Colour Space: " << pl::to_string( img->pf( ) ) << endl;
		stream << "Planes      : " << img->plane_count( ) << endl;

		for ( int p = 0; p < img->plane_count( ); p ++ )
		{
			int w = img->width( p );
			int h = img->height( p );
			int s = img->pitch( p );
			stream << "Dimensions " << p << ": " << w << "x" << h << "@" << s << endl;
		}
	}
	else
	{
		stream << "None." << endl;
	}

	stream << endl;
}

void report_audio( std::ostream &stream, const ml::audio_type_ptr &audio )
{
	stream << "Audio Report" << endl << endl;

	if ( audio )
	{
		stream << "Format      : " << pl::to_string( audio->af( ) ) << endl;
		stream << "Frequency   : " << audio->frequency( ) << endl;
		stream << "Channels    : " << audio->channels( ) << endl;
		stream << "Samples     : " << audio->samples( ) << endl;
	}
	else
	{
		stream << "None." << endl;
	}

	stream << endl;
}

void to_stream( std::ostream &stream, const std::string &name, std::vector< double > l )
{
	stream << name << "=";
	for ( vector< double >::iterator iter = l.begin( ); iter != l.end( ); iter ++ )
		stream << *iter << " ";
	stream << endl;
}

void to_stream( std::ostream &stream, const std::string &name, std::vector< int > l )
{
	stream << name << "=";
	for ( vector< int >::iterator iter = l.begin( ); iter != l.end( ); iter ++ )
		stream << *iter << " ";
	stream << endl;
}

void report_props( std::ostream &stream, const pl::pcos::property_container &props )
{
	stream << "Properties Report" << endl << endl;

	// Obtain the keys on the filter
	pcos::key_vector keys = props.get_keys( );

	if ( keys.size( ) )
	{
		// For each key...
		for( pcos::key_vector::iterator it = keys.begin( ); it != keys.end( ); it ++ )
		{
			std::string name( ( *it ).as_string( ) );
			pcos::property p = props.get_property_with_key( *it );
			if ( p.is_a< double >( ) )
				stream << name << "=" << p.value< double >( ) << endl;
			else if ( p.is_a< int >( ) )
				stream << name << "=" << p.value< int >( ) << endl;
			else if ( p.is_a< pl::wstring >( ) )
				stream << name << "=" << pl::to_string( p.value< pl::wstring >( ) ) << endl;
			else if ( p.is_a< vector< double > >( ) )
				to_stream( stream, name, p.value< vector< double > >( ) );
			else if ( p.is_a< vector< int > >( ) )
				to_stream( stream, name, p.value< vector< int > >( ) );
			else
				stream << name << endl;
		}
	}
	else
	{
		stream << "None." << endl;
	}

	stream << endl;
}

void frame_report_basic( const ml::frame_type_ptr &frame )
{
	report_frame( cout, frame );
}

void frame_report_image( const ml::frame_type_ptr &frame )
{
	report_image( cout, frame->get_image( ), frame->get_sar_num( ), frame->get_sar_den( ) );
}

void frame_report_alpha( const ml::frame_type_ptr &frame )
{
	report_alpha( cout, frame->get_alpha( ) );
}

void frame_report_audio( const ml::frame_type_ptr &frame )
{
	report_audio( cout, frame->get_audio( ) );
}

void frame_report_props( const ml::frame_type_ptr &frame )
{
	report_props( cout, frame->properties( ) );
}

std::string to_multibyte_string( const olib::t_string& str )
{
#ifdef OLIB_ON_WINDOWS
    #ifdef OLIB_USE_UTF16
        // Make sure to convert to multibyte here
        // Converting to utf-8 will cause the ansi versions of the win32 API
        // used by boost::interprocess to fail.
        size_t required_length = wcstombs(0, str.c_str(), str.size() );
        std::vector< char > path_as_multibyte( required_length + 1, 0 );
        wcstombs(&path_as_multibyte[0], str.c_str(), str.size() );
        return std::string(&path_as_multibyte[0]);
    #else
        return str;
    #endif
#else
    return olib::opencorelib::str_util::to_string( str);
#endif
}


} }
