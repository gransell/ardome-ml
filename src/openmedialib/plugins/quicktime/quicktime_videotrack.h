// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

/**
* Definition of class quicktime_videotrack
*/
#ifndef quicktime_videotrack_h
#define quicktime_videotrack_h

//SYSTEM INCLUDES
#include <map>

// OL INCLUDES
#include <openmedialib/ml/frame.hpp>
#include <openpluginlib/pl/string.hpp>	

//LOCAL INCLUDES
#include <openmedialib/plugins/quicktime/quicktime_track.h>


namespace opl = olib::openpluginlib;

namespace olib { namespace openmedialib { namespace ml {

    /**
    * Represents a video track in a Quicktime movie file.
    */
	class quicktime_videotrack : public quicktime_track
    {
        public:
        
		/**
        * Constructor for decoding
		*@param track_num Track identifier 
		*@param qt_movie Quicktime parent movie info
		*@param qt_track Quicktime track info
		*@param qt_media Quicktime media info
        */
        quicktime_videotrack(int track_num,
							Movie& qt_movie,
							Track  qt_track,
							Media  qt_media);

		/**
		* Constructor for encoding
		*@param track_num Track identifier 
		*@param qt_movie Quicktime parent movie info
		*@param width Width of image
		*@param height Height of image
		*@param fps_num Frames-per-second numerator
		*@param fps_den Frames-per-second denominator
		*@param sar_num Sample Aspect Ratio numerator
		*@param sar_den Sample Aspect ratio denominator
		*/
		quicktime_videotrack(int track_num,
							Movie& qt_movie,
							int width,
							int height,
							int fps_num,
							int fps_den,
							int sar_num,
							int sar_den);

        /**
        * Destructor
        */
        ~quicktime_videotrack();
       
		/**
		* Called by the decoded callback function to add a new decoded image to the track
		* should not be invoked directly
		*@param image New decoded image
		*@param display_time Display time of image
		*/
		void add_decoded_image( image_type_ptr image, TimeValue64 display_time);

		/**
		* Decodes & returns the next image in the track
		*@param error Returned error code
		*@param current Current frame number
		*@return Decoded image from media
		*/
		il::image_type_ptr decode( int current );


		/**
		* Encodes an image into the video track
		*@param image Raw image to be compressed and written to track
		*/
		bool encode( image_type_ptr image );

		/**
		* Invoked when there are no more frames to be decompressed
		*/
		void finish_decode( );

		/**
		* Invoked when there are no more frame to be compressed
		*/
		void finish_encode( );

		/**
		* Creates a track for encoding to
		*@param vfourcc Video codec used in encoding
		*@param video_bit_rate Video bit-rate in encoding
		*/
		bool open_for_encode( opl::string vfourcc,
							  int video_bit_rate );

		/**
		* Opens the video track and reads the track meta-data 
		*/
		bool open_for_decode();

		/**
		* Gets the video frame count
		*/
		int get_frame_count() const { return frame_count_; }

		/**
		* Gets the width of the images in pixels
		*/
		int get_width( ) const { return display_area_.right; }

		/**
		* Gets the height of the images in pixels
		*/
		int get_height( ) const { return display_area_.bottom; }

		/**
		* Gets the sample (pixel) aspect ratio
		*/
		void get_sar( int& num, int& den) const { num = sar_num_; den = sar_den_; }

		/**
		* Gets the list of frame positions in the track
		*/
		const std::vector< TimeValue >& get_frame_track_list( ) const { return frame_track_list_ ; }
        
		/**
		* Tells whether the track is in seeking mode
		*/
		bool is_seeking( ) const { return is_seeking_; }

		/**
		* Sets the frame number of the next frame to be returned 
		* Note that when decode() is returned, if the track is encoded using B-frames, then the next frame may not be the one requested
		* eg. frame 56 may come before frame 55 in decode order. Therefore setting the wanted frame may return frames that are temporally
		* after the wanted frame, but need to be decoded beforehand as the the wanted frame is predicted from them
		*/
		void set_wanted_frame(int w );

		/**
		* Image buffer. This image is used inside the QT gworld object to receive the image data.
		* The image data is then copied into separate images for each new frame num ber
		*/
		ml::image_type_ptr image_buffer_;


	private:

		/**
		* Cleans up resources
		*/
		void clean_up();

		/**
		* Create quicktime compression session for encoding
		*/
		bool create_encode_compression_session( );

		/**
		* Create quicktime decompression session for decoding
		*/
		bool create_decode_decompression_session( );

		/**
		* Invoked when seeking. It moves to the previous key-frame to ensure synching, and then decodes up to the target frame specified by the 
		* target track position. This ensures that predicted frames are always decoded correctly. Although the target is specified in terms of track units, the calculations must take place in media units
		* to ensure that the correct position is reached. 
		*@param target_track_time Time (in track units) of the frame to be seeked to
		*@return Frame number (of the media track) of the next frame to be decoded. This may differ from the target frame as, when we have B-frames, they
		* are temporally after the wanted frame, but need to be decoded beforehand as the the wanted frame is predicted from them
		*/
		int decode_from_key_frame(int target_track_time );
		
		/**
		* Ends quicktime compressions session
		*/
		void end_compression_session(  );
		
		/**
		* Display Area
		*/
		Rect	display_area_;

		/**
		* Number of frames in track
		*/
		int		frame_count_;

		/**
		* Last frame position decoded
		*/
		int		frame_position_;

		/**
		* Sample (pixel) aspect ratio
		*/
		int sar_num_; int sar_den_;

		/**
		* Quicktime description of media samples (SDK)
		*/
		ImageDescriptionHandle qt_image_desc_;

		/**
		* Image identifier
		*/
		ImageSequence qt_image_sequence_;

		/**
		* Quicktime decompression session (SDK)
		*/
		ICMDecompressionSessionRef	qt_decompression_session_;

		/**
		* Quicktime compression session (SDK)
		*/
		ICMCompressionSessionRef	qt_compression_session_;

		/**
		* Pixel Buffer to hold image data before encoding in compression session
		*/
		CVPixelBufferRef			qt_pixel_buffer_;

		/**
		* List of all the track positions of the frames (in track units). This allows us quickly access anywhere in the track
		*/
		std::vector < TimeValue >	frame_track_list_;


		/**
		* Is video interlaced?
		*/
		bool	interlace_;

		/**
		* Interlace field order
		*/
		bool	top_field_first_;

		/**
		* Target frame used for keeping track of when a seek is required
		*/
		int wanted_frame_;

		/**
		* Frame number of next frame to be returned 
		*/
		int next_frame_to_return_;

		/**
		* <B>true</B> if we are currently seeking for a frame
		*/
		bool is_seeking_;

		/**
		* Average data rate when encoding (bytes per second)
		*/
		int	average_data_rate_;

	
		
    };

} } }

#endif
