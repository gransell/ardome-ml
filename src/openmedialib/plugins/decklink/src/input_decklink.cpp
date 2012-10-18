// AMF Decklink Media Plugin
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

		virtual ~DeckLinkCaptureDelegate( )
		{
		}

		void set_fps( int fps_num, int fps_den )
		{
			fps_num_ = fps_num;
			fps_den_ = fps_den;
		}

		void set_audio( const pl::wstring af, int frequency, int channels )
		{
			af_ = af;
			frequency_ = frequency;
			channels_ = channels;
		}

		void set_image( const pl::wstring pf, int width, int height )
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
			{
				boost::recursive_mutex::scoped_lock lock( mutex_ );
				ref_count_ ++;
			}
			return ref_count_;
		}

		virtual ULONG STDMETHODCALLTYPE Release( void )
		{
			{
				boost::recursive_mutex::scoped_lock lock( mutex_ );
				ref_count_ --;
			}

			if (ref_count_ == 0)
			{
				delete this;
				return 0;
			}

			return ref_count_;
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
					fprintf(stderr, "Frame received (#%d) - No input signal detected\n", frame_count_ );
					// TODO: Generate test card to image_type_ptr
				}
				else
				{
					void *bytes;
					picture->GetBytes( &bytes );
					image = il::allocate( pf_, picture->GetWidth( ), picture->GetHeight( ) );
					image->set_position( frame_count_ );
					// TODO: Transfer bytes to image_type_ptr 
					boost::uint8_t *dst = ( boost::uint8_t * )image->data( );
					boost::uint8_t *src = ( boost::uint8_t * )bytes;
					boost::uint8_t *end = ( boost::uint8_t * )image->data( ) + image->size( );
					while ( dst < end )
					{
						*dst ++ = src[ 1 ];
						*dst ++ = src[ 0 ];
						*dst ++ = src[ 3 ];
						*dst ++ = src[ 2 ];
						src += 4;
					}
					// TODO: sort out field order
				}
				frame->set_image( image );
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
		mutable boost::recursive_mutex mutex_;
		ml::lru_frame_type lru_;
		ULONG ref_count_;
		int frame_count_;
		int fps_num_;
		int fps_den_;
		pl::wstring pf_;
		int width_;
		int height_;
		pl::wstring af_;
		int frequency_;
		int channels_;
};


class ML_PLUGIN_DECLSPEC input_decklink : public ml::input_type
{
	public:
		// Constructor and destructor
		input_decklink( const pl::wstring &filename ) 
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
			properties( ).append( prop_pf_ = pl::wstring( L"yuv422" ) );
			properties( ).append( prop_af_ = pl::wstring( L"pcm16" ) );
			properties( ).append( prop_mode_ = 8 );
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
		virtual const pl::wstring get_uri( ) const { return uri_; }
		virtual const pl::wstring get_mime_type( ) const { return L""; }
		
		// Audio/Visual
		virtual int get_frames( ) const
		{
			return std::numeric_limits< int >::max();
		}

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
			HRESULT error = S_OK;

			int fps_num = 0;
			int fps_den = 0;
			int width = 0;
			int height = 0;

			BMDDisplayMode selectedDisplayMode = bmdModeNTSC;
			BMDPixelFormat pixelFormat = bmdFormat8BitYUV;
			BMDVideoInputFlags inputFlags = 0;

			// Create the decklink iterator
			decklink_iter_ = CreateDeckLinkIteratorInstance( );
			if ( decklink_iter_ == 0 )
			{
				error = !S_OK;
				ARLOG( "This input requires the DeckLink drivers installed." ).level( cl::log_level::error );
			}

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
			if ( error == S_OK && prop_pf_.value< pl::wstring >( ) != L"yuv422" )
			{
				error = !S_OK;
				ARLOG( "AML currently only supports a picture format of yuv422." ).level( cl::log_level::error );
			}

			// Handle the requested display mode
			if ( error == S_OK )
			{
				IDeckLinkDisplayMode *display;
				int mode = 0;
				bool done = false;

				while ( error == S_OK && decklink_display_mode_iter_->Next( &display ) == S_OK )
				{
					BMDTimeValue frameRateDuration, frameRateScale;
					display->GetFrameRate(&frameRateDuration, &frameRateScale);
				
					fprintf(stderr, "%2d:  \t %li x %li \t %d:%d FPS\n",
						mode, display->GetWidth(), display->GetHeight(), ( int )frameRateScale, ( int )frameRateDuration);

					// Handle this one if it's the selected mode
					if ( mode == prop_mode_.value< int >( ) )
					{
						fps_num = int( frameRateScale );
						fps_den = int( frameRateDuration );
						width = int( display->GetWidth( ) );
						height = int( display->GetHeight( ) );
						selectedDisplayMode = display->GetDisplayMode( );

						// Indicate an error is unsupported
						BMDDisplayModeSupport support;
						decklink_input_->DoesSupportVideoMode( selectedDisplayMode, pixelFormat, bmdVideoInputFlagDefault, &support, NULL );
						if ( support == bmdDisplayModeNotSupported )
						{
							error = !S_OK;
							fprintf( stderr, "The requested display mode is not supported with the selected pixel format\n" );
						}
						else
						{
							done = true;
						}
					}

					// Increment the mode
					mode ++;

					// Release the IDeckLinkDisplayMode object to prevent a leak
					display->Release();
				}

				// If the mode was invalid, indicate that now
				if ( !done )
					error = !S_OK;
			}

			// Register the callback object
			if ( error == S_OK )
			{
				decklink_delegate_ = new DeckLinkCaptureDelegate( );
				decklink_delegate_->set_fps( fps_num, fps_den );
				decklink_delegate_->set_image( prop_pf_.value< pl::wstring >( ), width, height );
				decklink_delegate_->set_audio( prop_af_.value< pl::wstring >( ), prop_frequency_.value< int >( ), prop_channels_.value< int >( ) );
				decklink_input_->SetCallback( decklink_delegate_ );
			}

			if ( error == S_OK )
			{
				error = decklink_input_->EnableVideoInput( selectedDisplayMode, pixelFormat, inputFlags );
				if( error != S_OK )
					fprintf(stderr, "Failed to enable video input. Is another application using the card?\n");
			}

			if ( error == S_OK )
			{
				error = decklink_input_->EnableAudioInput( bmdAudioSampleRate48kHz, 16, 2 );
			}

			if ( error == S_OK )
			{
				error = decklink_input_->StartStreams( );
			}
			
			return error == S_OK;
		}

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
		}
		
		// Fetch method
		void do_fetch( ml::frame_type_ptr &result )
		{
			ARENFORCE_MSG( decklink_delegate_, "Delegate has not been constructed" );
			result = decklink_delegate_->wait( get_position( ), boost::posix_time::seconds( 1 ) );
			if ( result ) result = result->shallow( );
		}

	private:
		const pl::wstring uri_;

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
	
	
ml::input_type_ptr ML_PLUGIN_DECLSPEC create_input_decklink( const pl::wstring &filename )
{
	return ml::input_type_ptr( new input_decklink( filename ) );
}
	
} }



