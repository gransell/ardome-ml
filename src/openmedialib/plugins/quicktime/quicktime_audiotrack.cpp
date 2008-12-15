// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#include <openmedialib/plugins/quicktime/quicktime_audiotrack.h>

//OL INCLUDES
#include <openmedialib/ml/openmedialib_plugin.hpp>		// for audio sampling - TODO: replace with utilities.hpp

namespace ml = olib::openmedialib::ml;
using namespace ml;

// DEFINES
// quicktime callback function when compressing audio frames. this function should specfiy the data to be compressed
// it is invoked by calling SCAudioFillBuffer(). It should not be invoked directly.
static ComponentResult provide_audio_input( ComponentInstance component, 
											UInt32 *num_data_packets, 
											AudioBufferList *input_buffer_list, 
											AudioStreamPacketDescription **output_packet_description,
											void *in_ref);

//---------------------------LIFECYCLE------------------------------------------------------------

quicktime_audiotrack::quicktime_audiotrack( int track_num,
										    Movie& qt_movie,
										    Track  qt_track,
										    Media  qt_media,
										    const std::vector< TimeValue >& track_list ):
quicktime_track(track_num,
			   qt_movie,
			   qt_track,
			   qt_media),
frame_track_list_( track_list )
{
 //audio_buffer_ = new char (100000);
 qt_audio_buffer_list_ = (AudioBufferList*) calloc(1, 256);
}

quicktime_audiotrack::quicktime_audiotrack( Movie& qt_movie,
										    int frequency,
											int channels ):
quicktime_track(0, qt_movie),
qt_audio_stream_packet_desc_(NULL),
audio_input_buffer_size_( 0 ),
frame_track_list_( std::vector< TimeValue > ( ) )
{
 
	qt_input_audio_stream_desc_.mChannelsPerFrame = channels;
	qt_input_audio_stream_desc_.mSampleRate = frequency; 
	qt_audio_buffer_list_ = (AudioBufferList*) calloc(1, 256);
	memset( &qt_output_audio_stream_desc_, 0, sizeof(qt_output_audio_stream_desc_) );
}

quicktime_audiotrack::~quicktime_audiotrack( )
{
	CleanUp( );
}

//--------------------------PUBLIC-----------------------------------------------------------------

audio_type_ptr quicktime_audiotrack::decode( int current )
{

	typedef audio< unsigned char, pcm16 > pcm16_audio_type; 
	OSStatus	err;			

	// calc required track position
	TimeValue track_position;
	if( frame_track_list_.size( ) <= current)
	{
		track_position = ((double)(current) * track_units_per_frame_) + (double)GetTrackOffset( qt_track_ );
	}
	else
	{
		track_position = frame_track_list_[current] + GetTrackOffset( qt_track_ );
	}
	
	UInt32 outFlags = 0;
	TimeRecord wanted_time;

	wanted_time.scale = GetMovieTimeScale( qt_parent_movie_ );
	wanted_time.base = NULL;
	wanted_time.value.hi = 0;
	wanted_time.value.lo = track_position;

	// set the wanted time
	err = MovieAudioExtractionSetProperty( qt_audio_extractor_,
										   kQTPropertyClass_MovieAudioExtraction_Movie,
										   kQTMovieAudioExtractionMoviePropertyID_CurrentTime,
										   sizeof(TimeRecord), &wanted_time );

	// calculate number of wanted samples
	//UInt32 audio_frame_count = UInt32( double( qt_audio_stream_desc_.mSampleRate ) / fps( ) );
	UInt32 audio_frame_count = audio_samples_for_frame( current, qt_input_audio_stream_desc_.mSampleRate, fps_num_, fps_den_ );

	// reset buffer size - this is because MovieAudioExtractionFillBuffer modifies the parameter according to the number of 
	// bytes read in last operation; not neccessarily the same as the next operation
	qt_audio_buffer_list_->mBuffers[0].mDataByteSize=audio_frame_count*qt_input_audio_stream_desc_.mBytesPerFrame;

	// fill audio buffer
	err = MovieAudioExtractionFillBuffer( qt_audio_extractor_, &audio_frame_count, qt_audio_buffer_list_, &outFlags );
	unsigned long size = audio_frame_count * ( qt_input_audio_stream_desc_.mBytesPerFrame ) ;
	memcpy( audio_input_buffer_, qt_audio_buffer_list_->mBuffers[0].mData, size  );
	
	// create audio object as 16-bit PCM
	audio_type_ptr audio = audio_type_ptr( new audio_type( pcm16_audio_type( qt_input_audio_stream_desc_.mSampleRate, 
																			 qt_input_audio_stream_desc_.mChannelsPerFrame, 
																			 audio_frame_count ) ) );
	audio->set_position( current );
			
	
	// set audio data
	memcpy( audio->data( ), audio_input_buffer_, audio->size( ) );
			
	return audio;
	
}

bool quicktime_audiotrack::encode( audio_type_ptr audio )
{
	// copy the audio samples to the buffer
	memcpy( audio_input_buffer_+audio_input_buffer_size_, audio->data(), audio->samples( ) * audio->channels( ) * 2 );
	audio_input_buffer_size_+=audio->samples( );

	// calculate the number of packets we wish to pull from input
	UInt32 pull_packets = audio_input_buffer_size_/qt_output_audio_stream_desc_.mFramesPerPacket;
	if( !pull_packets )
		return false;
	
	// set up output buffer
	qt_audio_buffer_list_->mBuffers[0].mNumberChannels = (qt_audio_buffer_list_->mNumberBuffers > 1 ? 1 : qt_output_audio_stream_desc_.mChannelsPerFrame);
	qt_audio_buffer_list_->mBuffers[0].mDataByteSize = 100000;
	qt_audio_buffer_list_->mBuffers[0].mData = audio_output_buffer_;

	// fill output buffer with audio in output format
	OSStatus err = SCAudioFillBuffer( qt_audio_,
									  provide_audio_input,
									  this,
									  &pull_packets,
									  qt_audio_buffer_list_,
									  qt_audio_stream_packet_desc_ );
	if (err)
		return false;
	
	// calculate how much the input has been encoded
	//TODO: this needs to modified for the variable frame-per-packet case (shift calcs down to AddMediaSample2)
	int data_encoded_size = pull_packets * qt_input_audio_stream_desc_.mFramesPerPacket * qt_input_audio_stream_desc_.mBytesPerFrame;
	memmove( audio_input_buffer_, audio_input_buffer_ + data_encoded_size, (audio_input_buffer_size_*qt_input_audio_stream_desc_.mFramesPerPacket*qt_input_audio_stream_desc_.mBytesPerFrame) - data_encoded_size );
	audio_input_buffer_size_ -= pull_packets * qt_output_audio_stream_desc_.mFramesPerPacket;

	// add to the movie -- depending on whether is raw or compressed
	if ( qt_audio_stream_packet_desc_ )
	{
		// if we've got packet descriptions, we need to add them one at a time
		UInt32 i;
			
		// compressed format
		for (i = 0; i < pull_packets; i++)
		{
			err = AddMediaSample2(	qt_media_,
									(UInt8*)qt_audio_buffer_list_->mBuffers[0].mData + qt_audio_stream_packet_desc_[i].mStartOffset,
									qt_audio_stream_packet_desc_[i].mDataByteSize,
									(qt_audio_stream_packet_desc_[i].mVariableFramesInPacket 
											? qt_audio_stream_packet_desc_[i].mVariableFramesInPacket 
											: qt_output_audio_stream_desc_.mFramesPerPacket), //decodeDurationPerSample
									0, 
									(SampleDescriptionHandle)qt_audio_description_,
									1, 
									0, 
									NULL ); 
					if (err)
						return false;
				}
			}
	else 
	{
		// basic PCM format
		ItemCount num_samples = pull_packets * qt_output_audio_stream_desc_.mFramesPerPacket;
		TimeValue64 tv;
		err = AddMediaSample2(	qt_media_,
								(UInt8*)qt_audio_buffer_list_->mBuffers[0].mData,
								qt_audio_buffer_list_->mBuffers[0].mDataByteSize,
								1, 
								0, 
								(SampleDescriptionHandle)qt_audio_description_,
								num_samples,
								0, 
								&tv ); 
		if( err )
			return false;			
	}	
		
	return true;
	
}

void quicktime_audiotrack::finish_encode( )
{
	// end editing and insert media into track
	OSStatus err = EndMediaEdits( qt_media_ );
	TimeValue64 g=GetMediaDuration( qt_media_ );
	err = InsertMediaIntoTrack( qt_track_, 0, 0, GetMediaDuration( qt_media_ ), fixed1 );
		
}

bool quicktime_audiotrack::open_for_decode( )
{
	OSStatus err;
	unsigned long output_layout_size;

	// get number of audio samples & timescale
	audio_sample_count_ = GetMediaSampleCount( qt_media_ );
	track_duration_ = GetTrackDuration( qt_track_ );
	media_duration_ = GetMediaDuration( qt_media_ );
	media_time_scale_ = GetMediaTimeScale( qt_media_ );
	TimeScale movie_time_scale = GetMovieTimeScale( qt_parent_movie_ );

	// calc units per frame
	if( track_units_per_frame_ < 0 )
		track_units_per_frame_ = movie_time_scale / fps( );

	// start an audio extraction session
	MovieAudioExtractionBegin( qt_parent_movie_, 0, &qt_audio_extractor_ );

	// Get the size of the extraction output layout
	err = MovieAudioExtractionGetPropertyInfo( qt_audio_extractor_, 
											   kQTPropertyClass_MovieAudioExtraction_Audio,
                                               kQTMovieAudioExtractionAudioPropertyID_AudioChannelLayout,
                                               NULL, &output_layout_size, NULL );	
	if( err != noErr )
		return false;

	// Allocate memory for the layout
	AudioChannelLayout* qt_audio_channel_layout = (AudioChannelLayout *) calloc( 1, output_layout_size );
	if ( qt_audio_channel_layout == 0 ) 
	{
		err = memFullErr;
		return false;
	}

	// Get the layout for the current extraction configuration.
	// This will have already been expanded into channel descriptions.
	err = MovieAudioExtractionGetProperty( qt_audio_extractor_, 
										   kQTPropertyClass_MovieAudioExtraction_Audio,
										   kQTMovieAudioExtractionAudioPropertyID_AudioChannelLayout,
										   output_layout_size, qt_audio_channel_layout, NULL );	


	//TODO: Handle multi-channel sound
	qt_audio_channel_layout->mChannelLayoutTag = kAudioChannelLayoutTag_Stereo; 
	err = MovieAudioExtractionSetProperty( qt_audio_extractor_,
										   kQTPropertyClass_MovieAudioExtraction_Audio,
										   kQTMovieAudioExtractionAudioPropertyID_AudioChannelLayout,
										   sizeof(qt_audio_channel_layout), qt_audio_channel_layout );


	// get audio stream description
	err = MovieAudioExtractionGetProperty( qt_audio_extractor_,
										   kQTPropertyClass_MovieAudioExtraction_Audio,
										   kQTMovieAudioExtractionAudioPropertyID_AudioStreamBasicDescription,
										   sizeof(qt_input_audio_stream_desc_), &qt_input_audio_stream_desc_, NULL );
	if( err != noErr)
		return false;


	// Convert the stream to return interleaved 16-bit PCM instead of non-interleaved Float32.
	qt_input_audio_stream_desc_.mFormatFlags = kAudioFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked
											| kAudioFormatFlagsNativeEndian;// | kAudioFormatFlagIsInterleaved;
	qt_input_audio_stream_desc_.mBitsPerChannel = sizeof (SInt16) * 8;
	qt_input_audio_stream_desc_.mChannelsPerFrame = qt_input_audio_stream_desc_.mChannelsPerFrame > 2 ? 2 : qt_input_audio_stream_desc_.mChannelsPerFrame;
	qt_input_audio_stream_desc_.mBytesPerFrame = sizeof(SInt16) * qt_input_audio_stream_desc_.mChannelsPerFrame;
	qt_input_audio_stream_desc_.mBytesPerPacket = qt_input_audio_stream_desc_.mBytesPerFrame;

	// Set the new audio extraction stream
	err = MovieAudioExtractionSetProperty( qt_audio_extractor_,
										   kQTPropertyClass_MovieAudioExtraction_Audio,
										   kQTMovieAudioExtractionAudioPropertyID_AudioStreamBasicDescription,
										   sizeof (qt_input_audio_stream_desc_), &qt_input_audio_stream_desc_ );
	if( err != noErr)
		return false;

	// disable mixing of audio channels
	Boolean allChannelsDiscrete = false;
	err = MovieAudioExtractionSetProperty(	qt_audio_extractor_,
 											kQTPropertyClass_MovieAudioExtraction_Movie,
 											kQTMovieAudioExtractionMoviePropertyID_AllChannelsDiscrete,
    										sizeof (Boolean), &allChannelsDiscrete);
	if( err != noErr)
		return false;

	// set up audio buffer - we only use one audio buffer as the data is interleaved
	qt_audio_buffer_list_->mNumberBuffers = 1;
	qt_audio_buffer_list_->mBuffers[0].mNumberChannels = qt_input_audio_stream_desc_.mChannelsPerFrame;
	//qt_audio_buffer_list_->mBuffers[0].mDataByteSize = ( qt_audio_stream_desc_.mSampleRate / ( movie_time_scale / track_units_per_frame_ ) ) * (qt_audio_stream_desc_.mBytesPerFrame );
	qt_audio_buffer_list_->mBuffers[0].mDataByteSize = (  ( (double) qt_input_audio_stream_desc_.mSampleRate ) / fps() ) * ((double)qt_input_audio_stream_desc_.mBytesPerFrame) + 10;
	qt_audio_buffer_list_->mBuffers[0].mData = malloc( qt_audio_buffer_list_->mBuffers[0].mDataByteSize );

	return true;

}

bool quicktime_audiotrack::open_for_encode( )
{
	// create audio track
	qt_track_ = NewMovieTrack( qt_parent_movie_, 0, 0, kFullVolume);
		
	// create underlying audio media
	// the media time scale will be the sample rate of the audio
	qt_media_ = NewTrackMedia( qt_track_, SoundMediaType, (TimeScale)qt_input_audio_stream_desc_.mSampleRate, NULL, 0);
		
	// start adding 
	OSStatus err = BeginMediaEdits( qt_media_ );
	if (err)
		return false;

	return create_encode_compression_session( );
}

//------------------------PRIVATE-------------------------------------------------------------------------------

void quicktime_audiotrack::CleanUp()
{
	// signal to QT audio extraction end
	if( qt_audio_extractor_ )
	{
		MovieAudioExtractionEnd( qt_audio_extractor_ );
		qt_audio_extractor_ = NULL;
	}

	// free audio buffer 
	if( qt_audio_buffer_list_->mBuffers[0].mDataByteSize )
		free ( qt_audio_buffer_list_->mBuffers[0].mData );
}

bool quicktime_audiotrack::create_encode_compression_session( )
{
	// open audio component
	OSStatus err = OpenADefaultComponent(StandardCompressionType, StandardCompressionSubTypeAudio, &qt_audio_ );
    if (err)
		return false;

	//////////////////////// set the input details - 16-bit PCM /////////////////////////////



	qt_input_audio_stream_desc_.mFormatID = kAudioFormatLinearPCM;
	qt_input_audio_stream_desc_.mBitsPerChannel = 16;
    qt_input_audio_stream_desc_.mFormatFlags =  kAudioFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked; //kAudioFormatFlagIsLittleEndian
	qt_input_audio_stream_desc_.mBytesPerFrame = qt_input_audio_stream_desc_.mBytesPerPacket = (qt_input_audio_stream_desc_.mBitsPerChannel/8) * qt_input_audio_stream_desc_.mChannelsPerFrame;
	qt_input_audio_stream_desc_.mFramesPerPacket = 1;

	// set the input desc info 
	err = QTSetComponentProperty( qt_audio_, kQTPropertyClass_SCAudio, 
								  kQTSCAudioPropertyID_InputBasicDescription,
								  sizeof(qt_input_audio_stream_desc_), &qt_input_audio_stream_desc_ );


	/////////////// set the output details ///////////////////////////////////

	qt_output_audio_stream_desc_.mFormatID = kAudioFormatLinearPCM; // kAudioFormatMPEG4AAC; 
	qt_output_audio_stream_desc_.mBitsPerChannel = qt_input_audio_stream_desc_.mBitsPerChannel;
    qt_output_audio_stream_desc_.mFormatFlags = qt_input_audio_stream_desc_.mFormatFlags;
	qt_output_audio_stream_desc_.mSampleRate = qt_input_audio_stream_desc_.mSampleRate;
	qt_output_audio_stream_desc_.mBytesPerFrame = qt_output_audio_stream_desc_.mBytesPerPacket = (qt_input_audio_stream_desc_.mBitsPerChannel/8) * qt_input_audio_stream_desc_.mChannelsPerFrame;
	qt_output_audio_stream_desc_.mFramesPerPacket = 1;
	qt_output_audio_stream_desc_.mChannelsPerFrame = qt_input_audio_stream_desc_.mChannelsPerFrame;

	// create output audio stream description
	err = QTSoundDescriptionCreate( &qt_output_audio_stream_desc_, NULL, 0, NULL, 0, 
									kQTSoundDescriptionKind_Movie_LowestPossibleVersion,
									&qt_audio_description_ );


	err = QTSetComponentProperty( qt_audio_, kQTPropertyClass_SCAudio, 
								  kQTSCAudioPropertyID_BasicDescription,
								  sizeof(qt_output_audio_stream_desc_), &qt_output_audio_stream_desc_ );


	// set buffer
	qt_audio_buffer_list_->mNumberBuffers=1;
 
	// see if the output format is externally framed and requires packet descriptions
	bool requires_packets;
	err = QTGetComponentProperty(	qt_audio_, kQTPropertyClass_SCAudio, 
									kQTSCAudioPropertyID_OutputFormatIsExternallyFramed,
									sizeof(requires_packets), &requires_packets, NULL );
	if (requires_packets)
		qt_audio_stream_packet_desc_ = (AudioStreamPacketDescription*)calloc( 1, sizeof(AudioStreamPacketDescription) );

	return true;
	
}


//------------------------------------STATIC-------------------------------------------------------------------------



static ComponentResult provide_audio_input( ComponentInstance component, 
											UInt32 *num_data_packets, 
											AudioBufferList *input_buffer_list, 
											AudioStreamPacketDescription **output_packet_description,
											void *in_ref)
{
	quicktime_audiotrack* audio_track = static_cast<quicktime_audiotrack*>(in_ref);
	
	//audio_type_ptr audio  = audio_track->get_audio( );
	//input_buffer_list->mNumberBuffers = 1;
	//input_buffer_list->mBuffers[0].mDataByteSize = audio->channels( ) * audio->samples( ) * 2;
	//input_buffer_list->mBuffers[0].mData = audio->data( );

	input_buffer_list->mNumberBuffers = 1;
	input_buffer_list->mBuffers[0].mDataByteSize = audio_track->get_input_buffer_size( );
	input_buffer_list->mBuffers[0].mData = audio_track->get_input_buffer( );

	return noErr;
		
}
	
		
