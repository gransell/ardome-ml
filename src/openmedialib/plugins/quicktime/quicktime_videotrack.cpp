// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#include <openmedialib/plugins/quicktime/quicktime_videotrack.h>
#include <iostream>

namespace ml = olib::openmedialib::ml;
namespace opl = olib::openpluginlib;
using namespace ml;

// DEFINES
/**
* Quicktime callback funtion invoked when ICMCompressionSessionEncodeFrame is called. This method will be called with
* the encoded data passed as parameter. It is then written to the media file as a block of data.
* This method should not be invoked directly.
*/
static OSStatus write_encoded_frame( void *encoded_frame_class, ICMCompressionSessionRef session, OSStatus error, ICMEncodedFrameRef encoded_frame, void *reserved );

/**
* Quicktime callback funtion invoked when ICMCompressionSessionDecodeFrame is called. This method will be called by
* quicktime with the next decoded frame available. The next frame available will be the next one in display order.
* This method should not be invoked directly.
*/
static void get_decoded_frame( void *video_track, OSStatus result, ICMDecompressionTrackingFlags track_flags, 
					           CVPixelBufferRef qt_pixel_buffer, TimeValue64 display_time, TimeValue64 display_duration, 
							   ICMValidTimeFlags time_flags, void *reserved, void *sourceFrameRefCon );




//---------------------------LIFECYCLE------------------------------------------------------------

quicktime_videotrack::quicktime_videotrack(int track_num,
										   Movie& qt_movie,
										   Track  qt_track,
										   Media  qt_media ):
quicktime_track(track_num,
			   qt_movie,
			   qt_track,
			   qt_media),
sar_num_(0),
sar_den_(0),
frame_position_(0),
interlace_(false),
top_field_first_(true),
is_seeking_(false),
wanted_frame_(-1)
{
	
}

quicktime_videotrack::quicktime_videotrack(int track_num,
										   Movie& qt_movie,
										   int width,
										   int height,
										   int fps_num,
										   int fps_den,
										   int sar_num,
										   int sar_den ):
quicktime_track(track_num,
			    qt_movie),
sar_num_(sar_num),
sar_den_(sar_den),
frame_position_(0),
interlace_(false),
top_field_first_(true),
is_seeking_(false),
wanted_frame_(-1),
average_data_rate_(100000),
qt_image_desc_(NULL)
{

	fps_num_=fps_num;
	fps_den_=fps_den;
	display_area_.right=width;
	display_area_.bottom=height;
	qt_fourcc_codec_=kH264CodecType;
}


quicktime_videotrack::~quicktime_videotrack()
{
	clean_up( );
}

//--------------------------PUBLIC-----------------------------------------------------------------

void quicktime_videotrack::add_decoded_image( image_type_ptr image, TimeValue64 display_time)
{
	// calc display number
	long sample_num = display_time / media_units_per_frame_;

	// set image parameters
	image->set_writable( false );
	double pts = ((double)sample_num) / fps(); 
	image->set_pts( pts );
	image->set_position( sample_num );
	
	if( interlace_ )
		image->set_field_order( top_field_first_ ? ml::image::top_field_first : ml::image::bottom_field_first );

	image_buffer_ = image;
	next_frame_to_return_++;


}

void quicktime_videotrack::set_wanted_frame(int wanted_f )
{
	is_seeking_ = false;

	// move back to previous key frame and decode if this is not the frame we expected (i.e looks like a seek)
	if( wanted_frame_+1 != wanted_f ) 
	{
		is_seeking_=true;
	}

	wanted_frame_ = wanted_f;
}


ml::image_type_ptr quicktime_videotrack::decode( int current )
{
	OSStatus err;
	TimeValue64 track_position;
	TimeValue64 media_time;
	TimeValue64 display_time;
	image_buffer_ = image_type_ptr( );

	int fnum_to_decode=expected_frame_pos_;

	if( is_seeking_ )
		fnum_to_decode = wanted_frame_;

	// check size
	if( current >= get_frame_count( ))
		return ml::image_type_ptr( );

	// calc required track position
	if( frame_track_list_.size() && current < frame_track_list_.size() )
		track_position = frame_track_list_[fnum_to_decode] + GetTrackOffset( qt_track_ );
	else
		track_position = ((double)(fnum_to_decode) * track_units_per_frame_) + (double)GetTrackOffset( qt_track_ );

	if( is_seeking_ )
	{
		// if we are in seek mode, then we need to look for the next correct frame to decode
		// this method returns the next frame to decode in decode order 
		fnum_to_decode = decode_from_key_frame(track_position);
		is_seeking_=false;
		expected_frame_pos_ = fnum_to_decode;
		// convert from frame number to media decode time
		next_frame_to_return_=wanted_frame_;
		//next_frame_to_return_ = wanted_frame_;
		SampleNumToMediaDecodeTime( qt_media_, fnum_to_decode+1, &media_time, NULL );
		SampleNumToMediaDisplayTime( qt_media_, fnum_to_decode+1, &display_time, NULL );
		//next_frame_to_return_=media_time/media_units_per_frame_;
	}
	else
	{
		// convert from track time to media decode time
		display_time = TrackTimeToMediaDisplayTime( track_position, qt_track_ ); 
		SInt64 sampleNumber = 0;
		SampleNumToMediaDecodeTime( qt_media_, fnum_to_decode+1, &media_time, NULL );
		SampleNumToMediaDisplayTime( qt_media_, fnum_to_decode+1, &display_time, NULL );
	}


	// get the size of the encoded data block
	MediaSampleFlags sample_flags=0;
	ByteCount encoded_data_size;
	err = GetMediaSample2( qt_media_, NULL, 0, &encoded_data_size, media_time,
							NULL, NULL, NULL, NULL, NULL, 1, NULL, &sample_flags );
    
    // create memory to hold the encoded data
    UInt8* encoded_data = (UInt8*)malloc( encoded_data_size );

	// extract the raw data corresponding to the frame at the given media time
	ByteCount frame_size=0;
	TimeValue64 sample_time=0;
	TimeValue64 duration_per_sample=0;
	ItemCount number_of_samples;
	
	short num_samples=1;
	err = GetMediaSample2(  qt_media_,
							encoded_data,
							encoded_data_size,			
							&frame_size,
							media_time, 
							NULL,
							NULL,
							NULL,
							NULL,									
							NULL,
							NULL,
							NULL,
							&sample_flags);

	if (err != noErr) 
		return ml::image_type_ptr( );
	
	// set up data for next frame to be decoded
	ICMFrameTimeRecord qt_frame_time = {0};
	memset(&qt_frame_time, 0, sizeof(ICMFrameTimeRecord)); 
    qt_frame_time.recordSize = sizeof(ICMFrameTimeRecord); 
    *(TimeValue64 *) &qt_frame_time.value = display_time; 
    qt_frame_time.scale = media_time_scale_; 
    qt_frame_time.rate = fixed1;  
    qt_frame_time.flags = icmFrameTimeIsNonScheduledDisplayTime;  
	qt_frame_time.frameNumber  = fnum_to_decode+1;
  
	// push encoded data into queue for decoding
    err = ICMDecompressionSessionDecodeFrame( qt_decompression_session_, 
                                              encoded_data,            
                                              encoded_data_size,
                                              NULL, 
                                              &qt_frame_time, 
                                              encoded_data); 

	// push out next decoded frame in display order
	err = ICMDecompressionSessionSetNonScheduledDisplayTime( qt_decompression_session_, next_frame_to_return_*media_units_per_frame_, media_time_scale_, 0);

	// the next available frame should now be set in image_buffer_

	// no-error
	expected_frame_pos_++;
	return image_buffer_;

}

bool quicktime_videotrack::encode( image_type_ptr image )
{

	// convert to required colour-space
	// the native colour space of the Mac is ARGB 32. The mac doesn't seem to like using other colour spaces
	// Remove un-necessary conversion - this will be forced upstream
	//image = il::convert( image, L"a8r8g8b8");
	if( !image)
		return false;

	// create the pixel buffer to be used in encoding
	CVPixelBufferRef pixel_buffer=NULL;
	CVPixelBufferPoolRef pixel_buffer_pool = ICMCompressionSessionGetPixelBufferPool( qt_compression_session_ );
	OSStatus err = CVPixelBufferPoolCreatePixelBuffer( NULL, pixel_buffer_pool, &pixel_buffer ); 
	CVBufferRetain( pixel_buffer );

	// set image data into pixel buffer
	CVPixelBufferLockBaseAddress( pixel_buffer, 0 );
	void* pix_ptr = CVPixelBufferGetBaseAddress( pixel_buffer );
	//pix_ptr = image->data( );
	memcpy( pix_ptr, image->data( ), image->pitch()*image->height() );

	// Pass frame to compression session 
	err = ICMCompressionSessionEncodeFrame( qt_compression_session_,  
										    pixel_buffer,  
											expected_frame_pos_ * media_units_per_frame_, 
											media_units_per_frame_,  
											kICMValidTime_DisplayTimeStampIsValid | kICMValidTime_DisplayDurationIsValid,  
											NULL,  
											NULL,  
											NULL ); 

	CVPixelBufferUnlockBaseAddress( pixel_buffer, 0 );

     if(err) 
		return false; 

	 expected_frame_pos_++;

	 return true;
}

void quicktime_videotrack::finish_decode( )
{
	ICMDecompressionSessionRelease( qt_decompression_session_ );
}

void quicktime_videotrack::finish_encode( )
{
	// end compression
	end_compression_session( );

    // End adding media-samples
	OSStatus err = EndMediaEdits( qt_media_ ); 
       
    // make things a little neater
    ExtendMediaDecodeDurationToDisplayEndTime( qt_media_, NULL); 
          
    // Insert the media into the track
    err = InsertMediaIntoTrack( qt_track_ , GetTrackDuration( qt_track_ ) , 0, GetMediaDisplayDuration( qt_media_ ), fixed1); 
        
     
}

bool quicktime_videotrack::open_for_encode( opl::string vfourcc,
										    int video_bit_rate )
{

	OSErr err = noErr;
	average_data_rate_ = video_bit_rate;
	const char* cc = vfourcc.c_str();
	if(vfourcc.size( )==4)
		qt_fourcc_codec_ = FOURCC( cc[0], cc[1], cc[2], cc[3] );

			
	// set unit scales
	media_time_scale_ = fps_num_;
	media_units_per_frame_ = fps_den_;
	
	// create quicktime track
	qt_track_ = NewMovieTrack( qt_parent_movie_,
							   display_area_.right * fixed1,
							   display_area_.bottom * fixed1,
							   0 );

	if( qt_track_ == 0 ) 
		return false;

	// create underlying media
	qt_media_ = NewTrackMedia( qt_track_, 
							   VideoMediaType,
							   media_time_scale_, 
							   0, 0 );

	if( qt_media_ == 0) 
		return false;

	// start edits
	err = BeginMediaEdits( qt_media_ );
	if( err != noErr ) 
		return false;

	// set up compression session
	create_encode_compression_session( );

	return true;
}

bool quicktime_videotrack::open_for_decode()
{
	OSStatus err;

	// set display size
	display_area_.left = 0;
	display_area_.top = 0;
	GetMovieBox( qt_parent_movie_, &display_area_ );

	// get number of frames
	frame_count_ = GetMediaSampleCount( qt_media_ );
				
	// get durations & time-scales
	track_duration_ = GetTrackDuration( qt_track_ );
	media_duration_ = GetMediaDuration( qt_media_ );
	media_time_scale_ = GetMediaTimeScale( qt_media_ );
	TimeScale movie_time_scale = GetMovieTimeScale( qt_parent_movie_ );

	// calc units per frame
	media_units_per_frame_ = GetMediaDecodeDuration( qt_media_ ) / frame_count_;
	track_units_per_frame_ = (movie_time_scale * media_units_per_frame_) / media_time_scale_;

	// calc fps
	fps_num_ = media_time_scale_;
	fps_den_ = media_units_per_frame_;
	
	// get info about the actual media type used
	qt_image_desc_ = (ImageDescriptionHandle)NewHandleClear(sizeof(ImageDescription));
	GetMediaSampleDescription( qt_media_ , 1, (SampleDescriptionHandle)qt_image_desc_ );
	qt_fourcc_codec_  = (**qt_image_desc_).cType;
	int h;
	if(qt_fourcc_codec_ == FOURCC('v','2','1','0'))
		h=9;

	// get sample aspect ratio
	PixelAspectRatioImageDescriptionExtension pixelAspectRatio;
	OSStatus status;
	status = ICMImageDescriptionGetProperty(qt_image_desc_,
											kQTPropertyClass_ImageDescription,
											kICMImageDescriptionPropertyID_PixelAspectRatio,
											sizeof(pixelAspectRatio), 
											&pixelAspectRatio,        
											NULL);                   
	if(status != noErr)
		return false;

	sar_num_ = pixelAspectRatio.hSpacing;
	sar_den_ = pixelAspectRatio.vSpacing;

	// get interlace setting
	FieldInfoImageDescriptionExtension2 field_info;
	status = ICMImageDescriptionGetProperty(qt_image_desc_,
											kQTPropertyClass_ImageDescription,
											kICMImageDescriptionPropertyID_FieldInfo,
											sizeof(field_info), 
											&field_info,        
											NULL);    
	if(status == noErr)
	{
		if( field_info.fields == kQTFieldsInterlaced )
			interlace_ = true;
		if( field_info.detail == kQTFieldDetailTemporalBottomFirst)
			top_field_first_ = false;
	}

	// look along the track and get out all the frame-start positions - this is recommended by Apple, as frames may not always
	// be equi-distant across the track
	OSType media_type = VideoMediaType;
	int frame_count=0;
    short flags = nextTimeMediaSample + nextTimeEdgeOK;
    TimeValue duration=1;
    TimeValue time = 0;

    while ( time >= 0 && duration > 0 ) 
	{
        GetTrackNextInterestingTime( qt_track_,
									 flags,
									 time,
									 1,
									 &time,
									 &duration );

		// save track position
		frame_track_list_.push_back( time );

        // after the first interesting time, don't include
        // the time we are currently at. 
        flags = nextTimeMediaSample;

		frame_count++;
    }
	
	next_frame_to_return_=0;
	
	// create decode session
	create_decode_decompression_session( );

	return true;

}

//------------------PRIVATE---------------------------------------------------------------------------------

void quicktime_videotrack::clean_up( )
{

	if ( qt_image_desc_ ) 
	{
		DisposeHandle ((Handle)qt_image_desc_ );
		qt_image_desc_=NULL;
	}
	
}

bool quicktime_videotrack::create_encode_compression_session( )
{

	OSStatus err = noErr; 
	ICMEncodedFrameOutputRecord encoded_frame_output_record = {0}; 
	ICMCompressionSessionOptionsRef qt_session_options = NULL; 
   
	// create the compression session
	err = ICMCompressionSessionOptionsCreate(NULL, &qt_session_options );
	if( err )
		return false;

	// enables predicted frames
	err = ICMCompressionSessionOptionsSetAllowTemporalCompression( qt_session_options, true);
	if( err )
		return false;

	// enables B-frames
	err = ICMCompressionSessionOptionsSetAllowFrameReordering( qt_session_options, true);
	if( err )
		return false;

	// set key frame interval
	err = ICMCompressionSessionOptionsSetMaxKeyFrameInterval( qt_session_options, 250);
	if( err )
		return false;

	// ensure we save durations
	err = ICMCompressionSessionOptionsSetDurationsNeeded( qt_session_options, true);
	if( err )
		return false;

	// set the average data rate
	err = ICMCompressionSessionOptionsSetProperty( qt_session_options,
												   kQTPropertyClass_ICMCompressionSessionOptions,
												   kICMCompressionSessionOptionsPropertyID_AverageDataRate, 
												   sizeof(average_data_rate_),
												   &average_data_rate_ );


	// The default quality is codecNormalQuality.
//err = ICMCompressionSessionOptionsSetProperty(sessionOptions,
            //                    kQTPropertyClass_ICMCompressionSessionOptions,
           //                     kICMCompressionSessionOptionsPropertyID_Quality,
          //                      sizeof(compressionQuality),
          //                      &compressionQuality );

	CFNumberRef number = NULL;
	CFMutableDictionaryRef pixelBufferAttributes = NULL;
	pixelBufferAttributes = CFDictionaryCreateMutable( NULL, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks );
	
	int width = display_area_.right;
	int height = display_area_.bottom;
	number = CFNumberCreate( NULL, kCFNumberIntType, &width );
	CFDictionaryAddValue( pixelBufferAttributes, kCVPixelBufferWidthKey, number );
	CFRelease( number );
	
	number = CFNumberCreate( NULL, kCFNumberIntType, &height );
	CFDictionaryAddValue( pixelBufferAttributes, kCVPixelBufferHeightKey, number );
	CFRelease( number );

	// the native colour space of the Mac is ARGB 32. The mac doesn't seem to like using other colour spaces
	OSType pixelFormat = k32ARGBPixelFormat;
	number = CFNumberCreate( NULL, kCFNumberSInt32Type, &pixelFormat );
	CFDictionaryAddValue( pixelBufferAttributes, kCVPixelBufferPixelFormatTypeKey, number );
	CFRelease( number );
	
	// set compression callback functions
	encoded_frame_output_record.encodedFrameOutputCallback = write_encoded_frame; 
    encoded_frame_output_record.encodedFrameOutputRefCon = this; 
    encoded_frame_output_record.frameDataAllocator = NULL; 
      
//	qt_fourcc_codec_= kH264CodecType;
//	qt_fourcc_codec_ = kRawCodecType;
	// create compression session
	err = ICMCompressionSessionCreate( NULL, display_area_.right, display_area_.bottom, qt_fourcc_codec_, media_time_scale_, 
									   qt_session_options, pixelBufferAttributes, &encoded_frame_output_record, &qt_compression_session_ ); 
     if ( err ) 
		 return false;

//	CFRelease( pixelBufferAttributes );
//	ICMCompressionSessionOptionsRelease( qt_session_options );

//


	 return true;
}

bool quicktime_videotrack::create_decode_decompression_session( )
{

	OSType pixel_format = k32ARGBPixelFormat;
	CFMutableDictionaryRef qt_pixel_buffer_attributes = NULL;
	ICMDecompressionSessionOptionsRef qt_session_options = NULL;
	ICMDecompressionTrackingCallbackRecord qt_callback_method;
  
	// Create a dictionary describing the pixel buffers we want to get back.
	qt_pixel_buffer_attributes = CFDictionaryCreateMutable( NULL, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks );
  
	// add width to buffer
	int width = get_width( );
	CFNumberRef w = CFNumberCreate( NULL, kCFNumberSInt32Type, &width );
	CFDictionaryAddValue( qt_pixel_buffer_attributes, kCVPixelBufferWidthKey, w );
	CFRelease( w );

	// add height to buffer
	int height = get_height( );
	CFNumberRef h = CFNumberCreate( NULL, kCFNumberSInt32Type, &height );
	CFDictionaryAddValue( qt_pixel_buffer_attributes, kCVPixelBufferHeightKey, h );
	CFRelease( h );

	// add wanted format to buffer
	CFNumberRef pf = CFNumberCreate( NULL, kCFNumberSInt32Type, &pixel_format );
	CFDictionaryAddValue( qt_pixel_buffer_attributes, kCVPixelBufferPixelFormatTypeKey, pf );
	CFRelease( pf );
  
	// set callback method for when a decoded frame becomes available
	qt_callback_method.decompressionTrackingCallback = get_decoded_frame;
	qt_callback_method.decompressionTrackingRefCon = this;
  
	// create session
	OSStatus err = ICMDecompressionSessionCreate( NULL, qt_image_desc_, qt_session_options, 
        qt_pixel_buffer_attributes, &qt_callback_method, &qt_decompression_session_ );

	image_buffer_ = ml::image_type_ptr();

	return true;
}


int quicktime_videotrack::decode_from_key_frame(int target_track_time )
{
	OSStatus err = noErr;
	SInt64 sync_sample_number, next_sample_number, target_sample_number;
	TimeValue64 target_decode_time, sync_decode_time, target_display_time, current_display_time;
	
	// calc the media time of the track position
	TimeValue64 display_time = TrackTimeToMediaDisplayTime( target_track_time, qt_track_ ); 
	MediaDisplayTimeToSampleNum( qt_media_, display_time, &target_sample_number, NULL, NULL );
	target_sample_number--;
	SampleNumToMediaDisplayTime( qt_media_, target_sample_number+1, &target_display_time, NULL );

	// Find the last key frame (sync sample) at or before the target sample number.
	SampleNumToMediaDecodeTime( qt_media_, target_sample_number+1, &target_decode_time, NULL );
	err = GetMoviesError();
	
	GetMediaNextInterestingDecodeTime( qt_media_,
									   nextTimeSyncSample | nextTimeEdgeOK,
									   target_decode_time,
									   -fixed1,
									   &sync_decode_time,
									   NULL );
	err = GetMoviesError();
	// get the sample number of the key frame
	MediaDecodeTimeToSampleNum( qt_media_, sync_decode_time, &sync_sample_number, NULL, NULL );
	err = GetMoviesError();

	// samples start at 1 so....
	sync_sample_number--;

	// Pick the starting point.
//	if( ( expected_frame_pos_ <= target_sample_number ) && ( sync_sample_number < expected_frame_pos_ ) )
//		next_sample_number = expected_frame_pos_;
//	else
		next_sample_number = sync_sample_number;

	
	// Walk forward in decode order.
	for( ; next_sample_number < target_sample_number; next_sample_number++ ) 
	{
		TimeValue64 sample_decode_time;
		MediaSampleFlags sample_flags = 0;
		ByteCount sample_data_size = 0;
		UInt8 *sample_data = NULL;
		ICMFrameTimeRecord frame_time = {0};
		
		// Get the frame's data size and sample flags.  
		SampleNumToMediaDecodeTime( qt_media_, next_sample_number+1, &sample_decode_time, NULL );
		err = GetMediaSample2( qt_media_, NULL, 0, &sample_data_size, sample_decode_time,
							   NULL, NULL, NULL, NULL, NULL, 1, NULL, &sample_flags );
		
		// if the sample is temporally after the target in display order, then docode this now, as we will need it for playing
		SampleNumToMediaDisplayTime( qt_media_, next_sample_number+1, &current_display_time, NULL );
		if( current_display_time > target_display_time )
			return next_sample_number;

		// We can skip droppable frames before the target.
		if( ( next_sample_number != target_sample_number ) && ( mediaSampleDroppable & sample_flags ) )
			continue;
		
		// Load the frame.
		sample_data = (UInt8*)malloc( sample_data_size );
		err = GetMediaSample2( qt_media_, sample_data, sample_data_size, NULL, sample_decode_time,
                 NULL, NULL, NULL, NULL, NULL, 1, NULL, NULL );
    
		// Set up an immediate decode request -- we don't care about frame times, we just need to pass a flag.
		frame_time.recordSize = sizeof(ICMFrameTimeRecord);
		*(TimeValue64 *)&frame_time.value = sample_decode_time;
		frame_time.scale = media_time_scale_;
		frame_time.rate = fixed1;
		frame_time.frameNumber = next_sample_number+1;
    
		// If we haven't reached the target sample, tell the session not to emit the frame.
		frame_time.flags = icmFrameTimeDoNotDisplay;
    
		// Decode the frame. 
		err = ICMDecompressionSessionDecodeFrame( qt_decompression_session_, 
												  sample_data, sample_data_size, NULL, &frame_time, sample_data );	
	}

	return target_sample_number;
}

void quicktime_videotrack::end_compression_session(  )
{
	// end the compression session
	ICMCompressionSessionCompleteFrames( qt_compression_session_, true, 0, 0 ); 
	ICMCompressionSessionRelease( qt_compression_session_ ); 
}

// callback function after decoding a frame
OSStatus write_encoded_frame( void *encoded_frame_class, ICMCompressionSessionRef session, OSStatus error, ICMEncodedFrameRef encoded_frame, void *reserved )
{
	quicktime_videotrack* vtrack = static_cast<quicktime_videotrack*>(encoded_frame_class);

	// Encode the frame if it has duration 
	if(ICMEncodedFrameGetDecodeDuration( encoded_frame ) > 0) 
	{ 
		error = AddMediaSampleFromEncodedFrame( vtrack->get_media( ) , encoded_frame, NULL);  
    } 

	return error;	

}

// The "tracking callback" for our decompression session.
// It releases source frame buffers when no longer needed, 
void get_decoded_frame( void *video_track, OSStatus result,
						ICMDecompressionTrackingFlags track_flags,
						CVPixelBufferRef qt_pixel_buffer, TimeValue64 display_time, TimeValue64 display_duration,
						ICMValidTimeFlags time_flags, void *reserved, void *encoded_source_data )
{
	quicktime_videotrack* vtrack = static_cast<quicktime_videotrack*>(video_track);
  
	if( kICMDecompressionTracking_ReleaseSourceData & track_flags ) {
		// Our encoded_source_data are the malloced frame data buffers.
		free( encoded_source_data );
	}
  
	if( ( kICMDecompressionTracking_EmittingFrame & track_flags ) && qt_pixel_buffer ) {

		// lock pixel buffer
		CVPixelBufferLockBaseAddress( qt_pixel_buffer, 0 );

		// create image with decoded data
		// the native colour space of the Mac is ARGB 32. The mac doesn't seem to like using other colour spaces
		ml::image_type_ptr image = ml::image::allocate( _CT("a8r8g8b8"), vtrack->get_width( ), vtrack->get_height( ) );
		memcpy( image->data( ), CVPixelBufferGetBaseAddress( qt_pixel_buffer ), image->pitch()*image->height()  );
		vtrack->add_decoded_image( image, display_time );

		// unlock
		CVPixelBufferUnlockBaseAddress( qt_pixel_buffer, 0 );


  }
}

