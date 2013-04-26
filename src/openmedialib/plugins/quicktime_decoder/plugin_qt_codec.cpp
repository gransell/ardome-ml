// rubberband - A rubberband plugin to ml.
//
// Copyright (C) 2009 Charles Yates
// Released under the LGPL.

#include <openmedialib/ml/openmedialib_plugin.hpp>
#include <openmedialib/ml/types.hpp>
#include <openmedialib/ml/frame.hpp>
#include <openmedialib/ml/stream.hpp>
#include <openmedialib/ml/scope_handler.hpp>

#include <openpluginlib/pl/pcos/observer.hpp>

#include <opencorelib/cl/log_defines.hpp>
#include <opencorelib/cl/enforce_defines.hpp>
#include <opencorelib/cl/guard_define.hpp>

#include <boost/bind.hpp>

#include <QuickTime/QuickTime.h>

namespace pl = olib::openpluginlib;
namespace ml = olib::openmedialib::ml;

namespace pcos = olib::openpluginlib::pcos;

namespace olib { namespace openmedialib { namespace ml { namespace quicktime {
	
static const pl::pcos::key key_temporal_reference = pl::pcos::key::from_string( "temporal_reference" );
	
void frame_decoded_callback( void *tracking_ref_con, OSStatus result,
							 ICMDecompressionTrackingFlags tracking_flags,
							 CVPixelBufferRef pixel_buffer, TimeValue64 display_time,
							 TimeValue64 display_duration,
							 ICMValidTimeFlags valid_time_flags,
							void *reserved, void *sourceFrameRefCon );	

class ML_PLUGIN_DECLSPEC filter_qt_decode : public filter_simple
{
public:
	filter_qt_decode( )
	: filter_simple( )
	, initialized_( false )
	, decompression_session_( NULL )
	, expected_( 0 )
	, image_( )
	, next_frame_to_decoder_( 0 )
	, sar_( 1, 1 )
	, field_order_( ml::image::progressive )
	, lru_cache_( )
	{
		lru_cache_ = ml::the_scope_handler::Instance().lru_cache( L"temp" );
	}

	// Indicates if the input will enforce a packet decode
	bool requires_image( ) const { return false; }

	// Return the filter id
	const std::wstring get_uri( ) const { return L"qt_decode"; }

	// Return the number of slots that can be connected to this filter
	const size_t slot_count( ) const { return 1; }

	void frame_decoded( void *tracking_ref_con, OSStatus result,
						ICMDecompressionTrackingFlags tracking_flags,
						CVPixelBufferRef pixel_buffer, TimeValue64 display_time,
						TimeValue64 display_duration,
						ICMValidTimeFlags valid_time_flags,
						void *reserved, void *sourceFrameRefCon)
	{
		if( kICMDecompressionTracking_ReleaseSourceData & tracking_flags ) {
			// Since the stream owns the data being decoded we let it handle dealloc
		}
		
		if( ( kICMDecompressionTracking_EmittingFrame & tracking_flags ) && pixel_buffer ) {
			
			// lock pixel buffer
			CVPixelBufferLockBaseAddress( pixel_buffer, 0 );
			
			// create image with decoded data
			// the native colour space of the Mac is ARGB 32. The mac doesn't seem to like using other colour spaces
			ml::image_type_ptr image = ml::image::allocate( "yuv422", 1920, 1080 );
			image->set_sar_num( sar_.num );
			image->set_sar_den( sar_.den );
			image->set_field_order( field_order_ );
			
			void *p = CVPixelBufferGetBaseAddress( pixel_buffer );
			memcpy( image->data( 0 ), p, image->size( ) );
			
			// unlock
			CVPixelBufferUnlockBaseAddress( pixel_buffer, 0 );
			
			// Store image in cache
			lru_cache_->insert_image_for_position( lru_key_for_position( display_time ), image ); 
		}
		
	}	
	
protected:
	void do_fetch( frame_type_ptr &result )
	{
		result = fetch_from_slot( 0 );

		int wanted_position = get_position( );
		
		lru_cache_type::key_type my_key = lru_key_for_position( wanted_position );
		ml::image_type_ptr img = lru_cache_->image_for_position( my_key );
		
		if( img )
		{
			result->set_image( img, true );
			return;
		}
		
		if( !initialized_ )
		{
			ARENFORCE_MSG( setup_decoder( result ), "Failed to set up decompression session" );
			
			initialized_ = true;
		}
		
		if( !result->get_stream( ) ) return;
		
		// Get the number of the key frame of this stream to calculate display time of the frames sent to the decoder
		int key = result->get_stream( )->key( );
		
		// Push data into the decoder until we get the callback that a frame has been decoded
		while( 1 )
		{
			// Check if the image that we want is already available in the decoder
			OSStatus err = ICMDecompressionSessionSetNonScheduledDisplayTime( decompression_session_, 
																			  wanted_position, 25, 0);
			
			if( lru_cache_->image_for_position( my_key ) ) break;
			
			input_type_ptr input = fetch_slot( 0 );
			input->seek( next_frame_to_decoder_ );
			frame_type_ptr current_frame = input->fetch( );
			
			stream_type_ptr strm;
			ARENFORCE_MSG( strm = current_frame->get_stream( ), "Failed to get stream from frame" );
			
			// Get temporal offset so that we know the display time of current_frame
			ARENFORCE_MSG( strm->properties( ).get_property_with_key( key_temporal_reference ).valid( ),
						  "Stream does not have a temporal reference property" );
			int temp_ref = strm->properties( ).get_property_with_key( key_temporal_reference ).value< int >( );
			
			ICMFrameTimeRecord frame_time;
			memset( &frame_time, 0, sizeof( ICMFrameTimeRecord ) );
			frame_time.recordSize = sizeof( ICMFrameTimeRecord );
			frame_time.value = SInt64ToWide( key + temp_ref );
			frame_time.scale = 25;
			frame_time.duration = 1;
			frame_time.rate = fixed1;
			frame_time.flags = icmFrameTimeIsNonScheduledDisplayTime;
			frame_time.frameNumber = current_frame->get_position( );
			
			
			err = ICMDecompressionSessionDecodeFrame( decompression_session_,
													  strm->bytes( ),
													  strm->length( ),
													  NULL,
													  &frame_time,
													  strm->bytes( ) );
						
			++next_frame_to_decoder_;
		}
		
		// Reset the position of our input
		fetch_slot( 0 )->seek( get_position( ) );
		
		ml::image_type_ptr image;
		ARENFORCE_MSG( image = lru_cache_->image_for_position( my_key ), "Failed to decode image" );
		result->set_image( image, true );
	}
	
	bool setup_decoder( const frame_type_ptr& frame )
	{
		// Make a CFDictionary that describes the pixel buffers we want the decompression session to output.  
		// We want them to be the right dimensions and pixel format; we want them to be compatible with 
		// CGBitmapContext and CGImage.  
		
		// Note: if we didn't need to draw on frames with CG or display the frames in an HIImageView, we 
		// could improve performance by getting the compression session's preferred pixel buffer attributes
		// via the kICMCompressionSessionPropertyID_CompressorPixelBufferAttributes property
		// and passing that when creating the decompression session.
		CFMutableDictionaryRef pixel_buffer_attributes = CFDictionaryCreateMutable( NULL, 
																				    0,
																				    &kCFTypeDictionaryKeyCallBacks,
																				    &kCFTypeDictionaryValueCallBacks );
		ARGUARD( boost::bind( &CFRelease, (CFTypeRef)pixel_buffer_attributes ) );
		
		int width = 1920, height = 1080;
		CFNumberRef number = CFNumberCreate( NULL, kCFNumberIntType, &width );
		CFDictionaryAddValue( pixel_buffer_attributes, kCVPixelBufferWidthKey, number );
		CFRelease( number );
		
		number = CFNumberCreate( NULL, kCFNumberIntType, &height );
		CFDictionaryAddValue( pixel_buffer_attributes, kCVPixelBufferHeightKey, number );
		CFRelease( number );
		
//		OSType pixel_format = kYVYU422PixelFormat;
		OSType pixel_format = kYUVSPixelFormat;
		number = CFNumberCreate( NULL, kCFNumberSInt32Type, &pixel_format );
		CFDictionaryAddValue( pixel_buffer_attributes, kCVPixelBufferPixelFormatTypeKey, number );
		CFRelease( number );
		
//		CFDictionaryAddValue( pixel_buffer_attributes, kCVPixelBufferCGBitmapContextCompatibilityKey, kCFBooleanTrue );
//		CFDictionaryAddValue( pixel_buffer_attributes, kCVPixelBufferCGImageCompatibilityKey, kCFBooleanTrue );
		
		ICMDecompressionTrackingCallbackRecord callback_record;
		callback_record.decompressionTrackingCallback = &frame_decoded_callback;
		callback_record.decompressionTrackingRefCon = this;
		
		ImageDescriptionHandle image_desc = frame_to_image_desc( frame );
		ARGUARD( boost::bind( &DisposeHandle, (Handle)image_desc ) );
		
		PixelAspectRatioImageDescriptionExtension pasp;
		OSStatus status = ICMImageDescriptionGetProperty( image_desc,
														  kQTPropertyClass_ImageDescription,
														  kICMImageDescriptionPropertyID_PixelAspectRatio,
														  sizeof( pasp ), 
														  &pasp,        
														  NULL );
		if(status == noErr)
		{
			sar_.num = pasp.hSpacing;
			sar_.den = pasp.vSpacing;
		}
		
		
		FieldInfoImageDescriptionExtension2 field_info;
		status = ICMImageDescriptionGetProperty(image_desc,
												kQTPropertyClass_ImageDescription,
												kICMImageDescriptionPropertyID_FieldInfo,
												sizeof(field_info), 
												&field_info,        
												NULL);
		if(status == noErr)
		{
			if( field_info.fields == kQTFieldsInterlaced )
			{
				if( field_info.detail == kQTFieldDetailTemporalBottomFirst)
					field_order_ = ml::image::bottom_field_first;
				else
					field_order_ = ml::image::top_field_first;
			}
		}

		ICMDecompressionSessionOptionsRef session_opts = NULL;
		OSStatus err = ICMDecompressionSessionCreate( NULL, image_desc, session_opts, 
													  pixel_buffer_attributes, &callback_record,
													  &decompression_session_ );
		if( err != noErr ) {
			ARLOG_ERR( "ICMDecompressionSessionCreate failed (%1%)")( err );
			return false;
		}
		
		ICMDecompressionSessionOptionsRelease( session_opts );
		
		return true;
	}

private:
	
	lru_cache_type::key_type lru_key_for_position( boost::int32_t pos )
	{
		lru_cache_type::key_type my_key( pos, fetch_slot( 0 )->get_uri() );
//		if( !scope_uri_key_.empty( ) )
//			my_key.second = scope_uri_key_;
		
		return my_key;
	}
	
	// Translate a frame and its stream to a ImageDescriptionHandle used by the decompression session
	ImageDescriptionHandle frame_to_image_desc( const frame_type_ptr& f )
	{
		stream_type_ptr strm;
		ARENFORCE_MSG( strm = f->get_stream( ), "Failed to get stream from frame. Can not create image description" );
		
		ImageDescriptionHandle ret = (ImageDescriptionHandle)NewHandleClear( sizeof( ImageDescription ) );
		
		(*ret)->idSize = sizeof( ImageDescription );
		// Codec four cc code
		(*ret)->cType = 'xd5c';
		(*ret)->temporalQuality = codecMaxQuality;
		(*ret)->spatialQuality = codecMaxQuality;
		(*ret)->width = strm->size( ).width;
		(*ret)->height = strm->size( ).height;
		(*ret)->hRes = 72 << 16;
		(*ret)->vRes = 72 << 16;
		(*ret)->frameCount = 1;
		(*ret)->depth = 24;
		(*ret)->clutID = -1; // no color lookup table...
		
		return ret;
	}
	
	// So we know if we need to setup codec
	bool initialized_;
	// Codec instance
	ICMDecompressionSessionRef decompression_session_;
	// The expected position. So that we know if we have to seek to closest i-frame or not
	boost::int32_t expected_;
	// When a decode has succeeded
	ml::image_type_ptr image_;
	// How far ahead of the actual position we are
	boost::int32_t next_frame_to_decoder_;
	// The sar that we are going to get out
	fraction sar_;
	// Field order information collected from decoder.
	ml::image::field_order_flags field_order_;
	// Cache images that have been decoded
	lru_cache_type_ptr lru_cache_;
};


//
// Plugin object
//

class ML_PLUGIN_DECLSPEC plugin : public openmedialib_plugin
{
public:
	virtual input_type_ptr input(  const std::wstring &spec )
	{
		return input_type_ptr( );
	}

	virtual store_type_ptr store( const std::wstring &name, const frame_type_ptr &frame )
	{
		return store_type_ptr( );
	}

	virtual filter_type_ptr filter( const std::wstring &spec )
	{
		if ( spec == L"qt_decode" )
			return filter_type_ptr( new filter_qt_decode( ) );
		
		return filter_type_ptr( );
	}
};
	
void frame_decoded_callback( void *tracking_ref_con, OSStatus result,
							ICMDecompressionTrackingFlags tracking_flags,
							CVPixelBufferRef pixel_buffer, TimeValue64 display_time,
							TimeValue64 display_duration,
							ICMValidTimeFlags valid_time_flags,
							void *reserved, void *sourceFrameRefCon )
{
	filter_qt_decode* filter = static_cast< filter_qt_decode* >( tracking_ref_con );
	filter->frame_decoded( tracking_ref_con, result,
						  tracking_flags, pixel_buffer,
						  display_time, display_duration,
						  valid_time_flags, reserved, sourceFrameRefCon );
}
	

} } } }

//
// Access methods for openpluginlib
//

extern "C"
{
	ML_PLUGIN_DECLSPEC bool openplugin_init( void )
	{
		return true;
	}

	ML_PLUGIN_DECLSPEC bool openplugin_uninit( void )
	{
		return true;
	}
	
	ML_PLUGIN_DECLSPEC bool openplugin_create_plugin( const char*, pl::openplugin** plug )
	{
		*plug = new ml::quicktime::plugin;
		return true;
	}
	
	ML_PLUGIN_DECLSPEC void openplugin_destroy_plugin( pl::openplugin* plug )
	{ 
		delete static_cast< ml::quicktime::plugin * >( plug ); 
	}
}
