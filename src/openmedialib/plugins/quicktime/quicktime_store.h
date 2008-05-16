// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

/**
* Definition of class Quicktime_store
*/
#ifndef quicktime_store_h
#define quicktime_store_h

//SYSTEM INCLUDES
#ifdef WIN32
#include <qtml.h>								// QT SDK
#include <movies.h>								// QT SDK
#include <QuickTimeComponents.h>				// QT SDK
#include <MediaHandlers.h>						// QT SDK
#include <imagecodec.h>							// QT SDK
#else
#include <Quicktime/qtml.h>						// QT SDK
#include <Quicktime/movies.h>					// QT SDK
#include <Quicktime/QuickTimeComponents.h>		// QT SDK
#include <Quicktime/MediaHandlers.h>			// QT SDK
#include <Quicktime/imagecodec.h>				// QT SDK
#endif

// OL  INCLUDES
#include <openmedialib/ml/store.hpp>								// parent class

//LOCAL INCLUDES
#include <openmedialib/plugins/quicktime/quicktime_videotrack.h>	// movie has video tracks
#include <openmedialib/plugins/quicktime/quicktime_audiotrack.h>	// movie has audio tracks

namespace opl = olib::openpluginlib;
namespace plugin = olib::openmedialib::ml;
namespace il = olib::openimagelib::il;

namespace olib { namespace openmedialib { namespace ml {

/**
* Represents a quicktime movie output file
*/
class ML_PLUGIN_DECLSPEC quicktime_store : public store_type
{
	public:

        /**
        * Constructor
		*@param resource Destination stream
		*@param frame Frame that specifies default output size, sar, fps etc
        */
        quicktime_store( const opl::wstring &resource, const frame_type_ptr &frame );

        /**
        * Destructor
        */
        ~quicktime_store();
       
		
		/**
		* Ensure that all buffered frames are compressed and written out. Call when all frames have been 'pushed' to the store
		*/
		void complete( );
		
		/**
		* Creates a video track based on current property values. Call after creating the object.
		*/
		bool init( );

		/**
		* Pushes a frame to be encoded by quicktime
		*@frame Next frame to be encoded
		*/
		bool push( frame_type_ptr frame );

	private:

		/**
		* Closes movie file and frees resources
		*/
		void cleanup( );

		/**
		* Does input have valid audio?
		*/
		bool has_audio( ) const { return audio_index_ != -1 && audio_tracks_.size( ) > 0; }

		/**
		* Does input have valid video?
		*/
		bool has_video( ) const { return video_index_ != -1 && video_tracks_.size( ) > 0; }

		/**
		* Get active video-track
		*/
		quicktime_videotrack* get_video_track( ) const;

		/**
		* Get active audio-track
		*/
		quicktime_audiotrack* get_audio_track( ) const;


//-------------------------MEMBERS------------------------------------------------------

		/**
		* Quicktime movie (SDK)
		*/
		Movie	qt_movie_;

		/**
		* Output specification 
		*/
		FSSpec	movie_file_spec_;

		/**
		* Movie identifier
		*/
		short movie_resource_num_;

		/**
		* List of available video tracks
		*/
		std::vector < quicktime_videotrack* > video_tracks_;

		/**
		* 
		*/
		std::vector < quicktime_audiotrack* > audio_tracks_;

		/**
		* Current active video track
		*/
		int video_index_;

		/**
		* Current active audio track
		*/
		int audio_index_;

		// Application facing properties
		// TODO: Expose these properly
		pcos::property prop_vfourcc_;
		pcos::property prop_width_;
		pcos::property prop_height_;
		pcos::property prop_aspect_ratio_;
		pcos::property prop_fps_num_;
		pcos::property prop_fps_den_;
		pcos::property prop_video_bit_rate_;
		pcos::property prop_enable_video_;
		pcos::property prop_enable_audio_;
		pcos::property prop_frequency_;
		pcos::property prop_channels_;
		
    };

} } } 

#endif
