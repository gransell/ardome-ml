// VML Decklink Media Plugin
//
// Copyright (C) 2012 VizRT

#include <opencorelib/cl/lru.hpp>
#include <opencorelib/cl/core.hpp>
#include <opencorelib/cl/log_defines.hpp>
#include <opencorelib/cl/assert_defines.hpp>
#include <opencorelib/cl/enforce_defines.hpp>

#include <openmedialib/ml/input.hpp>
#include <openmedialib/ml/keys.hpp>

#include <DeckLinkAPI.h>

namespace cl = olib::opencorelib;
namespace il = olib::openimagelib::il;
namespace ml = olib::openmedialib::ml;
namespace pl = olib::openpluginlib;

namespace amf { namespace openmedialib {

class DeckLinkCaptureDelegate : public IDeckLinkInputCallback
{
	public:
		DeckLinkCaptureDelegate( )
		: ref_count_( 0 )
		, frame_count_( 0 )
		, fps_num_( 0 )
		, fps_den_( 0 )
		, width_( 0 )
		, height_( 0 )
		, frequency_( 0 )
		, channels_( 0 )
		{
		}

		void reset( )
		{
			frame_count_ = 0;
			lru_.clear( );
			last_image_ = il::image_type_ptr( );
		}

		void set_fps( int fps_num, int fps_den )
		{
			fps_num_ = fps_num;
			fps_den_ = fps_den;
		}

		void set_audio( const std::wstring af, int frequency, int channels )
		{
			af_ = af;
			frequency_ = frequency;
			channels_ = channels;
		}

		void set_image( const std::wstring pf, int width, int height )
		{
			pf_ = pf;
			width_ = width;
			height_ = height;
		}

		virtual HRESULT STDMETHODCALLTYPE QueryInterface( REFIID iid, LPVOID *ppv ) 
		{ 
			return E_NOINTERFACE; 
		}

		virtual ULONG STDMETHODCALLTYPE AddRef( void )
		{
			return 1;
		}

		virtual ULONG STDMETHODCALLTYPE Release( void )
		{
			return 0;
		}

		virtual HRESULT STDMETHODCALLTYPE VideoInputFormatChanged( BMDVideoInputFormatChangedEvents events, IDeckLinkDisplayMode *mode, BMDDetectedVideoInputFormatFlags flags )
		{
			fprintf( stderr, "mode changed\n" );
			return S_OK;
		}

		virtual HRESULT STDMETHODCALLTYPE VideoInputFrameArrived( IDeckLinkVideoInputFrame *picture, IDeckLinkAudioInputPacket *sound )
		{
			// Generate frame
			ml::frame_type_ptr frame = ml::frame_type_ptr( new ml::frame_type( ) );
			frame->set_position( frame_count_ );
			frame->set_fps( fps_num_, fps_den_ );
			frame->set_duration( 1.0 / ( double( fps_num_ ) / fps_den_ ) );
			frame->set_sar( 1, 1 );
			pl::pcos::assign< int >( frame->properties( ), ml::keys::source_timecode, frame_count_ );

			// Handle Video Frame
			if( picture )
			{
				il::image_type_ptr image;
				if ( picture->GetFlags() & bmdFrameHasNoInputSource )
				{
					ARLOG( "No video frame received for %d with %dx%d @ %d:%d fps" )( frame_count_ )( width_ )( height_ )( fps_num_ )( fps_den_ ).level( cl::log_level::error );
					image = last_image_;
				}
				else
				{
					// TODO: sort out field order, sar and deal with non-matching pitch on the destination image
					void *bytes;
					picture->GetBytes( &bytes );
					image = il::allocate( pf_, picture->GetWidth( ), picture->GetHeight( ) );
					image->set_position( frame_count_ );
					boost::uint8_t *dst = ( boost::uint8_t * )image->data( );
					memcpy( dst, bytes, image->size( ) );
				}
				frame->set_image( image );
				last_image_ = image;
			}

			// Handle Audio Frame
			if ( sound )
			{
				ml::audio_type_ptr audio;
				void *bytes;
				sound->GetBytes( &bytes );
				audio = ml::audio::allocate( std::wstring( af_ ), frequency_, channels_, sound->GetSampleFrameCount( ), false );
				memcpy( audio->pointer( ), bytes, audio->size( ) );
				audio->set_position( frame_count_ );
				frame->set_audio( audio );
			}
			else
			{
				// TODO: Generate missing audio samples if they ever occur?
				ARLOG( "No audio frame received for %d with %d hz %d channels @ %d:%d fps" )( frame_count_ )( frequency_ )( channels_ )( fps_num_ )( fps_den_ ).level( cl::log_level::error );
			}

			// Append to lru
			lru_.append( frame_count_, frame );

			// Increment the frame count
			frame_count_ ++;

			return S_OK;
		}

		ml::frame_type_ptr wait( int index, boost::posix_time::time_duration time )
		{
			return lru_.wait( index, time );
		}

	private:
		ml::lru_frame_type lru_;
		il::image_type_ptr last_image_;
		ULONG ref_count_;
		int frame_count_;
		int fps_num_;
		int fps_den_;
		std::wstring pf_;
		int width_;
		int height_;
		std::wstring af_;
		int frequency_;
		int channels_;
};

class ML_PLUGIN_DECLSPEC input_decklink : public ml::input_type
{
	public:
		// Constructor and destructor
		input_decklink( const std::wstring &filename ) 
		: uri_( filename )
		, prop_mode_( pl::pcos::key::from_string( "mode" ) )
		, prop_pf_( pl::pcos::key::from_string( "pf" ) )
		, prop_af_( pl::pcos::key::from_string( "af" ) )
		, prop_frequency_( pl::pcos::key::from_string( "frequency" ) )
		, prop_channels_( pl::pcos::key::from_string( "channels" ) )
		, decklink_( 0 )
		, decklink_input_( 0 )
		, decklink_display_mode_iter_( 0 )
		, decklink_iter_( 0 )
		, decklink_delegate_( 0 )
		{
			properties( ).append( prop_pf_ = std::wstring( L"uyv422" ) );
			properties( ).append( prop_af_ = std::wstring( L"pcm16" ) );
			properties( ).append( prop_mode_ = -1 );
			properties( ).append( prop_frequency_ = 48000 );
			properties( ).append( prop_channels_ = 2 );
		}
		
		virtual ~input_decklink( ) 
		{
			stop( );
		}
		
		// Indicates if the input will enforce a packet decode
		virtual bool requires_image( ) const { return false; }
		
		// Basic information
		virtual const std::wstring get_uri( ) const 
		{
			return uri_; 
		}

		virtual const std::wstring get_mime_type( ) const 
		{
			return L""; 
		}
		
		// Audio/Visual
		virtual int get_frames( ) const
		{
			return std::numeric_limits< int >::max( );
		}

		// Indicates this input is not seekable
		virtual bool is_seekable( ) const
		{
			return false;
		}
		
		// Visual
		virtual int get_video_streams( ) const
		{
			return 1;
		}
		
		// Audio
		virtual int get_audio_streams( ) const
		{
			return 1;
		}
		
	protected:
		virtual bool initialize( )
		{
			// TODO: Check sanity of audio paramaters and add suport for pcm32 and aes(?)

			// Provides the return value
			bool found = false;

			// Create the decklink iterator
			decklink_iter_ = CreateDeckLinkIteratorInstance( );

			// Check if the drivers exist
			HRESULT error = decklink_iter_ == 0 ? S_FALSE : S_OK;
			ARLOG_IF( error != S_OK, "This input requires the DeckLink drivers installed." ).level( cl::log_level::error );

			// Connect to the first deck instance
			if ( error == S_OK )
			{
				error = decklink_iter_->Next( &decklink_ );
				ARLOG_IF( error != S_OK, "No DeckLink PCI cards found." ).level( cl::log_level::error );
			}

			// Check that we have have the correct interface
			if ( error == S_OK )
			{
				error = decklink_->QueryInterface( IID_IDeckLinkInput, ( void ** )&decklink_input_ );
				ARLOG_IF( error != S_OK, "Query interface of input failed - wrong version of drivers?" ).level( cl::log_level::error );
			}

			// Obtain an IDeckLinkDisplayModeIterator to enumerate the display modes supported on output
			if ( error == S_OK )
			{
				error = decklink_input_->GetDisplayModeIterator( &decklink_display_mode_iter_ );
				ARLOG_IF( error != S_OK, "Could not obtain the video input display mode iterator - result = %08x" )( error ).level( cl::log_level::error );
			}

			// Check that the picture format is valid
			if ( error == S_OK && prop_pf_.value< std::wstring >( ) != L"uyv422" )
			{
				error = S_FALSE;
				ARLOG( "The AML decklink plugin currently only supports a picture format of uyv422." ).level( cl::log_level::error );
			}

			// Attempt to find a valid mode
			if ( error == S_OK )
			{
				IDeckLinkDisplayMode *display;
				int mode = 0;
				int requested = prop_mode_.value< int >( );

				decklink_delegate_ = new DeckLinkCaptureDelegate( );
				decklink_input_->SetCallback( decklink_delegate_ );

				while ( error == S_OK && !found && decklink_display_mode_iter_->Next( &display ) == S_OK )
				{
					// If this is the requested one or we're in auto mode, attempt to start the input
					if ( mode == requested || requested < 0 )
						found = select( display, mode );

					// Release the IDeckLinkDisplayMode object to prevent a leak
					display->Release( );

					// Increment the mode
					mode ++;
				}
			}

			return found;
		}

		bool select( IDeckLinkDisplayMode *display, int mode )
		{
			// Extract information for the delegate
			BMDTimeValue frameRateDuration, frameRateScale;
			display->GetFrameRate( &frameRateDuration, &frameRateScale );
			int fps_num = int( frameRateScale );
			int fps_den = int( frameRateDuration );
			int width = int( display->GetWidth( ) );
			int height = int( display->GetHeight( ) );
			BMDDisplayMode selectedDisplayMode = display->GetDisplayMode( );
			BMDPixelFormat pixelFormat = bmdFormat8BitYUV;
			BMDVideoInputFlags inputFlags = 0;

			// Indicate an error if unsupported
			BMDDisplayModeSupport support;
			decklink_input_->DoesSupportVideoMode( selectedDisplayMode, pixelFormat, bmdVideoInputFlagDefault, &support, NULL );
			HRESULT error = support == bmdDisplayModeNotSupported ? S_FALSE : S_OK;
			ARLOG_IF( error != S_OK, "The requested display mode %d is not supported with the selected pixel format" )( mode ).level( cl::log_level::error );

			// Sync up this display mode with the delegate
			if ( error == S_OK )
			{
				decklink_delegate_->reset( );
				decklink_delegate_->set_fps( fps_num, fps_den );
				decklink_delegate_->set_image( prop_pf_.value< std::wstring >( ), width, height );
				decklink_delegate_->set_audio( prop_af_.value< std::wstring >( ), prop_frequency_.value< int >( ), prop_channels_.value< int >( ) );
			}

			// Attempt to enable the video feed
			if ( error == S_OK )
			{
				error = decklink_input_->EnableVideoInput( selectedDisplayMode, pixelFormat, inputFlags );
				ARLOG_IF( error != S_OK, "Failed to enable video input. Is another application using the card?" ).level( cl::log_level::error );
			}

			// Attempt to enable the audio feed
			if ( error == S_OK )
			{
				error = decklink_input_->EnableAudioInput( bmdAudioSampleRate48kHz, 16, prop_channels_.value< int >( ) );
				ARLOG_IF( error != S_OK, "Failed to enable audio input." ).level( cl::log_level::error );
			}

			// Start the feed
			if ( error == S_OK )
			{
				error = decklink_input_->StartStreams( );
				ARLOG_IF( error != S_OK, "Failed to start the streams." ).level( cl::log_level::error );
			}

			// Check that the mode is valid
			if ( error == S_OK )
			{
				ml::frame_type_ptr frame = decklink_delegate_->wait( 0, boost::posix_time::milliseconds( 500 ) );
				error = frame && frame->get_image( ) ? S_OK : S_FALSE;
				if ( error != S_OK )
					decklink_input_->StopStreams( );
			}

			return error == S_OK;
		}

		// Fetch method
		void do_fetch( ml::frame_type_ptr &result )
		{
			ARENFORCE_MSG( decklink_delegate_, "Delegate has not been constructed" );
			result = decklink_delegate_->wait( get_position( ), boost::posix_time::seconds( 1 ) );
			if ( result ) result = result->shallow( );
		}

		// Shut everything down
		void stop( )
		{
			if ( decklink_display_mode_iter_ != 0 )
				decklink_display_mode_iter_->Release( );

			if ( decklink_input_ != NULL)
				 decklink_input_->Release( );

			if ( decklink_ != 0 )
				decklink_->Release( );

			if ( decklink_iter_ != 0 )
				decklink_iter_->Release( );

			delete decklink_delegate_;
		}

	private:
		const std::wstring uri_;

		pl::pcos::property prop_mode_;
		pl::pcos::property prop_pf_;
		pl::pcos::property prop_af_;
		pl::pcos::property prop_frequency_;
		pl::pcos::property prop_channels_;

		IDeckLink *decklink_;
		IDeckLinkInput *decklink_input_;
		IDeckLinkDisplayModeIterator *decklink_display_mode_iter_;

		IDeckLinkIterator *decklink_iter_;
		DeckLinkCaptureDelegate *decklink_delegate_;
};

ml::input_type_ptr ML_PLUGIN_DECLSPEC create_input_decklink( const std::wstring &filename )
{
	return ml::input_type_ptr( new input_decklink( filename ) );
}

} }
