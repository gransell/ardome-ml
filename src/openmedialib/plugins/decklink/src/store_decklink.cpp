
#include <deque>

#include <opencorelib/cl/lru.hpp>
#include <opencorelib/cl/enforce_defines.hpp>

#include <openimagelib/il/basic_image.hpp>

#include <openmedialib/ml/ml.hpp>

#include <openmedialib/ml/stream.hpp>
#include <openmedialib/ml/frame.hpp>
#include <openmedialib/ml/store.hpp>

#include "decklink_utilities.h"

namespace pl = olib::openpluginlib;
namespace il = olib::openimagelib::il;
namespace ml = olib::openmedialib::ml;
namespace du = amf::openmedialib::decklink::utilities;

namespace amf { namespace openmedialib { 

class ML_PLUGIN_DECLSPEC store_decklink : public ml::store_type, public IDeckLinkVideoOutputCallback, public IDeckLinkAudioOutputCallback
{
private:
	du::decklink_output_ptr device_;
	ml::frame_type_ptr last_frame_;
	std::deque< std::pair< int, il::image_type_ptr > > downloaded_image_;
	std::deque< std::pair< int, ml::audio_type_ptr > > downloaded_audio_;
	boost::mutex mutex_image_;
	boost::mutex mutex_audio_;
	boost::condition_variable cond_image_;
	boost::condition_variable cond_audio_;
	// The amount of frames that should be prerolled to the decklink hardware
	pl::pcos::property prop_preroll_;
	
	BMDPixelFormat decklink_pf_;
	BMDTimeValue frame_duration_;
	BMDTimeScale time_scale_;
	bool started_;
	int frames_;
	
public:
	store_decklink( const std::wstring & resource, const ml::frame_type_ptr &frame )
	: store_type()
	, device_( )
	, last_frame_( frame )
	, prop_preroll_( pl::pcos::key::from_string( "preroll" ) )
	, decklink_pf_( bmdFormat8BitYUV )
	, started_( false )
	, frames_( 0 )
	{
		properties().append( prop_preroll_ = 5 );
	}
	
	virtual ~store_decklink( )
	{
		// Stop the audio and video output streams immediately
		device_->StopScheduledPlayback(0, NULL, 0);

		device_->DisableAudioOutput();
		device_->DisableVideoOutput();
	}
	
	virtual bool init( )
	{
		du::decklink_iterator_ptr dli( CreateDeckLinkIteratorInstance(), false );
		if( !dli ) return false;
		
		IDeckLink *link = NULL;
		while( dli->Next( &link ) == S_OK )
		{
			IDeckLinkOutput *output = NULL;
			if( link->QueryInterface( IID_IDeckLinkOutput, reinterpret_cast< void** >( &output ) ) == S_OK )
			{
				device_ = du::decklink_output_ptr( output );
				break;
			}
		}

		ARENFORCE_MSG( device_, "Failed to find a valid decklink output device" );

		BMDDisplayMode d_mode = du::frame_to_display_mode( last_frame_ );
		decklink_pf_ = du::frame_to_pixel_format( last_frame_ );
		BMDVideoOutputFlags output_flags = du::frame_to_output_flags( last_frame_ );
		BMDDisplayModeSupport disp_mode_supported;
		IDeckLinkDisplayMode *disp_mode = NULL;
		ARENFORCE_MSG( device_->DoesSupportVideoMode( d_mode, decklink_pf_, output_flags, &disp_mode_supported, &disp_mode ) == S_OK,
					   "Failed to check for support of video mode" )( disp_mode )( decklink_pf_ )( output_flags );
		
		ARENFORCE_MSG( disp_mode, "Display mode not supported" )( disp_mode )( decklink_pf_ )( output_flags );
		
		ARENFORCE_MSG( disp_mode->GetFrameRate( &frame_duration_, &time_scale_ ) == S_OK,
					   "Failed to get frame duration and time scale from output device" );
		
		ARENFORCE_MSG( device_->SetScheduledFrameCompletionCallback( this ) == S_OK, "Failed to set frame completion callback" );
		ARENFORCE_MSG( device_->EnableVideoOutput( d_mode, output_flags ) == S_OK, "Failed to enable video output" )( d_mode )( output_flags );
		
		if( last_frame_->get_audio() )
		{
			// Decklink only supports 48 kHz atm.
			BMDAudioSampleRate sample_rate = bmdAudioSampleRate48kHz;
			BMDAudioSampleType sample_type = du::frame_to_audio_sample_type( last_frame_ );
			
			ARENFORCE_MSG( device_->EnableAudioOutput( sample_rate,  sample_type, last_frame_->get_audio()->channels(), bmdAudioOutputStreamContinuous ) == S_OK,
						  "Failed to enable audio output" );
			
			ARENFORCE_MSG( device_->SetAudioCallback( this ) == S_OK, "Failed to set audio callback" );
		}
		
		return true;
	}
	
/*
		bool result = frame && frame->get_image( ) && frame->get_audio( );

		if ( result && video_.wait_for_space( boost::posix_time::milliseconds( 500 ) )
			video_.append( frame->get_position( ), frame->get_image( ) );
		else
			result = false;
		
		if ( result && audio_.wait_for_space( boost::posix_time::milliseconds( 500 ) )
			audio_.append( frame->get_position( ), frame->get_audio( ) );
		else
			result = false;

		return result;
*/

	virtual bool push( ml::frame_type_ptr frame )
	{
		// Make sure that we decode on this thread so that its not done on the display thread
		il::image_type_ptr img = frame->get_image( );
		ml::audio_type_ptr aud = frame->get_audio( );

		if ( img )
		{
			// Now block until we need to push more frame into the queue
			boost::mutex::scoped_lock lck( mutex_image_ );

			if ( !aud && downloaded_image_.size() >= prop_preroll_.value< int >( ) && !started_ )
			{
				device_->StartScheduledPlayback( 0, 100, 1.0 );
				started_ = true;
			}
			
			while( downloaded_image_.size() >= prop_preroll_.value< int >( ) )
				cond_image_.wait( lck );
			
			IDeckLinkMutableVideoFrame *new_frame = NULL;
			boost::int32_t bytes_per_row = img->pitch();
			ARENFORCE_MSG( device_->CreateVideoFrame( img->width(), img->height(), bytes_per_row, decklink_pf_, bmdFrameFlagDefault, &new_frame) == S_OK,
						   "Failed to allocate video frame on device" );
			
			du::decklink_mutable_video_frame_ptr temp( new_frame, false );
			
			char *data = 0;
			ARENFORCE_MSG( temp->GetBytes( (void **)&data ) == S_OK, "Failed to get data pointer to downloaded frame" );
			memcpy( data, img->data(), img->size() );
			
			downloaded_image_.push_back( std::make_pair( frames_, img ) );

			device_->ScheduleVideoFrame( new_frame, frames_ * frame_duration_, frame_duration_, time_scale_ );
		}

		if ( aud )
		{
			// Now block until we need to push more frame into the queue
			boost::mutex::scoped_lock lck( mutex_audio_ );

			if ( !started_ )
			{
				device_->BeginAudioPreroll( );
				started_ = true;
			}

			while( downloaded_audio_.size() >= prop_preroll_.value< int >( ) )
				cond_audio_.wait( lck );
			
			downloaded_audio_.push_back( std::make_pair( frames_, aud ) );
		}

		frames_ ++;

		return true;
	}
	
	virtual void complete( )
	{
	}
	
	virtual ml::frame_type_ptr flush( )
	{
		return ml::frame_type_ptr();
	}
	
	virtual HRESULT ScheduledFrameCompleted( IDeckLinkVideoFrame *completedFrame, BMDOutputFrameCompletionResult result )
	{
		boost::mutex::scoped_lock lck( mutex_image_ );
		downloaded_image_.pop_front();
		cond_image_.notify_all( );
		return S_OK;
	}
	
	virtual HRESULT ScheduledPlaybackHasStopped( void )
	{
		return S_OK;
	}
	
	virtual HRESULT RenderAudioSamples( bool preroll )
	{
		boost::mutex::scoped_lock lck( mutex_audio_ );

		// Try to maintain the number of audio samples buffered in the API at a specified waterlevel
		boost::uint32_t buffered;
		if ( device_->GetBufferedAudioSampleFrameCount( &buffered ) == S_OK && buffered < 48000 && downloaded_audio_.size( ) )
		{
			ml::audio_type_ptr audio = downloaded_audio_.front( ).second;
			boost::uint32_t samples;
			device_->ScheduleAudioSamples( audio->pointer( ), audio->samples( ), 0, 0, &samples );
			if ( samples != audio->samples( ) )
				std::cerr << "offered " << audio->samples( ) << " took " << samples << std::endl;
			downloaded_audio_.pop_front( );
			cond_audio_.notify_all( );
		}

		if ( preroll )
		{
			device_->StartScheduledPlayback( 0, 100, 1.0 );
		}

		return S_OK;
	}
	
	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, LPVOID *ppv)
	{
		return S_FALSE;
	}
	
	virtual ULONG STDMETHODCALLTYPE AddRef(void)
	{
		return 1;
	}
	
	virtual ULONG STDMETHODCALLTYPE Release(void)
	{
		return 0;
	}
};


ml::store_type_ptr ML_PLUGIN_DECLSPEC create_store_decklink( const std::wstring& resource, const ml::frame_type_ptr& frame )
{
	ml::store_type_ptr result( new store_decklink( resource, frame ) );
	return result;
}
	
} }
