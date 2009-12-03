// Locked Audio Filter
//
// Copyright (C) 2009 Ardendo
// Released under the terms of the LGPL.
//
// #filter:locked_audio
//
// Locks audio for DV and IMX sample deliver.

#include "precompiled_headers.hpp"
#include "amf_filter_plugin.hpp"
#include "utility.hpp"

#include <openpluginlib/pl/log.hpp>

#include <opencorelib/cl/core.hpp>
#include <opencorelib/cl/log_defines.hpp>

namespace aml { namespace openmedialib {

class ML_PLUGIN_DECLSPEC filter_locked_audio : public ml::filter_type
{
	public:
		// Filter_type overloads
		explicit filter_locked_audio( const pl::wstring & )
			: ml::filter_type( )
			, prop_enable_( pcos::key::from_string( "enable" ) )
			, prop_profile_( pcos::key::from_string( "profile" ) )
			, fps_num_( -1 )
			, fps_den_( -1 )
			, frequency_( -1 )
			, channels_( -1 )
		{
			properties( ).append( prop_enable_ = 1 );
			properties( ).append( prop_profile_ = pl::wstring( L"dv" ) );

			std::vector< int > dv_samples;
			dv_samples.push_back( 1600 );
			dv_samples.push_back( 1602 );
			dv_samples.push_back( 1602 );
			dv_samples.push_back( 1602 );
			dv_samples.push_back( 1602 );

			std::vector< int > imx_samples;
			imx_samples.push_back( 1602 );
			imx_samples.push_back( 1601 );
			imx_samples.push_back( 1602 );
			imx_samples.push_back( 1601 );
			imx_samples.push_back( 1602 );

			profiles_[ pl::wstring( L"dv" ) ] = dv_samples;
			profiles_[ pl::wstring( L"dv25" ) ] = dv_samples;
			profiles_[ pl::wstring( L"dv50" ) ] = dv_samples;
			profiles_[ pl::wstring( L"dv1000" ) ] = dv_samples;
			profiles_[ pl::wstring( L"dvcpro" ) ] = dv_samples;
			profiles_[ pl::wstring( L"dvcpro50" ) ] = dv_samples;
			profiles_[ pl::wstring( L"imx" ) ] = imx_samples;
			profiles_[ pl::wstring( L"imx50" ) ] = imx_samples;
		}

		// Indicates if the input will enforce a packet decode
		virtual bool requires_image( ) const { return false; }

		// This provides the name of the plugin (used in serialisation)
		virtual const pl::wstring get_uri( ) const { return L"locked_audio"; }

	protected:
		// The main access point to the filter
		void do_fetch( ml::frame_type_ptr &result )
		{
			// Locked audio generation
			if ( prop_enable_.value< int >( ) && profiles_.find( prop_profile_.value< pl::wstring >( ) ) != profiles_.end( ) && get_position( ) < get_frames( ) )
			{
				ml::input_type_ptr input = fetch_slot( 0 );
				const std::vector< int > &table = profiles_[ prop_profile_.value< pl::wstring >( ) ];
				int start = get_position( ) - get_position( ) % table.size( );
				int final = start + table.size( );

				// Populate the audio_span for the number of the frames in the table each time we move to another group of frames
				if ( frames_.size( ) == 0 || frames_[ 0 ]->get_position( ) != start )
				{
					int start_audio = -1;
					int final_audio = -1;
					int samples = 0;

					ml::frame_type_ptr frame = fetch_from_slot( );
                    
                    if( !frame || !(frame->get_audio( )) )
                    {
                        result = frame;
                        return;
                    }

					// Allocate a single audio object to accomodate the number of samples specified in the table
					if ( audio_span_ == 0 || frame->get_audio( )->channels( ) != channels_ )
					{
                        frame->get_fps( fps_num_, fps_den_ );
                        frequency_ = frame->get_audio( )->frequency( );
                        channels_ = frame->get_audio( )->channels( );
                        if ( fps_num_ == 30000 && fps_den_ == 1001 && frequency_ == 48000 )
                        {
                            int total = 0;
                            for ( size_t i = 0; i < table.size( ); i ++ ) total += table[ i ];
                            audio_span_ = ml::audio::allocate( frame->get_audio( ), 48000, channels_, total );
                        }
                        else
                        {
                            result = frame;
                            return;
                        }
					}

					// Erase the previously collected frames
					frames_.erase( frames_.begin( ), frames_.end( ) );

					// Collect and analyse all frames needed to match the table size
					for( int index = start; index < final && index < get_frames( ); index ++ )
					{
						ml::frame_type_ptr frame = input->fetch( index );
						if ( frame && frame->get_audio( ) )
						{
							if( start_audio == -1 ) start_audio = index - start;
							final_audio = index - start;
							samples += frame->get_audio( )->samples( );
						}
						frames_.push_back( frame );
					}

					// Pack all the audio into the audio span - if the number of samples match precisely, we just need to append them
					// but if there is a short fall, then we pitch shift to make sure we have what we want
					if ( samples == audio_span_->samples( ) )
					{
						boost::uint8_t *ptr = ( boost::uint8_t * )audio_span_->pointer( );
						for( std::vector< ml::frame_type_ptr >::iterator iter = frames_.begin( ); iter != frames_.end( ); iter ++ )
						{
							memcpy( ptr, ( *iter )->get_audio( )->pointer( ), ( *iter )->get_audio( )->size( ) );
							ptr += ( *iter )->get_audio( )->size( );
						}
					}
					else if ( start_audio != -1 )
					{
						boost::uint8_t *ptr = ( boost::uint8_t *)audio_span_->pointer( );
                        if ( frames_.size( ) == table.size( ) )
						{
                            PL_LOG( pl::level::warning, "We do not have enough samples in our 5 frame cycle. Need to pitch shift." );
						}
                        
						for ( size_t i = 0; i < table.size( ); i ++ )
						{
							int size = table[ i ] * channels_ * audio_span_->sample_size( );

							if ( frames_[ i ] && frames_[ i ]->get_audio( ) && int( i ) >= start_audio && int( i ) <= final_audio )
							{
								ml::audio_type_ptr temp = ml::audio::pitch( frames_[ i ]->get_audio( ), table[ i ] );
								memcpy( ptr, temp->pointer( ), temp->size( ) );
							}
							else
							{
								memset( ptr, 0, size );
							}

							ptr += size;
						}
					}
				}

				// Now we collect the frame we want - this may, or may not, have audio samples - if it has none, we just return it otherwise we collect the 
				// samples from the span.
				if ( get_position( ) - start < int( frames_.size( ) ) )
				{
					result = frames_[ get_position( ) - start ];
					if ( result && result->get_audio( ) )
					{
						boost::uint8_t *ptr = ( boost::uint8_t *)audio_span_->pointer( );
						for ( int index = start; index < get_position( ); index ++ )
							ptr += table[ index - start ] * channels_ * audio_span_->sample_size( );
						ml::audio_type_ptr aud = ml::audio::allocate( audio_span_, -1, -1, table[ get_position( ) - start ] );
						memcpy( aud->pointer( ), ptr, aud->size( ) );
						result->set_audio( aud );
					}
				}
			}
			else
			{
				// Frame to return
				result = fetch_from_slot( );
			}
		}

		pcos::property prop_enable_;
		pcos::property prop_profile_;
		int fps_num_;
		int fps_den_;
		int frequency_;
		int channels_;
		std::map< pl::wstring, std::vector< int > > profiles_;
		std::vector< ml::frame_type_ptr > frames_;
		ml::audio_type_ptr audio_span_;
};

ml::filter_type_ptr ML_PLUGIN_DECLSPEC create_locked_audio( const pl::wstring &resource )
{
	return ml::filter_type_ptr( new filter_locked_audio( resource ) );
}

} }
