
// ml::audio - channel conversion functionality for audio types

// Copyright (C) 2009 Ardendo
// Released under the LGPL.

#ifndef AML_AUDIO_CHANNEL_CONVERT_H_
#define AML_AUDIO_CHANNEL_CONVERT_H_

#include <openmedialib/ml/audio.hpp>
#include <cmath>

// Windows work around
#ifndef M_PI
#	define M_PI 3.14159265358979323846
#endif

namespace olib { namespace openmedialib { namespace ml { namespace audio {

namespace {

	inline bool is_channel_conversion_supported(int channels_in, int channels_out)
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
			return true;
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
	
	template< typename S > inline void mix_sample( const S *input, int channels_in, int channels_out, std::vector< float > &sum)
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
					const unsigned char CHANNEL_IDX_RIGHT				= 1;
					const unsigned char CHANNEL_IDX_CENTRE				= 2;
					const unsigned char CHANNEL_IDX_LOW_FREQ_EFFECTS	= 3;
					const unsigned char CHANNEL_IDX_LEFT_SURROUND		= 4;
					const unsigned char CHANNEL_IDX_RIGHT_SURROUND		= 5;

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
					const unsigned char CHANNEL_IDX_RIGHT			= 1;
					const unsigned char CHANNEL_IDX_LEFT_SURROUND	= 2;
					const unsigned char CHANNEL_IDX_RIGHT_SURROUND	= 3;
					const unsigned char CHANNEL_IDX_CENTRE	        = 4;

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
					const unsigned char CHANNEL_IDX_RIGHT				= 1;
					const unsigned char CHANNEL_IDX_CENTRE				= 2;
					const unsigned char CHANNEL_IDX_LOW_FREQ_EFFECTS	= 3;
					const unsigned char CHANNEL_IDX_LEFT_SURROUND		= 4;
					const unsigned char CHANNEL_IDX_RIGHT_SURROUND		= 5;

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

			default:
				{
					sum[ 0 ] = sum[ 1 ] = 0.0f;
					for ( int i = 0; i < channels_in; i ++ )
						sum[ i % 2 ] += float( input[ i ] );
				}
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
					const unsigned char CHANNEL_IDX_RIGHT				= 1;
					const unsigned char CHANNEL_IDX_CENTRE				= 2;
					const unsigned char CHANNEL_IDX_LOW_FREQ_EFFECTS	= 3;
					const unsigned char CHANNEL_IDX_LEFT_SURROUND		= 4;
					const unsigned char CHANNEL_IDX_RIGHT_SURROUND		= 5;

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

template < typename T >
ML_DECLSPEC boost::shared_ptr< T > channel_convert( const audio_type_ptr &in, int channels, const audio_type_ptr &last = audio_type_ptr( ) )
{
	const boost::shared_ptr< T > input_audio = coerce< T >( in );
	const boost::shared_ptr< T > last_audio = coerce< T >( last );

	// Reject if given dodgy input
	if( input_audio == boost::shared_ptr< T >( ) )
		return boost::shared_ptr< T >( );

	// Return input if already in desired form
	if( input_audio->channels() == channels )
		return input_audio;

	// Reject if conversion isn't supported
	if(!is_channel_conversion_supported(input_audio->channels(), channels))
		return boost::shared_ptr< T >( );

	// Create audio object for output
	const int samples = input_audio->samples( );
	T *base = new T( input_audio->frequency( ), channels, samples );

	boost::shared_ptr< T > output_audio = boost::shared_ptr< T >( base );
	typename T::sample_type min_sample = output_audio->min_sample( );
	typename T::sample_type max_sample = output_audio->max_sample( );
	
	// upfront filter calcs
	const float	cutoff_to_fs_ratio	= 0.5f;
	const float	exponent			= exp(-2.0f * M_PI * cutoff_to_fs_ratio);
	const float	one_minus_exponent	= 1.0f - exponent;
	
	typename T::sample_type *output = output_audio->data( );
	typename T::sample_type *input = input_audio->data( );
	typename T::sample_type *previous = last_audio ? last_audio->data( ) : 0;
	
	int channels_in = input_audio->channels( );

	std::vector< float > sum( channels );
	std::vector< float > clipped( channels );

	for( int sample_idx = 0; sample_idx < samples; ++sample_idx)
	{
		// Add appropriate channels together with appropriate weightings requested channels
		mix_sample< typename T::sample_type >( input, channels_in, channels, sum );
		input += channels_in;

		for(int ch = 0; ch < channels; ++ch)
		{
			// clip channels appropriately
			if(sum[ch] < min_sample )
				clipped[ch] = min_sample; 
			else if(sum[ch] > max_sample )
				clipped[ch] = max_sample ;
			else
				clipped[ch] = sum[ch];

			// low pass filter to counter any effects from clipping
			if(sample_idx == 0)
				*output = !previous ? ( typename T::sample_type )( one_minus_exponent * clipped[ch] ) :
									  ( typename T::sample_type )( one_minus_exponent * clipped[ch] + exponent * ( *( previous + last->samples( ) * last->channels( ) - channels + ch ) ) );
					
			else
				*output = ( typename T::sample_type )( one_minus_exponent * clipped[ch] + exponent * ( *( output - channels ) ) );

			output ++;
		}
	}
	
	return output_audio;
}

} } } }

#endif

