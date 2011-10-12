
// ml::audio - reseating capabailities for frame rate conversion

// Copyright (C) 2009 Ardendo
// Released under the LGPL.

#ifndef AML_AUDIO_RESEAT_H_
#define AML_AUDIO_RESEAT_H_

#include <openmedialib/ml/types.hpp>
#include <opencorelib/cl/core.hpp>
#include <opencorelib/cl/enforce_defines.hpp>
#include <string.h>

namespace olib { namespace openmedialib { namespace ml { namespace audio {

class reseat_impl : public reseat
{
	public:
		reseat_impl( ) 
			: queue( )
			, offset( 0 )
			, samples( 0 )
		{ 
		}

		virtual ~reseat_impl( ) 
		{ 
		}

		virtual bool append( audio_type_ptr audio, boost::uint32_t sample_offset = 0 )
		{
			// TODO: ensure that all audio packets are consistent and reject if not
			if ( audio )
			{
				ARENFORCE_MSG( !( sample_offset > 0 && queue.size() > 0 ), "Cannot push partial audio object into an audio_reseat if the queue is non-empty" )( sample_offset );

				queue.push_back( audio );
				if( sample_offset > 0 )
				{
					offset = sample_offset;
				}
				samples += ( audio->samples( ) - sample_offset ) ;
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

				audio_type_ptr temp = allocate( audio, -1, -1, requested );
				void *dst = temp->pointer( );

				result = temp;

				while( index != requested && queue.size( ) > 0 )
				{
					audio = audio::coerce( temp->af( ), queue[ 0 ] );

					int remainder = audio->samples( ) - offset;
					int samples_from_packet = requested - index < remainder ? requested - index : remainder;

					void *src = ( void * )( ( boost::uint8_t * )audio->pointer( ) + offset * channels * audio->sample_size( ) );
					memcpy( dst, src, samples_from_packet * channels * audio->sample_size( ) );

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
					dst = ( void * )( ( boost::uint8_t * )( dst ) + samples_from_packet * channels * audio->sample_size( ) );
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

		virtual int size( )
		{
			return samples;
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

} } } }

#endif

