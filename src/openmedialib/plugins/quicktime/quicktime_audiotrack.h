// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

/**
* Definition of class quicktime_audiotrack
*/
#ifndef quicktime_audiotrack_h
#define quicktime_audiotrack_h

//SYSTEM INCLUDES

// OL INCLUDES
#include <openmedialib/ml/audio.hpp>


//LOCAL INCLUDES
#include <openmedialib/plugins/quicktime/quicktime_track.h>


namespace olib { namespace openmedialib { namespace ml {

    /**
    * Represents an audio track in a Quicktime movie file.
    */
	class quicktime_audiotrack : public quicktime_track
    {
        public:
        
		/**
        * Constructor - for decoding
		*@param track_num Track identifier 
		*@param qt_movie Quicktime parent movie info
		*@param qt_track Quicktime track info
		*@param qt_media Quicktime media info
        */
        quicktime_audiotrack(int track_num,
							Movie& qt_movie,
							Track  qt_track,
							Media  qt_media,
							const std::vector< TimeValue >& frame_track_list = std::vector< TimeValue >() );
		/**
        * Constructor - for encoding
		*@param qt_movie Quicktime parent movie info
		*@param frequency Audio sample rate
		*@param channels Number of audio channels
        */
        quicktime_audiotrack( Movie& qt_movie,
							  int frequency,
							  int channels );
							  

        /**
        * Destructor
        */
        ~quicktime_audiotrack();
       
		/**
		* Decodes & returns the next audio sample in the track
		*@param error Returned error code
		*@param current Current audio frame number
		*@return Decoded image from media
		*/
		audio_type_ptr decode( int current );

		/**
		* Encodes a group  of audio samples
		*@param audio Collection of audio samples to encode
		*@return <B>true</B> Success
		*/
		bool encode( audio_type_ptr audio );

		/**
		* Inform track that encoding is early
		*/
		void finish_encode( );

		/**
		* Sets the video meta-data
		*/
		bool open_for_decode();

		/**
		* Creates an audio track for encoding to
		*/
		bool open_for_encode( );

		/**
		* Gets the output audio-frame count
		*@return Number of audio-frames in track. This is the number of audio objects that will potentially be returned
		*/
		int get_frame_count() const { return track_duration_ / track_units_per_frame_; }

		/**
		* Gets current audio
		*/
		audio_type_ptr get_audio( ) const { return audio_to_encode_; }

		char* get_input_buffer( ) { char* c = audio_input_buffer_; return c; }
		int get_input_buffer_size( ) const { return audio_input_buffer_size_ *
												    qt_input_audio_stream_desc_.mBytesPerFrame; }

	private:

		/**
		* Cleans up resources
		*/
		void CleanUp();

		/**
		* Creates an audio compression session
		*/
		bool create_encode_compression_session( );


		//---------------------MEMBERS----------------------------------------------------------

		/**
		* Number of audio samples in track
		*/
		int		audio_sample_count_;

		/**
		* Media time-scale (media units per second)
		*/ 
		TimeScale	media_time_scale_;

		/**
		* Last frame position decoded
		*/
		int			frame_position_;

		/**
		*	Quicktime Audio extractor (SDK) - used when decoding
		*/
		MovieAudioExtractionRef					qt_audio_extractor_;

		/**
		* Quicktime Audio input stream description (SDK) - used when decoding and specifying input format when encoding
		*/
		AudioStreamBasicDescription				qt_input_audio_stream_desc_;

		/**
		* Quicktime Audio output stream description (SDK) - output format when encoding
		*/
		AudioStreamBasicDescription				qt_output_audio_stream_desc_;

		/**
		* Quicktime sound description handle (SDK)
		*/
		SoundDescriptionHandle					qt_audio_description_;

		/**
		* Quicktime audio-buffer list (SDK)
		*/
		AudioBufferList							*qt_audio_buffer_list_;

		/**
		* Quicktime audio component (SDK)
		*/
		 ComponentInstance						qt_audio_;

		/**
		* Quciktime packet descriptions (SDK) - used when encoding VBR audio (eg. AAC)
		*/
		AudioStreamPacketDescription*			qt_audio_stream_packet_desc_;

		/**
		* Buffer for storing audio data output from Quicktime encoder
		* TODO: Use sensible calc figure
		*/
		char									audio_output_buffer_[100000];		

		/**
		* Buffer for storing input audio data to be encoded
		* TODO: calc on the fly
		*/
		char									audio_input_buffer_[100000];

		/**
		* Number of bytes in input buffer
		*/
		int										audio_input_buffer_size_;

		/**
		* List of frame points (in track units) for the audio to sync to
		* For audio only, this list will be empty
		*/
		const std::vector < TimeValue >&				frame_track_list_;

		/**
		* Current audio to be encoded - required for provide_audio_input() access
		*/
		audio_type_ptr		audio_to_encode_;
    };

} } }

#endif
