//
//  utilities.h
//  amf
//
//  Created by Mikael Gransell on 2012-07-17.
//
//

#ifndef amf_utilities_h
#define amf_utilities_h

#include <boost/intrusive_ptr.hpp>
#include "DeckLinkAPI.h"

#include <openmedialib/ml/types.hpp>

void intrusive_ptr_add_ref( IUnknown *p );
void intrusive_ptr_release( IUnknown *p );

namespace amf { namespace openmedialib { namespace decklink { namespace utilities {

typedef boost::intrusive_ptr< IDeckLinkIterator > decklink_iterator_ptr;
typedef boost::intrusive_ptr< IDeckLink > decklink_ptr;
typedef boost::intrusive_ptr< IDeckLinkOutput > decklink_output_ptr;
typedef boost::intrusive_ptr< IDeckLinkInput > decklink_input_ptr;
typedef boost::intrusive_ptr< IDeckLinkMutableVideoFrame > decklink_mutable_video_frame_ptr;

BMDDisplayMode frame_to_display_mode( const olib::openmedialib::ml::frame_type_ptr& frame );
BMDPixelFormat frame_to_pixel_format( const std::wstring &pf );
BMDVideoOutputFlags frame_to_output_flags( const olib::openmedialib::ml::frame_type_ptr& frame );
	
BMDAudioSampleType frame_to_audio_sample_type( const olib::openmedialib::ml::frame_type_ptr& frame );
	
} } } }

#endif
