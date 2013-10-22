// VML Decklink Media Plugin
//
// Copyright (C) 2012 VizRT
//
// #input:decklink:
//
// Provides an input for the decklink blackmagic cards.
//
// Accepts the following properties:
//
// pf        Picture format               "uyv422"
// af        Audio format                 "pcm16" or "pcm32"
// mode      Display mode                 -1 (auto), or 0 to N for blackmagic index
// frequency Audio frequency              48000
// channels  Number of channels           2, 8 or 16
// timeout   Time to wait on fetch in ms  100
// queue     Number of frames to cache    200
// card      The card number required     0

#include <opencorelib/cl/lru.hpp>
#include <opencorelib/cl/core.hpp>
#include <opencorelib/cl/log_defines.hpp>
#include <opencorelib/cl/assert_defines.hpp>
#include <opencorelib/cl/enforce_defines.hpp>

#include <openmedialib/ml/input.hpp>
#include <openmedialib/ml/keys.hpp>
#include <openmedialib/ml/audio_utilities.hpp>

#include <DeckLinkAPI.h>

namespace cl = olib::opencorelib;
namespace ml = olib::openmedialib::ml;
namespace pl = olib::openpluginlib;

namespace amf { namespace openmedialib {

#ifndef WIN32
#define BD_HOME "HOME"
#else
#define BD_HOME "USERPROFILE"
#endif

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
		, id_( ml::audio::pcm16_id )
		, frequency_( 0 )
		, channels_( 0 )
		, samples_( 0 )
		{
		}

		void reset( int queue )
		{
			lru_.resize( queue );
			frame_count_ = 0;
			lru_.clear( );
			last_image_ = ml::image_type_ptr( );
			samples_ = 0;
		}

		void set_fps( int fps_num, int fps_den )
		{
			fps_num_ = fps_num;
			fps_den_ = fps_den;
		}

		void set_audio( ml::audio::identity id, int frequency, int channels )
		{
			id_ = id;
			frequency_ = frequency;
			channels_ = channels;
		}

		void set_image( const olib::t_string pf, int width, int height )
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

			// We need to keep a running total of audio samples to allow us to generate silence for missing ones
			int samples = 0;

			// Handle Video Frame
			if( picture )
			{
				ml::image_type_ptr image;
				if ( picture->GetFlags() & bmdFrameHasNoInputSource )
				{
					// Log failure for all frames except 0 to avoid errors during auto detect
					if ( frame_count_ != 0 )
					{
						ARLOG( "No video frame received for %d with %dx%d @ %d:%d fps" )( frame_count_ )( width_ )( height_ )( fps_num_ )( fps_den_ ).level( cl::log_level::error );
						image = last_image_;
					}
				}
				else
				{
					// TODO: sort out field order, sar and deal with non-matching pitch on the destination image
					void *bytes;
					picture->GetBytes( &bytes );
					image = ml::image::allocate( pf_, picture->GetWidth( ), picture->GetHeight( ) );
					image->set_position( frame_count_ );
					memcpy( image->ptr( ), bytes, image->size( ) );
				}
				frame->set_image( image );
				last_image_ = image;
			}

			// Handle Audio Frame
			if ( sound )
			{
				void *bytes;
				samples = sound->GetSampleFrameCount( );
				sound->GetBytes( &bytes );
				ml::audio_type_ptr audio = ml::audio::allocate( id_ , frequency_, channels_, samples, false );
				memcpy( audio->pointer( ), bytes, audio->size( ) );
				audio->set_position( frame_count_ );
				frame->set_audio( audio );
			}
			else
			{
				samples = ml::audio::samples_to_frame( frame_count_ + 1, frequency_, fps_num_, fps_den_ ) - samples_;
				ARLOG( "No audio frame received for %d with %d hz %d channels @ %d:%d fps generating %d" )( frame_count_ )( frequency_ )( channels_ )( fps_num_ )( fps_den_ )( samples ).level( cl::log_level::error );
				ml::audio_type_ptr audio = ml::audio::allocate( id_, frequency_, channels_, samples, true );
				audio->set_position( frame_count_ );
				frame->set_audio( audio );
			}

			// Append to lru
			if( lru_.append_if_not_full( frame_count_, frame, boost::posix_time::milliseconds( 0 ) ) )
			{
				// Increment the frame count on success - failure will result in the following frame created having the same position
				frame_count_ ++;
				samples_ += samples;
			}
			else
			{
				ARLOG( "No room in queue - discarding frame %d" )( frame_count_ ).level( cl::log_level::error );
			}

			return S_OK;
		}

		ml::frame_type_ptr wait( int index, boost::posix_time::time_duration time )
		{
			ml::frame_type_ptr frame = lru_.wait( index, time );
			if ( frame )
				lru_.remove( index );
			return frame;
		}

	private:
		ml::lru_frame_type lru_;
		ml::image_type_ptr last_image_;
		ULONG ref_count_;
		int frame_count_;
		int fps_num_;
		int fps_den_;
		olib::t_string pf_;
		int width_;
		int height_;
		ml::audio::identity id_;
		int frequency_;
		int channels_;
		boost::int64_t samples_;
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
		, prop_timeout_( pl::pcos::key::from_string( "timeout" ) )
		, prop_queue_( pl::pcos::key::from_string( "queue" ) )
		, prop_card_( pl::pcos::key::from_string( "card" ) )
		, decklink_( 0 )
		, decklink_input_( 0 )
		, decklink_display_mode_iter_( 0 )
		, decklink_iter_( 0 )
		, decklink_delegate_( 0 )
		{
			properties( ).append( prop_pf_ = olib::t_string( _CT("uyv422") ) );
			properties( ).append( prop_af_ = std::wstring( L"pcm16" ) );
			properties( ).append( prop_mode_ = -1 );
			properties( ).append( prop_frequency_ = 48000 );
			properties( ).append( prop_channels_ = 2 );
			properties( ).append( prop_timeout_ = 100 );
			properties( ).append( prop_queue_ = 200 );
			properties( ).append( prop_card_ = 0 );
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
#			ifndef _WIN32
			decklink_iter_ = CreateDeckLinkIteratorInstance( );
#			else
			CoCreateInstance(CLSID_CDeckLinkIterator, NULL, CLSCTX_ALL, IID_IDeckLinkIterator, (void**)&decklink_iter_ );
#			endif

			// Check if the drivers exist
			HRESULT error = decklink_iter_ == 0 ? S_FALSE : S_OK;
			ARLOG_IF( error != S_OK, "This input requires the DeckLink drivers installed." ).level( cl::log_level::error );

			// Connect to the card required if it exists
			if ( error == S_OK )
			{
				// Iterate through each input until we find the card requested by the card property
				for ( int card = 0; ( error = decklink_iter_->Next( &decklink_ ) ) == S_OK; )
				{
					// Check that this one is an input
					if ( decklink_->QueryInterface( IID_IDeckLinkInput, ( void ** )&decklink_input_ ) == S_OK )
					{
						// If the current card matches the request, then break
						if ( card ++ == prop_card_.value< int >( ) )
							break;
					}
				}

				// Log failure if necessary
				ARLOG_IF( error != S_OK, "No DeckLink PCI card found for %d." )( prop_card_.value< int >( ) ).level( cl::log_level::error );
			}

			// Obtain an IDeckLinkDisplayModeIterator to enumerate the display modes supported on output
			if ( error == S_OK )
			{
				error = decklink_input_->GetDisplayModeIterator( &decklink_display_mode_iter_ );
				ARLOG_IF( error != S_OK, "Could not obtain the video input display mode iterator - result = %08x" )( error ).level( cl::log_level::error );
			}

			// Check that the picture format is valid
			if ( error == S_OK && prop_pf_.value< olib::t_string >( ) != "uyv422" )
			{
				error = S_FALSE;
				ARLOG( "The AML decklink plugin currently only supports a picture format of uyv422." ).level( cl::log_level::error );
			}

			// Attempt to find a valid mode
			if ( error == S_OK )
			{
				int requested = prop_mode_.value< int >( );

				// Construct delegate now
				decklink_delegate_ = new DeckLinkCaptureDelegate( );
				decklink_input_->SetCallback( decklink_delegate_ );

				// Holds all the valid modes
				std::vector< IDeckLinkDisplayMode * > modes;

				// Collect all the modes
				IDeckLinkDisplayMode *display;
				while ( decklink_display_mode_iter_->Next( &display ) == S_OK )
					modes.push_back( display );

				// Load the mode order file for this card
				std::vector< size_t > preferences = load_preferences( prop_card_.value< int >( ), modes );

				// Find the requested mode or check each in turn
				for ( size_t index = 0; !found && index < preferences.size( ); index ++ )
				{
					size_t mode = preferences[ index ];
					if ( int( mode ) == requested || requested < 0 )
						if ( select( modes[ mode ], mode ) )
							found = move_preferences( preferences, mode );
				}

				// If we have a found item, then save the file now
				if ( found )
					save_preferences( prop_card_.value< int >( ), preferences );

				// Clean up the modes
				for ( std::vector< IDeckLinkDisplayMode * >::iterator iter = modes.begin( ); iter != modes.end( ); iter ++ )
					( *iter )->Release( );
			}

			return found;
		}

		// Provides the location of the mode order preferences file
		std::string file_preferences( int card )
		{
			return std::string( getenv( BD_HOME ) ) + "/.aml.decklink." + boost::lexical_cast< std::string >( card );
		}

		// Load the mode order for this card, or generate the default order
		std::vector< size_t > load_preferences( int card, std::vector< IDeckLinkDisplayMode * > &modes )
		{
			// Will hold the result
			std::vector< size_t > result;

			// Get the file name for this card
			std::string filename = file_preferences( card );

			// Open the file
			std::fstream file( filename.c_str( ), std::fstream::in );

			// Parse the file - just contains a list of numbers separated by spaces
			if ( file.is_open( ) )
			{
				size_t temp = 0;
				file >> temp;

				while( file.good( ) )
				{
					result.push_back( temp );
					file >> temp;
				}

				file.close( );
			}

			// If the file doesn't exist or has the wrong number of entries, create the default lookup order
			if ( result.size( ) != modes.size( ) )
			{
				ARLOG_INFO( "Loaded preferences had %1% entries which doesn't match the mode count of %2% - defaulting to searching all" )( result.size( ) )( modes.size( ) );
				result.clear( );
				for ( size_t index = 0; index < modes.size( ); index ++ )
					result.push_back( index );
			}

			return result;
		}

		// Move the mode specified to the start of the vector
		bool move_preferences( std::vector< size_t > &preferences, size_t mode )
		{
			if ( preferences[ 0 ] != mode )
			{
				preferences.insert( preferences.begin( ), mode );
				for ( std::vector< size_t >::iterator iter = ++ preferences.begin( ); iter != preferences.end( ); iter ++ )
					if ( *iter == mode )
						iter = preferences.erase( iter );
			}

			return true;
		}

		// Save the file
		void save_preferences( int card, std::vector< size_t > &preferences )
		{
			std::string filename = file_preferences( card );
			std::fstream file( filename.c_str( ), std::fstream::out );
			if ( file.is_open( ) )
			{
				for ( std::vector< size_t >::iterator iter = preferences.begin( ); iter != preferences.end( ); iter ++ )
					file << *iter << " ";
				file << std::endl;
				file.close( );
			}
			else
			{
				ARLOG_ERR( "Unable to open %1% for writing - decklink selection not saved" )( filename );
			}
		}

		// Attempt to use the selected mode
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
				decklink_delegate_->reset( prop_queue_.value< int >( ) );
				decklink_delegate_->set_fps( fps_num, fps_den );
				decklink_delegate_->set_image( prop_pf_.value< olib::t_string >( ), width, height );
				decklink_delegate_->set_audio( ml::audio::af_to_id( olib::opencorelib::str_util::to_t_string( prop_af_.value< std::wstring >( ) ) ), prop_frequency_.value< int >( ), prop_channels_.value< int >( ) );
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
				error = decklink_input_->EnableAudioInput( bmdAudioSampleRate48kHz, BMDAudioSampleType( 16 ), prop_channels_.value< int >( ) );
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
				last_frame_ = frame;
				error = frame && frame->get_image( ) ? S_OK : S_FALSE;
				if ( error != S_OK )
					decklink_input_->StopStreams( );
			}

			return error == S_OK;
		}

		// Fetch method
		void do_fetch( ml::frame_type_ptr &result )
		{
			// Ensure that we have a delegate
			ARENFORCE_MSG( decklink_delegate_, "Delegate has not been constructed" );

			// Check if we're repeating the last frame or a new open is requested
			if ( !last_frame_ || last_frame_->get_position( ) != get_position( ) )
				result = decklink_delegate_->wait( get_position( ), boost::posix_time::milliseconds( prop_timeout_.value< int >( ) ) );
			else
				result = last_frame_;

			// Check that we have a frame
			ARENFORCE_MSG( result, "No frame obtained for %d" )( get_position( ) );

			// Save as last frame in case of a repeat request and shallow copy
			last_frame_ = result;
			result = result->shallow( );
		}

		// Shut everything down
		void stop( )
		{
			if ( decklink_input_ )
				decklink_input_->StopStreams( );

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
		pl::pcos::property prop_timeout_;
		pl::pcos::property prop_queue_;
		pl::pcos::property prop_card_;

		IDeckLink *decklink_;
		IDeckLinkInput *decklink_input_;
		IDeckLinkDisplayModeIterator *decklink_display_mode_iter_;

		IDeckLinkIterator *decklink_iter_;
		DeckLinkCaptureDelegate *decklink_delegate_;

		ml::frame_type_ptr last_frame_;
};

ml::input_type_ptr ML_PLUGIN_DECLSPEC create_input_decklink( const std::wstring &filename )
{
	return ml::input_type_ptr( new input_decklink( filename ) );
}

} }
