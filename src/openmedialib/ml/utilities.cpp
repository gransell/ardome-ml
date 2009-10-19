
// ml - A media library representation.

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#ifdef WIN32
#define _USE_MATH_DEFINES
#endif

#include <iostream>

#include <limits>
#include <cmath>
#include <openmedialib/ml/types.hpp>
#include <openmedialib/ml/audio.hpp>
#include <openmedialib/ml/ml.hpp>
#include <openmedialib/ml/openmedialib_plugin.hpp>
#include <openpluginlib/pl/openpluginlib.hpp>
#include <openpluginlib/pl/utf8_utils.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <deque>

namespace pl = olib::openpluginlib;
namespace ml = olib::openmedialib::ml;
namespace il = olib::openimagelib::il;

namespace olib { namespace openmedialib { namespace ml {

// Locate the largest common denominator of a and b
ML_DECLSPEC boost::int64_t gcd( boost::int64_t a, boost::int64_t b )
{
	if(a < 0)
		a = int( abs( static_cast<long>(a) ) );
	if(b < 0)
		b = int( abs( static_cast<long>(b) ) );
	if ( b > a )
	{
		boost::int64_t t = a;
		a = b;
		b = t;
	}
	while ( a % b > 0 )
	{
		boost::int64_t t = a % b;
		a = b;
		b = t;
	}
	return b;
}

ML_DECLSPEC boost::int64_t remove_gcd( boost::int64_t &a, boost::int64_t &b )
{
	boost::int64_t f = gcd( a, b );
	a /= f;
	b /= f;
	return f;
}

namespace
{
	inline float audio_convert_db( float db ) { return static_cast<float>(std::pow( 10.0, db / 20.0 )); }

	// Define min and max values for shorts
	// Note: Syntax used to avoid clash with min & max macro definitions
	//       see http://www.boost.org/more/lib_guide.htm
	const short min_short = (std::numeric_limits<short>::min)();
	const short max_short = (std::numeric_limits<short>::max)();

	inline boost::int64_t abs( boost::int64_t value )
	{ return value < 0 ? - value : value; }
	
	class audio_reseat_impl : public audio_reseat
	{
		public:
			audio_reseat_impl( ) 
				: queue( )
				, offset( 0 )
				, samples( 0 )
			{ 
			}

			virtual ~audio_reseat_impl( ) 
			{ 
			}

			virtual bool append( audio_type_ptr audio )
			{
				// TODO: ensure that all audio packets are consistent and reject if not
				if ( audio )
				{
					queue.push_back( audio );
					samples += audio->samples( );
				}

				return true;
			}

			virtual audio_type_ptr retrieve( int requested, bool pad )
			{
				audio_type_ptr result;
				if ( has( requested ) )
				{
					int index = 0;
					audio_type_ptr audio = queue[ 0 ];
					int channels = audio->channels( );

					audio::pcm16_ptr temp = audio::pcm16_ptr( new audio::pcm16( audio->frequency( ), channels, requested ) );
					boost::int16_t *dst = temp->data( );

					result = temp;

					while( index != requested && queue.size( ) > 0 )
					{
						audio = queue[ 0 ];
						short *src = ( short * )audio->pointer( ) + offset * channels;

						int remainder = audio->samples( ) - offset;
						int samples_from_packet = requested - index < remainder ? requested - index : remainder;

						for ( int i = 0; i < samples_from_packet * channels; i ++ )
							*dst ++ = *src ++;

						if ( samples_from_packet == remainder )
						{
							samples -= samples_from_packet;
							queue.pop_front( );
							offset = 0;
						}
						else
						{
							offset += samples_from_packet;
							samples -= samples_from_packet;
						}

						index += samples_from_packet;
					}
				}
				return result;
			}

			virtual void clear( )
			{
				queue.clear( );
				offset = 0;
				samples = 0;
			}

			bool has( int requested )
			{
				return requested <= samples;
			}

			virtual iterator begin( )
			{
				return queue.begin( );
			}

			virtual const_iterator begin( ) const
			{
				return queue.begin( );
			}

			virtual iterator end( )
			{
				return queue.end( );
			}

			virtual const_iterator end( ) const
			{
				return queue.end( );
			}

		private:
			bucket queue;
			int offset;
			int samples;
	};
}

// Query structure used
struct ml_query_traits : public pl::default_query_traits
{
	ml_query_traits( const pl::wstring& filename, const pl::wstring &type )
		: filename_( filename )
		, type_( type )
	{ }
		
	pl::wstring to_match( ) const
	{ return filename_; }

	pl::wstring libname( ) const
	{ return pl::wstring( L"openmedialib" ); }

	pl::wstring type( ) const
	{ return pl::wstring( type_ ); }
	

	const pl::wstring filename_;
	const pl::wstring type_;
};

static boost::recursive_mutex mutex_;

typedef boost::recursive_mutex::scoped_lock scoped_lock;

// Retrieve the first plugin which matches the resource and type (input or output)
static openmedialib_plugin_ptr get_plug( const pl::wstring &resource, const pl::wstring type )
{
	typedef pl::discovery< ml_query_traits > discovery;
	openmedialib_plugin_ptr result = openmedialib_plugin_ptr( );
	ml_query_traits query( resource, type );
	discovery plugins( query );
	if ( plugins.size( ) == 0 ) return result;
	discovery::const_iterator i = plugins.begin( );
	return boost::shared_dynamic_cast<openmedialib_plugin>( i->create_plugin( "" ) );
}

// Check if a plugin is avaialble
ML_DECLSPEC bool has_plugin_for( const pl::wstring &resource, const pl::wstring &type )
{
	scoped_lock lock( mutex_ );
	openmedialib_plugin_ptr plug = get_plug( resource, type );
	return plug != 0;
}

// Return the first matching input object
ML_DECLSPEC input_type_ptr create_delayed_input( const pl::wstring &resource )
{
	scoped_lock lock( mutex_ );
	PL_LOG( pl::level::debug5, boost::format( "Looking for a plugin for: %1%" ) % opl::to_string( resource ) );
	openmedialib_plugin_ptr plug = get_plug( resource, L"input" );
	if ( plug == 0 )
		PL_LOG( pl::level::error, boost::format( "Failed to find a plugin for: %1%" ) % opl::to_string( resource ) );
	return plug == 0 ? input_type_ptr( ) : plug->input( resource );
}

// Return the first matching input object
ML_DECLSPEC input_type_ptr create_input( const pl::wstring &resource )
{
	scoped_lock lock( mutex_ );
	input_type_ptr result = create_delayed_input( resource );
	if ( result )
		result->init( );
	return result;
}

// Return the first matching input object
ML_DECLSPEC input_type_ptr create_input( const pl::string &resource )
{
	scoped_lock lock( mutex_ );
	return create_input( pl::to_wstring( resource ) );
}

// Return the first matching store object
ML_DECLSPEC store_type_ptr create_store( const pl::wstring &resource, frame_type_ptr frame )
{
	scoped_lock lock( mutex_ );
	store_type_ptr result = store_type_ptr( );
	openmedialib_plugin_ptr plug = get_plug( resource, L"output" );
	if ( plug == 0 )
		PL_LOG( pl::level::error, boost::format( "Failed to find a plugin for: %1%" ) % opl::to_string( resource ) );
	return plug == 0 ? result : plug->store( resource, frame );
}

// Return the first matching store object
ML_DECLSPEC store_type_ptr create_store( const pl::string &resource, frame_type_ptr frame )
{
	scoped_lock lock( mutex_ );
	return create_store( pl::to_wstring( resource ), frame );
}

ML_DECLSPEC filter_type_ptr create_filter( const pl::wstring &resource )
{
	scoped_lock lock( mutex_ );
	filter_type_ptr result = filter_type_ptr( );
	openmedialib_plugin_ptr plug = get_plug( resource, L"filter" );
	PL_LOG( pl::level::debug5, boost::format( "Looking for a plugin for filter: %1%" ) % opl::to_string( resource ) );
	if ( plug == 0 )
		PL_LOG( pl::level::error, boost::format( "Failed to find a plugin for: %1%" ) % opl::to_string( resource ) );
	result = plug == 0 ? result : plug->filter( resource );
	if ( result )
		result->init( );
	return result;
}

ML_DECLSPEC audio_type_ptr audio_resample(const audio_type_ptr& input_audio, int sampling_freq)
{
	if(input_audio == audio_type_ptr())
		return audio_type_ptr();
	
	if(sampling_freq <= 0)
		return audio_type_ptr();
	
	if(input_audio->frequency() == sampling_freq)
		return input_audio;
	
	int		samples_in			= input_audio->samples(),
			channels_in			= input_audio->channels();
	
	double	num_samples			= (double(samples_in) * sampling_freq) / input_audio->frequency();
	int		num_samples_rounded	= int(0.5 + num_samples);
	
	audio::pcm16_ptr output_audio = audio::pcm16_ptr(new audio::pcm16(sampling_freq, channels_in, num_samples_rounded));
	
	double ratio = double(samples_in) / num_samples;
	
	// This means of interpolation assumes that the first sample of input and output are in sync
	// The reality is that this is unlikely.
	
	short	*in_ptr		= ((short*)input_audio->pointer()),
			*out_ptr	= ((short*)output_audio->pointer());
			
	double	offset			= 0.0,
			delta_offset	= 0.0;
	int 	offset_rounded	= 0;
	short	sample_a		= 0,
			sample_b		= 0;
	
//#define USE_LP_FILTER
#ifdef USE_LP_FILTER	
	// upfront filter calcs
	const double	cutoff_to_fs_ratio	= 0.5;
	const double	exponent			= exp(-2.0 * M_PI * cutoff_to_fs_ratio);
	const double	one_minus_exponent	= 1 - exponent;
#endif
	
	for(int sample_idx = 0; sample_idx < num_samples_rounded; ++sample_idx)
	{
		for(int channel_idx = 0; channel_idx < channels_in; ++channel_idx)
		{
			if(sample_idx == 0)
#ifndef USE_LP_FILTER
				*(out_ptr + channel_idx) = *(in_ptr + channel_idx);
#else
				*(out_ptr + channel_idx) = short(one_minus_exponent * *(in_ptr + channel_idx));
#endif
			else
			{
				offset			= ratio * sample_idx;	// Number of input samples from 1st sample
				offset_rounded	= int(offset);			// Number of input samples from 1st sample rounded down
				delta_offset	= fmod(offset, 1);
				
				if(offset + 1.0 > double(samples_in))
					*(out_ptr + (sample_idx * channels_in) + channel_idx) = *(in_ptr + ((samples_in - 1) * channels_in) + channel_idx);
				else
				{
					sample_a = *(in_ptr + (offset_rounded * channels_in) + channel_idx);
					sample_b = *(in_ptr + ((offset_rounded + 1) * channels_in) + channel_idx);
					
#ifndef USE_LP_FILTER					
					*(out_ptr + (sample_idx * channels_in) + channel_idx) = short(sample_a + (delta_offset * (sample_b - sample_a)) + 0.5);
#else
					*(out_ptr + (sample_idx * channels_in) + channel_idx) = short(one_minus_exponent * (sample_a + (delta_offset * (sample_b - sample_a))) + exponent * *(out_ptr + ((sample_idx - 1) * channels_in) + channel_idx) + 0.5);
#endif					
				}
			}
		}
	}
	
	return output_audio;
}

ML_DECLSPEC audio_reseat_ptr create_audio_reseat( )
{
	return audio_reseat_ptr( new audio_reseat_impl( ) );
}

ML_DECLSPEC audio_type_ptr audio_mix(const audio_type_ptr& a, const audio_type_ptr& b)
{
	// Reject if audio input frequencies differ
	if(a->frequency() != b->frequency())
		return audio_type_ptr();
	
	// Reject if number of audio samples differ
	if(a->samples() != b->samples())
		return audio_type_ptr();
	
	// Reject if number of audio channels differ
	if(a->channels() != b->channels())
		return audio_type_ptr();
	
	// Reject if audio format isn't 16-bit PCM
	if(		(a->af().compare(audio::FORMAT_PCM16) != 0)
		||	(b->af().compare(audio::FORMAT_PCM16) != 0) )
		return audio_type_ptr();
	
	const int samples_out  = a->samples();   // a & b are the same.
	const int channels_out = a->channels();
	
	audio::pcm16_ptr mix = audio::pcm16_ptr(new audio::pcm16(a->frequency(), channels_out, samples_out));

	// upfront filter calcs
	const double	cutoff_to_fs_ratio	= 0.5;
	const double	exponent			= exp(-2.0 * M_PI * cutoff_to_fs_ratio);
	const double	one_minus_exponent	= 1 - exponent;
	
	long  sample_summation = 0;
	short clipped_sample   = 0;

	short *po = (short*)mix->pointer();
	short *pa = (short*)a->pointer();
	short *pb = (short*)b->pointer();

	for( int sample_idx = 0; sample_idx < samples_out; sample_idx++)
	{
		for( int channel_idx = 0; channel_idx < channels_out; ++channel_idx)
		{
			// Add a & b together
			sample_summation = long( *pa ++ ) + long( *pb ++ );
			
			// clip appropriately
			if(sample_summation < min_short)
				clipped_sample = min_short; 
			else if(sample_summation > max_short)
				clipped_sample = max_short;
			else
				clipped_sample = short(sample_summation);
			
			// low pass filter to counter any effects from clipping
			if(sample_idx == 0)
				*po = short(one_minus_exponent * clipped_sample);
			else
				*po = short(one_minus_exponent * clipped_sample + exponent * ( *( po - channels_out ) ) );

			po ++;
		}
	}
	
	return mix;
}

ML_DECLSPEC audio_type_ptr audio_reverse( audio_type_ptr audio )
{
	if ( audio )
	{
		const int channels = audio->channels( );
		const int samples = audio->samples( );

		short *in = ( short * )audio->pointer( );
		short *out = in + channels * samples - channels;
		int c;

		while ( in < out )
		{
			c = channels;
			while( c -- )
			{
				short t = *in;
				*in ++ = *out;
				*out ++ = t;
			}
			out -= 2 * channels;
		}
	}

	return audio;
}

ML_DECLSPEC frame_type_ptr frame_convert( frame_type_ptr frame, const openpluginlib::wstring &pf )
{
	frame_type_ptr result = frame;

	if ( result && result->get_image( ) )
	{
		il::image_type_ptr src = result->get_image( );

		if ( src && src->pf( ) != pf )
		{
			il::image_type_ptr alpha = il::extract_alpha( src );
			if ( alpha )
				result->set_alpha( alpha );
			result->set_image( il::convert( src, pf ) );
		}
	}

	return result;
}

ML_DECLSPEC frame_type_ptr frame_rescale( frame_type_ptr frame, int new_w, int new_h, il::rescale_filter filter )
{
	frame_type_ptr result = frame;

	if ( result )
	{
		if ( result->get_alpha( ) )
			result->set_alpha( il::rescale( result->get_alpha( ), new_w, new_h, 1, filter ) );
		if ( result->get_image( ) )
			result->set_image( il::rescale( result->get_image( ), new_w, new_h, 1, filter ) );
	}

	return result;
}

ML_DECLSPEC frame_type_ptr frame_crop_clear( frame_type_ptr frame )
{
	frame_type_ptr result = frame;

	if ( result )
	{
		if ( result->get_alpha( ) )
			result->get_alpha( )->crop_clear( );
		if ( result->get_image( ) )
			result->get_image( )->crop_clear( );
	}

	return result;
}

ML_DECLSPEC frame_type_ptr frame_crop( frame_type_ptr frame, int x, int y, int w, int h )
{
	frame_type_ptr result = frame;

	if ( result )
	{
		if ( result->get_alpha( ) )
			result->get_alpha( )->crop( x, y, w, h );
		if ( result->get_image( ) )
			result->get_image( )->crop( x, y, w, h );
	}

	return result;
}

ML_DECLSPEC frame_type_ptr frame_volume( frame_type_ptr frame, float volume )
{
	if ( frame && frame->get_audio( ) )
	{
		audio_type_ptr audio = frame->get_audio( );

		const int channels = audio->channels( );
		const int samples = audio->samples( );

		short *data = ( short * )audio->pointer( );
		size_t count = ( channels * samples );

		while( count -- )
		{
			*data = short( volume * *data );
			data ++;
		}
	}
	return frame;
}

static stream_handler_ptr ( *stream_handler_callback )( const openpluginlib::wstring, int ) = 0;

ML_DECLSPEC stream_handler_ptr stream_handler_fetch( const openpluginlib::wstring url, int flags )
{
	if ( stream_handler_callback )
		return stream_handler_callback( url, flags );
	return stream_handler_ptr( );
}

ML_DECLSPEC void stream_handler_register( stream_handler_ptr ( *cb )( const openpluginlib::wstring, int ) )
{
	stream_handler_callback = cb;
}

} } }


