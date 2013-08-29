
#include "decklink_utilities.h"

#include <opencorelib/cl/enforce_defines.hpp>
#include <openmedialib/ml/frame.hpp>
#include <openmedialib/ml/audio.hpp>
#include <openimagelib/il/basic_image.hpp>

namespace il = olib::openimagelib::il;
namespace ml = olib::openmedialib::ml;

namespace amf { namespace openmedialib { namespace decklink { namespace utilities {

BMDDisplayMode frame_to_display_mode( const ml::frame_type_ptr& frame )
{
	il::image_type_ptr img = frame->get_image();
	int fps_num = frame->get_fps_num();
	int fps_den = frame->get_fps_den();
	
	if( frame->height() == 1080 )
	{
		if( img->field_order() != il::progressive )
		{
			if( fps_num == 25 && fps_den == 1 )
			{
				return bmdModeHD1080i50;
			}
			else if( fps_num == 30000 && fps_den == 1001 )
			{
				return bmdModeHD1080i5994;
			}
			//else if( fps_num == 30 && fps_den == 1 )
			//{
				//return bmdModeHD1080i60;
			//}
		}
		else
		{
			if( fps_num == 25 && fps_den == 1 )
			{
				return bmdModeHD1080p25;
			}
			else if( fps_num == 24000 && fps_den == 1001 )
			{
				return bmdModeHD1080p2398;
			}
			else if( fps_num == 24 && fps_den == 1 )
			{
				return bmdModeHD1080p24;
			}
			else if( fps_num == 30000 && fps_den == 1001 )
			{
				return bmdModeHD1080p2997;
			}
			else if( fps_num == 30 && fps_den == 1 )
			{
				return bmdModeHD1080p30;
			}
			else if( fps_num == 50 && fps_den == 1001 )
			{
				return bmdModeHD1080p50;
			}
			else if( fps_num == 60000 && fps_den == 1001 )
			{
				return bmdModeHD1080p5994;
			}
			else if( fps_num == 60 && fps_den == 1 )
			{
				return bmdModeHD1080p6000;
			}
		}
	}
	else if( frame->height() == 720 )
	{
		if( fps_num == 50 && fps_den == 1 )
		{
			return bmdModeHD720p50;
		}
		else if( fps_num == 60000 && fps_den == 1001 )
		{
			return bmdModeHD720p5994;
		}
		else if( fps_num == 60 && fps_den == 1 )
		{
			return bmdModeHD720p60;
		}
	}
	else // Assume SD material
	{
#if 0
		if( img->field_order() == il::progressive )
		{
			if( fps_num == 30000 && fps_den == 1001 )
			{
				return bmdModeNTSCp;
			}
			else if( fps_num == 25 && fps_den == 1 )
			{
				return bmdModePALp;
			}
		}
		else
#endif
		{
			if( fps_num == 30000 && fps_den == 1001 )
			{
				return bmdModeNTSC;
			}
			else if( fps_num == 24000 && fps_den == 1001 )
			{
				return bmdModeNTSC2398;
			}
			else if( fps_num == 25 && fps_den == 1 )
			{
				return bmdModePAL;
			}    		
		}
	}
	
	ARENFORCE_MSG( false, "Could not convert frame to decklink display mode" )
		( fps_num )( fps_den )( frame->height() )( img->field_order() );

	return bmdDisplayModeNotSupported;
}
	
	
BMDPixelFormat frame_to_pixel_format( const std::wstring pf )
{
	if( pf == L"uyv422" )
	{
		return bmdFormat8BitYUV;
	}
	else if( pf == L"a8r8g8b8" )
	{
		return bmdFormat8BitARGB;
	}
	else if( pf == L"b8g8r8a8" )
	{
		return bmdFormat8BitBGRA;
	}
    // bmdFormat10BitYUV                                            = 'v210',
    // bmdFormat10BitRGB                                            = 'r210'	// Big-endian RGB 10-bit per component with SMPTE video levels (64-960). Packed as 2:10:10:10
	ARENFORCE_MSG( false, "Unsupported pf %s. Currently support uyv422, a8r8g8b8, b8g8r8a8" )( pf );
	return bmdFormat8BitYUV;
}
	
	
BMDVideoOutputFlags frame_to_output_flags( const ml::frame_type_ptr& frame )
{
	// Currently dont support anything but default
	return bmdVideoOutputFlagDefault;
    //bmdVideoOutputVANC                                           = 1 << 0,
    //bmdVideoOutputVITC                                           = 1 << 1,
    //bmdVideoOutputRP188                                          = 1 << 2,
    //bmdVideoOutputDualStream3D                                   = 1 << 4

}
	
	
BMDAudioSampleType frame_to_audio_sample_type( const ml::frame_type_ptr& frame )
{
	ml::audio_type_ptr aud = frame->get_audio();
	ARENFORCE_MSG( aud, "No audio on frame when trying to get sample type" );
	
	if( aud->id() == ml::audio::pcm16_id )
	{
		return bmdAudioSampleType16bitInteger;
	}
	else if( aud->id() == ml::audio::pcm32_id )
	{
		return bmdAudioSampleType32bitInteger;
	}
	
	ARENFORCE_MSG( false, "Unsupported audio sample width" )( aud->af() );

	return 0;
}
	
} } } }
