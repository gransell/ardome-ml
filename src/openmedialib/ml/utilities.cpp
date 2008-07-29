
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

namespace
{
	inline float audio_convert_db( float db ) { return static_cast<float>(std::pow( 10.0, db / 20.0 )); }

	// Define min and max values for shorts
	// Note: Syntax used to avoid clash with min & max macro definitions
	//       see http://www.boost.org/more/lib_guide.htm
	const short min_short = (std::numeric_limits<short>::min)();
	const short max_short = (std::numeric_limits<short>::max)();
	
	// Locate the largest common denominator of a and b
	int gcd( int a, int b )
	{
		if(a < 0)
			a = int( std::abs( a ) );
		if(b < 0)
			b = int( std::abs( b ) );
		if ( b > a )
		{
			int t = a;
			a = b;
			b = t;
		}
		while ( a % b > 0 )
		{
			int t = a % b;
			a = b;
			b = t;
		}
		return b;
	}
	
	int calculate_cycle_size(double frames_per_second, int samplefreq, double samples_per_frame, int &deficit)
	{
		int 	cycle_size	 	= int(floor(frames_per_second + 0.5));
		double 	extra_samples	= (int(floor(samples_per_frame + 0.5)) * int(floor(frames_per_second + 0.5))) - (samplefreq * int(floor(frames_per_second + 0.5)) / frames_per_second);
		
		while(int(extra_samples*1000) % 1000 != 0 && cycle_size <= 3000)
		{
			cycle_size *= 10;
			extra_samples *= 10;
		}
		
		if(int(floor(extra_samples + 0.5)) == 0)
			cycle_size = 1;
		else
			cycle_size /= gcd(cycle_size, int(floor(extra_samples + 0.5)));
	
		deficit = int(extra_samples) < 0 ? -1 : 1;

		return cycle_size;
	}

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
					typedef audio< unsigned char, pcm16 > pcm16;

					audio_type_ptr audio = queue[ 0 ];
					int channels = audio->channels( );

					result = audio_type_ptr( new audio_type( pcm16( audio->frequency( ), channels, requested ) ) );
					short *dst = ( short * )result->data( );

					while( index != requested && queue.size( ) > 0 )
					{
						audio = queue[ 0 ];
						short *src = ( short * )audio->data( ) + offset * channels;

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

	inline bool is_channel_convertion_supported(int channels_in, int channels_out)
	{
		switch(channels_out)
		{
		case 1:
			switch(channels_in)
			{
			case 2:
			case 3:
			case 4:
			case 6:
				return true;
			}
			break;
		case 2:
			switch(channels_in)
			{
			case 1:
			case 3:
			case 4:
			case 5:
			case 6:
				return true;
			}
			break;
		case 4:
			switch(channels_in)
			{
			//case 1:
			case 2:
			//case 3:
			case 6:
				return true;
			}
			break;
		}
		
		return false;
	}
	
	inline void mix_sample( const short *input, int channels_in, int channels_out, std::vector< float > &sum)
	{
		switch(channels_out)
		{
		case 1:
			switch( channels_in )
			{
			case 2:
				{
					const unsigned char CHANNEL_IDX_LEFT 		= 0;
					const unsigned char CHANNEL_IDX_RIGHT		= 1;
					
					sum[0] = ( float( input[ CHANNEL_IDX_LEFT ] ) + float( input[ CHANNEL_IDX_RIGHT ] ) );
				}
				break;
			case 3:
				{
					const unsigned char CHANNEL_IDX_LEFT 	= 0;
					const unsigned char CHANNEL_IDX_CENTRE	= 1;
					const unsigned char CHANNEL_IDX_RIGHT	= 2;
					
					sum[0] = 		( 	float(input[ CHANNEL_IDX_LEFT ]) 
								+ 		float(input[ CHANNEL_IDX_RIGHT ]) )
								+ 	  	float(input[ CHANNEL_IDX_CENTRE ]);
				}
				break;
			case 4:
				{
					const unsigned char CHANNEL_IDX_LEFT 			= 0;
					const unsigned char CHANNEL_IDX_RIGHT			= 1;
					const unsigned char CHANNEL_IDX_LEFT_SURROUND	= 2;
					const unsigned char CHANNEL_IDX_RIGHT_SURROUND	= 3;

					sum[0] =		(	float(input[ CHANNEL_IDX_LEFT ])
								+		float(input[ CHANNEL_IDX_RIGHT ]) )
								+	(	float(input[ CHANNEL_IDX_LEFT_SURROUND ])
								+		float(input[ CHANNEL_IDX_RIGHT_SURROUND ]) );
				}
				break;
			case 6:
				{
					const unsigned char CHANNEL_IDX_LEFT 				= 0;
					const unsigned char CHANNEL_IDX_CENTRE				= 1;
					const unsigned char CHANNEL_IDX_RIGHT				= 2;
					const unsigned char CHANNEL_IDX_LEFT_SURROUND		= 3;
					const unsigned char CHANNEL_IDX_RIGHT_SURROUND		= 4;
					const unsigned char CHANNEL_IDX_LOW_FREQ_EFFECTS	= 5;

					sum[0] =		(	float(input[ CHANNEL_IDX_LEFT ])
								+		float(input[ CHANNEL_IDX_RIGHT ]) )
								+	(	float(input[ CHANNEL_IDX_LEFT_SURROUND ])
								+		float(input[ CHANNEL_IDX_RIGHT_SURROUND ]) )
								+		float(input[ CHANNEL_IDX_CENTRE ])
								+		float(input[ CHANNEL_IDX_LOW_FREQ_EFFECTS ]);
				}
				break;
			}		
			break;
		case 2:
			switch( channels_in )
			{
			case 1:
				sum[0] = float( input[ 0 ] );
				sum[1] = sum[0];
				break;
			case 3:
				{
					const unsigned char CHANNEL_IDX_LEFT 	= 0;
					const unsigned char CHANNEL_IDX_CENTRE	= 1;
					const unsigned char CHANNEL_IDX_RIGHT	= 2;

					sum[0] =		float(input[ CHANNEL_IDX_LEFT ])
								+	float(input[ CHANNEL_IDX_CENTRE ]);
			
					sum[1] =		float(input[ CHANNEL_IDX_RIGHT ])
								+	float(input[ CHANNEL_IDX_CENTRE ]);
				}
				break;
			case 4:
				{
					const unsigned char CHANNEL_IDX_LEFT 			= 0;
					const unsigned char CHANNEL_IDX_RIGHT			= 1;
					const unsigned char CHANNEL_IDX_LEFT_SURROUND	= 2;
					const unsigned char CHANNEL_IDX_RIGHT_SURROUND	= 3;

					sum[0] =		float(input[ CHANNEL_IDX_LEFT ])
								+ 	float(input[ CHANNEL_IDX_LEFT_SURROUND ]);
			
					sum[1] =		float(input[ CHANNEL_IDX_RIGHT ])
								+ 	float(input[ CHANNEL_IDX_RIGHT_SURROUND ]);
				}
				break;
			case 5:
				{
					const unsigned char CHANNEL_IDX_LEFT 			= 0;
					const unsigned char CHANNEL_IDX_CENTRE	        = 1;
					const unsigned char CHANNEL_IDX_RIGHT			= 2;
					const unsigned char CHANNEL_IDX_LEFT_SURROUND	= 3;
					const unsigned char CHANNEL_IDX_RIGHT_SURROUND	= 4;

					sum[0] =		float(input[ CHANNEL_IDX_LEFT ])
								+ 	float(input[ CHANNEL_IDX_LEFT_SURROUND ])
								+	float(input[ CHANNEL_IDX_CENTRE ]);

					sum[1] =		float(input[ CHANNEL_IDX_RIGHT ])
								+ 	float(input[ CHANNEL_IDX_RIGHT_SURROUND ])
								+	float(input[ CHANNEL_IDX_CENTRE ]);
				}
				break;
			case 6:
				{
					const unsigned char CHANNEL_IDX_LEFT 				= 0;
					const unsigned char CHANNEL_IDX_CENTRE				= 1;
					const unsigned char CHANNEL_IDX_RIGHT				= 2;
					const unsigned char CHANNEL_IDX_LEFT_SURROUND		= 3;
					const unsigned char CHANNEL_IDX_RIGHT_SURROUND		= 4;
					const unsigned char CHANNEL_IDX_LOW_FREQ_EFFECTS	= 5;

					sum[0] =		float(input[ CHANNEL_IDX_LEFT ])
								+ 	float(input[ CHANNEL_IDX_LEFT_SURROUND ])
								+	float(input[ CHANNEL_IDX_CENTRE ])
								+	float(input[ CHANNEL_IDX_LOW_FREQ_EFFECTS ]);

					sum[1] =		float(input[ CHANNEL_IDX_RIGHT ])
								+ 	float(input[ CHANNEL_IDX_RIGHT_SURROUND ])
								+	float(input[ CHANNEL_IDX_CENTRE ])
								+	float(input[ CHANNEL_IDX_LOW_FREQ_EFFECTS ]);
				}
				break;
			}
			break;
		case 4:
			switch( channels_in )
			{
			case 1:
				sum[0] = float( input[ 0 ] );
				sum[1] = sum[0];
				sum[2] = 0;
				sum[3] = 0;
				break;
			case 2:
				{
					const unsigned char CHANNEL_IDX_LEFT 	= 0;
					const unsigned char CHANNEL_IDX_RIGHT	= 1;
					
					sum[0] = float(input[ CHANNEL_IDX_LEFT ]);
					sum[1] = float(input[ CHANNEL_IDX_RIGHT ]);
					sum[2] = 0;
					sum[3] = 0;
				}
				break;
			case 3:
				{
					const unsigned char CHANNEL_IDX_LEFT 	= 0;
					const unsigned char CHANNEL_IDX_CENTRE	= 1;
					const unsigned char CHANNEL_IDX_RIGHT	= 2;

					sum[0] =		float(input[ CHANNEL_IDX_LEFT ])
								+	float(input[ CHANNEL_IDX_CENTRE ]);
			
					sum[1] =		float(input[ CHANNEL_IDX_RIGHT ])
								+	float(input[ CHANNEL_IDX_CENTRE ]);
					sum[2] = 0;
					sum[3] = 0;
				}
				break;
			case 6:
				{
					const unsigned char CHANNEL_IDX_LEFT 				= 0;
					const unsigned char CHANNEL_IDX_CENTRE				= 1;
					const unsigned char CHANNEL_IDX_RIGHT				= 2;
					const unsigned char CHANNEL_IDX_LEFT_SURROUND		= 3;
					const unsigned char CHANNEL_IDX_RIGHT_SURROUND		= 4;
					const unsigned char CHANNEL_IDX_LOW_FREQ_EFFECTS	= 5;

					sum[0] =		float(input[ CHANNEL_IDX_LEFT ])
								+ 	float(input[ CHANNEL_IDX_CENTRE ])
								+	float(input[ CHANNEL_IDX_LOW_FREQ_EFFECTS ]);
			
					sum[1] =		float(input[ CHANNEL_IDX_RIGHT ])
								+ 	float(input[ CHANNEL_IDX_CENTRE ])
								+	float(input[ CHANNEL_IDX_LOW_FREQ_EFFECTS ]);
								
					sum[2] =		float(input[ CHANNEL_IDX_LEFT_SURROUND ]);
					sum[3] =		float(input[ CHANNEL_IDX_RIGHT_SURROUND ]);
	
				}
				break;
			}
			break;
		}
	}
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

// Return the first matching input object
ML_DECLSPEC input_type_ptr create_delayed_input( const pl::wstring &resource )
{
	scoped_lock lock( mutex_ );
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
	
	audio_type_ptr output_audio = audio_type_ptr(new audio_type(audio<unsigned char, pcm16>(sampling_freq, channels_in, num_samples_rounded)));
	
	double ratio = double(samples_in) / num_samples;
	
	// This means of interpolation assumes that the first sample of input and output are in sync
	// The reality is that this is unlikely.
	
	short	*in_ptr		= ((short*)input_audio->data()),
			*out_ptr	= ((short*)output_audio->data());
			
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

ML_DECLSPEC int audio_samples_for_frame(int frameoffset, int samplefreq, int framerate_numerator, int framerate_denominator)
{
	// Work around: some video files report frame rates with a common denominator (ie: 25000:1000)
	// Without factoring that out upfront (in this implementation), the results are incorrect
	int common = gcd( framerate_numerator, framerate_denominator );
	framerate_numerator /= common;
	framerate_denominator /= common; 

	double	frames_per_second	= double(framerate_numerator) / framerate_denominator;
	double	samples_per_frame	= double(samplefreq) / frames_per_second;
	
	int	deficit;
	int frames_per_cycle = calculate_cycle_size(frames_per_second, samplefreq, samples_per_frame, deficit);

	int		samples_per_frame_base	= int((( long long )samplefreq * framerate_denominator) / framerate_numerator);
	double	extra_sample_spread		= samples_per_frame - double(samples_per_frame_base);
	int		cycle_idx				= frameoffset % frames_per_cycle;

	if(cycle_idx == 0)
		return samples_per_frame_base + (extra_sample_spread > 0.5 ? deficit : 0);
	else
		return samples_per_frame_base + (int(floor((extra_sample_spread * cycle_idx) + 0.5)) > int(floor(extra_sample_spread * (cycle_idx - 1) + 0.5)) ? deficit : 0);
}

ML_DECLSPEC long long audio_samples_to_frame(int frameoffset, int samplefreq, int framerate_numerator, int framerate_denominator)
{
	int common = gcd( framerate_numerator, framerate_denominator );
	framerate_numerator /= common;
	framerate_denominator /= common; 

	double	frames_per_second	= double(framerate_numerator) / framerate_denominator;
	double	samples_per_frame	= double(samplefreq) / frames_per_second;
	
	int deficit;
	int frames_per_cycle = calculate_cycle_size(frames_per_second, samplefreq, samples_per_frame, deficit);
	
	int		samples_per_frame_base	= (samplefreq * framerate_denominator) / framerate_numerator;
	int		samples_per_cycle		= int(float(samples_per_frame * double(frames_per_cycle)));
	double	extra_sample_spread		= samples_per_frame - double(samples_per_frame_base);
	int		cycle_idx				= frameoffset % frames_per_cycle;
	
	long long offset = ((long long)(frameoffset / frames_per_cycle) * samples_per_cycle);
	
	if(cycle_idx > 0)
	{
		offset += cycle_idx * samples_per_frame_base;
		
		if(extra_sample_spread > 0.5)
			offset += deficit;
		
		offset += deficit * int(floor((extra_sample_spread * cycle_idx) - extra_sample_spread + 0.5));
	}

	return offset;
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
	if(		(a->af().compare(AUDIO_FORMAT_PCM16) != 0)
		||	(b->af().compare(AUDIO_FORMAT_PCM16) != 0) )
		return audio_type_ptr();
	
	const audio_type::size_type samples_out  = a->samples();   // a & b are the same.
	const audio_type::size_type channels_out = a->channels();
	
	audio_type_ptr mix = audio_type_ptr(new audio_type(audio<unsigned char, pcm16>(a->frequency(), channels_out, samples_out)));

	// upfront filter calcs
	const double	cutoff_to_fs_ratio	= 0.5;
	const double	exponent			= exp(-2.0 * M_PI * cutoff_to_fs_ratio);
	const double	one_minus_exponent	= 1 - exponent;
	
	long  sample_summation = 0;
	short clipped_sample   = 0;

	short *po = (short*)mix->data();
	short *pa = (short*)a->data();
	short *pb = (short*)b->data();

	for(audio_type::size_type sample_idx = 0; sample_idx < samples_out; sample_idx++)
	{
		for(audio_type::size_type channel_idx = 0; channel_idx < channels_out; ++channel_idx)
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

ML_DECLSPEC audio_type_ptr audio_channel_convert(const audio_type_ptr& input_audio, int channels)
{
	// Reject if given dodgy input
	if(input_audio == audio_type_ptr())
		return audio_type_ptr();

	// Return input if already in desired form
	if(input_audio->channels() == channels)
		return input_audio;

	// Reject if convertion isn't supported
	if(!is_channel_convertion_supported(input_audio->channels(), channels))
		return audio_type_ptr();

	// Reject if audio format isn't 16-bit PCM
	if(input_audio->af().compare(AUDIO_FORMAT_PCM16) != 0)
		return audio_type_ptr();

	// Create audio object for output
	const audio_type::size_type	samples = input_audio->samples();
	audio_type_ptr output_audio = audio_type_ptr(new audio_type(audio<unsigned char, pcm16>(input_audio->frequency(), channels, samples)));
	
	// upfront filter calcs
	const double	cutoff_to_fs_ratio	= 0.5;
	const double	exponent			= exp(-2.0 * M_PI * cutoff_to_fs_ratio);
	const double	one_minus_exponent	= 1 - exponent;
	
	short *output = (short*)output_audio->data();
	short *input = (short*)input_audio->data();
	int channels_in = input_audio->channels( );
	
	std::vector< float > sum( channels );
	std::vector< float > clipped( channels );

	for(audio_type::size_type sample_idx = 0; sample_idx < samples; ++sample_idx)
	{
		// Add appropriate channels together with appropriate weightings requested channels
		mix_sample( input, channels_in, channels, sum );
		input += channels_in;

		for(int ch = 0; ch < channels; ++ch)
		{
			// clip channels appropriately
			if(sum[ch] < min_short)
				clipped[ch] = min_short; 
			else if(sum[ch] > max_short)
				clipped[ch] = max_short;
			else
				clipped[ch] = sum[ch];

			// low pass filter to counter any effects from clipping
			if(sample_idx == 0)
				*output = short( one_minus_exponent * clipped[ch] );
			else
				*output = short( one_minus_exponent * clipped[ch] + exponent * ( *( output - channels ) ) );

			output ++;
		}
	}
	
	return output_audio;
}

ML_DECLSPEC audio_type_ptr audio_reverse( audio_type_ptr audio )
{
	if ( audio )
	{
		const audio_type::size_type	channels = audio->channels( );
		const audio_type::size_type	samples = audio->samples( );

		short *in = ( short * )audio->data( );
		short *out = in + channels * samples - channels;
		audio_type::size_type c;

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

		const audio_type::size_type	channels = audio->channels( );
		const audio_type::size_type	samples = audio->samples( );

		short *data = ( short * )audio->data( );
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


