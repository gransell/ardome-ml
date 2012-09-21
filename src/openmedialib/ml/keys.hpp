// ml - A media library representation.

// Copyright (C) 2012 Vizrt
// Released under the LGPL.

#ifndef ML_KEYS_
#define ML_KEYS_

#include <openmedialib/ml/openmedialib_plugin.hpp>
#include <openmedialib/ml/stream.hpp>

namespace olib { namespace openmedialib { namespace ml { namespace keys {

#ifndef ML_KEY_DEFINE
#	define ML_KEY( name ) extern ML_DECLSPEC olib::openpluginlib::pcos::key name
#else
#	define ML_KEY( name ) ML_DECLSPEC olib::openpluginlib::pcos::key name = olib::openpluginlib::pcos::key::from_string( #name )
#endif

// Once a stream_type is analysed, we set a property on the stream to avoid re-analysis
ML_KEY( analysed );

// Common keys which may be provided in the analysis collect method
ML_KEY( fps_num );
ML_KEY( fps_den );
ML_KEY( packet_size );
ML_KEY( width );
ML_KEY( height );
ML_KEY( pf );
ML_KEY( field_order );
ML_KEY( sar_num );
ML_KEY( sar_den );
ML_KEY( picture_coding_type );
ML_KEY( bit_rate_value );
ML_KEY( temporal_offset );
ML_KEY( temporal_stream );
ML_KEY( temporal_reference );

// Mpeg2 specific key values
ML_KEY( vbv_delay );
ML_KEY( closed_gop );
ML_KEY( broken_link );
ML_KEY( frame_rate_code );
ML_KEY( vbv_buffer_size );
ML_KEY( profile_and_level );
ML_KEY( chroma_format );
ML_KEY( top_field_first );
ML_KEY( frame_pred_frame_dct );
ML_KEY( progressive_frame );
ML_KEY( aspect_ratio_information );

ML_KEY( is_background );
ML_KEY( use_last_image );

ML_KEY( audio_reversed );

ML_KEY( enable );
ML_KEY( uri );

ML_KEY( pts );
ML_KEY( dts );
ML_KEY( has_b_frames );
ML_KEY( timebase_num );
ML_KEY( timebase_den );
ML_KEY( duration );

ML_KEY( codec_id );
ML_KEY( codec_type );
ML_KEY( codec_tag );
ML_KEY( max_rate );
ML_KEY( buffer_size );
ML_KEY( bit_rate );
ML_KEY( bits_per_coded_sample );
ML_KEY( ticks_per_frame );
ML_KEY( avg_fps_num );
ML_KEY( avg_fps_den );
ML_KEY( pid );

ML_KEY( source_timecode );
ML_KEY( source_byte_offset );
ML_KEY( source_position );
ML_KEY( source_format );

} } } }

#endif
