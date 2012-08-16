// General purpose utility functionality
//
// Copyright (C) 2007 Ardendo
// Released under the terms of the LGPL.

#include "precompiled_headers.hpp"
#include "amf_filter_plugin.hpp"
#include "utility.hpp"
#include <openmedialib/ml/stream.hpp>
#include <openmedialib/ml/audio_block.hpp>
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

void apply_volume( ml::frame_type_ptr &result, float start, float end )
{
	if ( result && result->get_audio( ) )
	{
		result->set_audio( ml::audio::volume( result->get_audio( ), start, end ) );
	}
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
	ml::audio_type_ptr audio = result->get_audio( );
	audio = ml::audio::channel_mixer( audio, channel->get_audio( ), volume, max_level, mute );
	result->set_audio( audio );
	return true;
}

static pl::pcos::key key_peaks_( pcos::key::from_string( "peaks" ) );

void join_peaks( ml::frame_type_ptr &result, std::vector< double > &max_levels )
{
	if( ! result->get_audio( ) )
		return;

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
		for ( std::vector< double >::iterator iter = max_levels.begin( ); iter != max_levels.end( ); ++iter )
			current.push_back( *iter );
		frame_peaks = current;
	}
}

void join_peaks( ml::frame_type_ptr &result, ml::frame_type_ptr &input )
{
	if( ! result->get_audio( ) || ! input->get_audio( ) )
		return;

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
	
std::string print_track_packets( const ml::audio::block_type::channel_map& track_packets )
{
	std::stringstream res;
	res << "[";
	for( ml::audio::block_type::channel_map::const_iterator it = track_packets.begin(); it != track_packets.end(); ++ it )
	{
		
		res << it->first;
		if( it != --track_packets.end() )
			res << ", ";
	}
	res << "]";
	
	return res.str();
}

void report_frame( std::ostream &stream, const ml::frame_type_ptr &frame )
{
	int num, den;
	frame->get_fps( num, den );
	stream << "Frame Report" << endl << endl;
	stream << "Position     : " << frame->get_position( ) << endl;
	if ( frame->has_image( ) )
		stream << "Image Info   : pf = " << pl::to_string( frame->pf( ) ) << ", codec = " << frame->video_codec( ) << ", size = " << frame->width( ) << "x" << frame->height( ) << ", sar = " << frame->get_sar_num( ) << ":" << frame->get_sar_den( ) << std::endl;
	if ( frame->has_audio( ) )
		stream << "Audio Info   : samples = " << frame->samples( ) << ", frequency = "  << frame->frequency( ) << ", channels = " << frame->channels( ) << std::endl;
	if ( frame->get_stream( ) )
		stream << "Video Stream : Yes, codec = " << frame->get_stream( )->codec( ) << ", pf = " << pl::to_string( frame->get_stream( )->pf( ) ) << " key = " << frame->get_stream( )->key( ) << ", position = " << frame->get_stream( )->position( ) << ", bitrate = " << frame->get_stream( )->bitrate( )<< ", bytes = " << frame->get_stream( )->length( )
        << ", dimensions = " << frame->get_stream( )->size( ).width << "x" << frame->get_stream( )->size( ).height << " gop = " << frame->get_stream()->estimated_gop_size() << endl;
	else
		stream << "Video Stream : No" << endl;
	
	if( frame->audio_block( ) ) {
		ml::audio::block_type_ptr audio_block = frame->audio_block();
		stream << "Audio Stream : Yes, position = " <<  audio_block->position << ", samples = " << audio_block->samples << 
			", sample size = " << ( audio_block->tracks.begin()->second.begin()->second->sample_size( ) * 8 ) << ", first = " << audio_block->first << ", count = " << audio_block->count << ", discard = " << audio_block->discard << ", tracks = " << audio_block->tracks.size( ) << endl;
		ml::audio::block_type::track_map::iterator it = frame->audio_block()->tracks.begin();
		for( ; it != frame->audio_block()->tracks.end(); ++it ) {
			stream << "     Track " << it->first << " : packets = " << print_track_packets( it->second ) << ", codec = " << it->second.begin()->second->codec( ) << ", channels = " << it->second.begin()->second->channels() << endl;
		}
	}
	else {
		stream << "Audio Stream : No" << endl;
	}

	if ( frame->get_image( ) )
		stream << "Has Image    : Yes, position = " << frame->get_image( )->position( ) << endl;
	else
		stream << "Has Image    : No" << endl;

	if ( frame->get_audio( ) )
		stream << "Has Audio    : Yes, position = " << frame->get_audio( )->position( ) << endl;
	else
		stream << "Has Audio    : No" << endl;

	stream << "Has Alpha    : " << ( frame->get_alpha( ) ? "Yes" : "No" ) << endl;
	stream << "Frame Rate   : " << frame->fps( ) << " (" << num << ":" << den << ")" << endl;
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
		if ( audio->samples( ) != audio->original_samples( ) )
			stream << "Samples(src): " << audio->original_samples( ) << endl;
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
	for ( vector< double >::iterator iter = l.begin( ); iter != l.end( ); ++iter )
		stream << *iter << " ";
	stream << endl;
}

void to_stream( std::ostream &stream, const std::string &name, std::vector< int > l )
{
	stream << name << "=";
	for ( vector< int >::iterator iter = l.begin( ); iter != l.end( ); ++iter )
		stream << *iter << " ";
	stream << endl;
}

void report_props( std::ostream &stream, const pl::pcos::property_container &props )
{
	// Obtain the keys on the filter
	pcos::key_vector keys = props.get_keys( );

	if ( keys.size( ) )
	{
		// For each key...
		for( pcos::key_vector::iterator it = keys.begin( ); it != keys.end( ); ++it )
		{
			std::string name( ( *it ).as_string( ) );
			pcos::property p = props.get_property_with_key( *it );
			if ( p.is_a< double >( ) )
				stream << name << "=" << p.value< double >( ) << endl;
			else if ( p.is_a< int >( ) )
				stream << name << "=" << p.value< int >( ) << endl;
			else if ( p.is_a< boost::int64_t >( ) )
				stream << name << "=" << p.value< boost::int64_t >( ) << endl;
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
	cout << "Frame Properties Report:\n\n";
	report_props( cout, frame->properties( ) );

	if( frame->get_stream( ) )
	{
		cout << "Stream Properties Report:\n\n";
		report_props( cout, frame->get_stream( )->properties( ) );
	}
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
