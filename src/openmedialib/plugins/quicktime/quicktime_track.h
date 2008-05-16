// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

/**
* Definition of class QuicktimeTrack
*/
#ifndef quicktime_track_h
#define quicktime_track_h

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

//LOCAL INCLUDES


//DEFINES
#define FOURCC(a,b,c,d) ((a << 24) | (b << 16) | (c << 8) | (d))		// fourcc


namespace olib { namespace openmedialib { namespace ml {


    /**
    * Represents a track in a Quicktime movie file.
    */
    class quicktime_track
    {
        public:

        /**
        * Constructor for decoding
		*@param track_num Track identifier 
		*@param qt_movie Quicktime parent movie info
		*@param qt_track Quicktime track info
		*@param qt_media Quicktime media info
        */
        quicktime_track( int track_num,
						 Movie& qt_movie,
						 Track  qt_track,
						 Media  qt_media );


		/**
        * Constructor for encoding
		*@param track_num Track identifier 
		*@param qt_movie Quicktime parent movie info
        */
        quicktime_track( int track_num,
						 Movie& qt_movie );
						
	
        /**
        * Destructor
        */
        ~quicktime_track();
       
		/**
		* Retrieves the track meta-data
		*/
		virtual bool open_for_decode();

		/**
		* Get frames per second
		*/
		virtual double fps() const;

		/**
		* Get frames per second
		*@param num Returns fps numerator
		*@param den Returns fps denominator
		*/
		virtual void get_fps( int &num, int &den ) const { num = fps_num_; den = fps_den_; };

		/**
		* Getsn the underlying media
		*/
		Media get_media( ) const { return qt_media_; }

		
		/**
		* Gets the number of track units per frame
		*/
		double get_track_units_per_frame( ) { return track_units_per_frame_; }

		/**
		* Sets the fps
		*/
		virtual void set_fps( int num, int den ) { fps_num_ = num, fps_den_ = den; };

		/**
		* Set the number of media track units per output frame
		*/
		void set_track_units_per_frame( int units) { track_units_per_frame_ = units; }

	protected:
		
		/**
        * Quicktime track identifier
		*/
		int			track_num_;

		/**
		* Quicktime parent movie of track (SDK)
		*/
		Movie		&qt_parent_movie_;

		/**
		* Quicktime track (SDK)
		*/
		Track		qt_track_;

		/**
		* Quicktime media (SDK)
		*/
        Media		qt_media_;

		/**
		* Track duration (in movie time units)
		*/ 
		TimeValue	track_duration_;

		/**
		* Media duration (in media time units)
		*/
		TimeValue	media_duration_;

		/**
		* Frames per second
		*/
		int fps_num_; int fps_den_;

		/**
		* Media time-scale (media units per second)
		*/ 
		TimeScale	media_time_scale_;

		/**
		* Media units per single frame
		*/
		int	media_units_per_frame_;
			
		/**
		* Track units per single frame
		*/
		double	track_units_per_frame_;

		/**
		* fourcc codec description
		*/
		CodecType	qt_fourcc_codec_;

		/**
		* Next expected frame position
		*/
		TimeValue	expected_frame_pos_;

    };

} } } 

#endif
