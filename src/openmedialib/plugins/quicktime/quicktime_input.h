// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

/**
* Definition quicktime_input
*/
#ifndef quicktime_plugin_h
#define quicktime_plugin_h

//QUICKTIME INCLUDES
#ifdef WIN32
#include <qtml.h>
#include <movies.h>
#include <CVPixelBuffer.h>
#include <Gestalt.h>
#include <QuickTimeComponents.h>
#else
#include <Quicktime/qtml.h>
#include <CoreServices/CoreServices.h>
#include <Quicktime/QuickTimeComponents.h>
#endif

// SYSTEM INCLUDES
#include <vector>													// stores tracks
#include <deque>													// stores audio & video ptrs
#include <map>														// stores frames & orders
#include <iostream>

// OL INCLUDES
#include <openmedialib/ml/openmedialib_plugin.hpp>					// images, audio etc
#include <openpluginlib/pl/string.hpp>								// converts file location

// LOCAL INCLUDES
#include <openmedialib/plugins/quicktime/quicktime_videotrack.h>	// movie has video
#include <openmedialib/plugins/quicktime/quicktime_audiotrack.h>	// movie has audio


namespace opl = olib::openpluginlib;
namespace plugin = olib::openmedialib::ml;

namespace olib { namespace openmedialib { namespace ml {


/**
* Represents a Quicktime movie
* This class is used for reading quicktime movie sources
*/
class ML_PLUGIN_DECLSPEC quicktime_input : public input_type
{
	public:

		/**
		* Constructor
		*/
        quicktime_input( opl::wstring resource, const opl::wstring mime_type = L"" ) ;

        /**
        * Destructor
        */
        ~quicktime_input();


		//------------------PUBLIC---------------------------------------------------------

		/**
		* Fetches the next frame from the movie source 
		*@return Frame as specified by the current frame number
		*/
		frame_type_ptr fetch( );


		//------------------ACCESSORS--------------------------------------------------------

		/*
		* Gets number of streams
		*/
		virtual int get_video_streams( ) const { return video_tracks_.size( ); }
		virtual int get_audio_streams( ) const { return audio_tracks_.size( ); }

		// Audio/Visual
		/**
		* Gets the total number of frames in the input
		*/
		int get_frames( ) const;
		
		/**
		* Get frames-per-second
		*/
		virtual void get_fps( int &num, int &den ) const;

		/**
		* Get sample aspect-ratio
		*/
		virtual void get_sar( int &num, int &den ) const;

		/**
		* Get video width
		*/
		virtual int get_width( ) const;

		/**
		* Get video height
		*/
		virtual int get_height( ) const;

		/**
		* Does input have valid video?
		*/
		virtual bool has_video( ) const { return video_index_ != -1 && video_tracks_.size( ) > 0; }

		/**
		* Does input have valid audio?
		*/
		virtual bool has_audio( ) const { return audio_index_ != -1 && audio_tracks_.size( ) > 0; }

		/**
		* Indicates if the plugin is valid
		*/
		bool is_valid( );



		virtual bool is_seekable( ) const { return true; }
		virtual const opl::wstring get_uri( ) const { return uri_; }
		virtual const opl::wstring get_mime_type( ) const { return mime_type_; }

		virtual double fps( ) const
		{
			int num, den;
			get_fps( num, den );
			return den != 0 ? double( num ) / double( den ) : 1;
		}

		virtual bool is_thread_safe( ) const
		{
			return false;
		}

	private:

		/**
		* Cleansup resources
		*/
		void cleanup( );
	
		/**
		* Locates the audio object for a given frame
		*@param frame Frame to set audio into
		*/
		void find_audio( frame_type *frame );

		/**
		* Locates the image object for a given frame
		*@param frame Frame to set image into
		*/
		void find_image( frame_type *frame );

		/**
		* Sets up the input by creating tracks and associated parameters
		*/
		void populate( );

		/**
		* Get active video-track
		*/
		quicktime_videotrack* get_video_track( ) const;

		/**
		* Get active audio-track
		*/
		quicktime_audiotrack* get_audio_track( ) const;

		//-----------------------MEMBERS----------------------------------------------

		/**
		* File specification 
		*/
		FSSpec	movie_file_spec_;

		/**
		* Quicktime movie (SDK)
		*/
		Movie	qt_movie_;

		/**
		* Unique movie ref number
		*/
		short movie_ref_num_;

		/**
		* List of available video tracks
		*/
		std::vector < quicktime_videotrack* > video_tracks_;

		/**
		* List of available audio tracks
		*/
		std::vector < quicktime_audiotrack* > audio_tracks_;

		/**
		* Duration of movie (in time units)
		*/
		TimeValue	movie_duration_;

		/*
		* Units per second
		*/
		TimeScale	movie_time_scale_;


		/**
		* Decoded image buffer
		*/
		std::map < int, image_type_ptr> images_;

		/**
		* Decoded image buffer
		*/
		std::deque < audio_type_ptr > audios_;

		/**
		* Current active video track
		*/
		int video_index_;

		/**
		* Current active audio track
		*/
		int audio_index_;

		/**
		* Indicayed if current plugin is currently valid
		*/
		bool valid_;

		opl::wstring uri_;
		opl::wstring mime_type_;

};

} } }

#endif
