
// quicktime - A Quicktime plugin to ml.

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

// OL INCLUDES
#include <openmedialib/ml/openmedialib_plugin.hpp>					// images, audio etc

// LOCAL INCLUDES
#include <openmedialib/plugins/quicktime/quicktime_input.h>			// header


namespace opl = olib::openpluginlib;
namespace il = olib::openimagelib::il;
namespace ml = olib::openmedialib::ml;

using namespace std;
using namespace ml;

//---------------------------LIFECYCLE-----------------------------------------------------------------------

// Constructor and destructor
quicktime_input::quicktime_input( opl::wstring resource, const opl::wstring mime_type ) 
: input_type( ) 
, uri_( resource )
, mime_type_( mime_type )
, video_index_( -1 )
, audio_index_( -1 )
, movie_ref_num_( 0 )
, qt_movie_( NULL )
, valid_( false )
{
			
	OSErr	err;
	long	result;

	// check we have quicktime installed
	//	err = Gestalt( gestaltQuickTime, &result );
	//		if ( err != noErr )
		//		return;

	// initialize quick-time media layer
	#ifdef WIN32
		if(InitializeQTML(0)!=noErr)
			return;
	#endif

	// initialize the movie toolbox
	if (EnterMovies() != noErr)
		return;

	// create a movie spec from the file name
	#ifdef WIN32
		err = NativePathNameToFSSpec( const_cast<char*>( opl::to_string( resource ).c_str( ) ), &movie_file_spec_, kErrorIfFileNotFound );
	#else
		FSRef ref;
		err = FSPathMakeRef( ( const UInt8* ) opl::to_string( resource ).c_str( ), &ref, 0 );
		err = FSGetCatalogInfo( &ref, kFSCatInfoNone, NULL, NULL, &movie_file_spec_, NULL );
	#endif


	// make file spec
	//	Str255	file;
	//	c2pstrcpy(file, opl::to_string( resource ).c_str( ));
	//	FSMakeFSSpec(0,0,file,&movie_file_spec_);
	
	// attempt to open the media file
	err = OpenMovieFile ( &movie_file_spec_, &movie_ref_num_, fsRdPerm );
	if(err != noErr) 
	{
		cleanup();
		return;
	}
	
	// load the movie information
	err = NewMovieFromFile(&qt_movie_, movie_ref_num_, 0, 0, newMovieActive, 0);
	if(err != noErr) 
	{
		cleanup();
		return;
	}

	// Populate the input properties
	if ( err == noErr )
		populate( );

}

	quicktime_input::~quicktime_input( ) 
	{
		cleanup();

	}

//-----------------------------PUBLIC-----------------------------------------------------------------------

protected:
	void quicktime_input::fetch( frame_type_ptr &result )
	{
		// we want to get the frame for this position (frame number)
		int current = get_position();
		int wanted  = current;
		int process_flags = get_process_flags();

		// if we have video, check if this frame already cached
		if(		process_flags & process_image
			&&	has_video() )
		{
			bool image_cached = false;
			get_video_track()->set_wanted_frame( wanted );

			if(		images_.size()
				&&	images_.find(current)!= images_.end() )
			{
				image_cached = true;
			}

			while( !image_cached )
			{
				// fetch the image at the given track position
				il::image_type_ptr image = video_tracks_[video_index_]->decode( current );
				if(image)
				{
					if( images_.size() )
					{
						std::map < int, image_type_ptr>::iterator iter;
						int first = images_.begin()->first;
						iter = images_.end( );
						iter--;
						int last = iter->first;
						if ( current < last )
							images_.clear( );
						else if ( first < wanted - 15 )
							images_.erase( images_.begin( ) );
					}

					register int pos = image->position();
					images_[pos]= image;
					if( pos == wanted )
						image_cached = true;
					else
						current++;
				}
			}
		}

		// if we have audio, check if this frame already cached
		if(		process_flags & process_audio
			&&	has_audio() )
		{
			bool audio_cached = false;
			if( audios_.size() )
			{
				// look to see if audio is currently in cache
				int first = audios_[ 0 ]->position( );
				int last = audios_[ audios_.size( ) - 1 ]->position( );
				if ( current >= first && current <= last )
					audio_cached = true;
			}

			if( !audio_cached )
			{
				// fetch the audio at the given track position
				audio_type_ptr audio = audio_tracks_[audio_index_]->decode( wanted );
				if( audio )
				{
					if( audios_.size() )
					{
						int first = audios_[ 0 ]->position( );
						int last = audios_[ audios_.size( ) - 1 ]->position( );
						if ( wanted < last )
							audios_.clear( );
						else if ( first < wanted - 15 )
							audios_.erase( audios_.begin( ) );
					}

					audios_.push_back( audio );
					audio_cached = true;
				}
			}
		}
			
		// Create the output frame
		frame_type *frame = new frame_type( );
		result = frame_type_ptr( frame );

		int num, den;
		get_sar( num, den );
		frame->set_sar( num, den );
		frame->set_position( wanted );
		frame->set_pts( wanted * 1.0 / quicktime_input::fps( ) );
		frame->set_duration( 1.0 / quicktime_input::fps( ) );
		num=0, den=0;
		get_fps( num, den );
		frame->set_fps( num, den );
		if ( ( process_flags & process_image ) && has_video( ) )
			find_image( frame );
		if ( ( process_flags & process_audio ) && has_audio( ) )
			find_audio( frame );
	}



//-----------------------------ACCESSORS--------------------------------------------------------------------------

	// Audio/Visual
	int quicktime_input::get_frames( ) const 
	{ 
		if( has_video( ) )
			return get_video_track( )->get_frame_count( );
		if( has_audio( ) )
			return get_audio_track( )->get_frame_count( );
		return 0;
	}

	void quicktime_input::get_fps( int &num, int &den ) const 
	{ 
		num=0, den=0;	
		if( has_video( ) )
			get_video_track( )->get_fps( num, den );
		else if (has_audio( ) )
			get_audio_track( )->get_fps( num, den );
	}
				
	void quicktime_input::get_sar( int &num, int &den ) const 
	{ 
		if( has_video( ) )
			get_video_track( )->get_sar( num, den );
	}

		
	int quicktime_input::get_width( ) const 
	{ 
		if( has_video( ) ) 
			return get_video_track( )->get_width( );
		return 0;
	}

	int quicktime_input::get_height( ) const 
	{ 
		if( has_video( ) )
			return get_video_track( )->get_height( );
		return 0;
	}

	bool quicktime_input::is_valid( )
	{
		return valid_;
	}
//-----------------------------PRIVATE-----------------------------------------------------------------------------

	void quicktime_input::cleanup() 
	{
		if( movie_ref_num_ ) 
			CloseMovieFile( movie_ref_num_ );
		if( qt_movie_ )
			DisposeMovie( qt_movie_ );

		// delete video tracks
		for(int i=0; i < video_tracks_.size(); ++i)
		{
			video_tracks_[i]->finish_decode( );
			delete video_tracks_[i];
		}
		video_tracks_.clear();

		// delete audio tracks
		for(int i=0; i < audio_tracks_.size(); ++i)
			delete audio_tracks_[i];
		audio_tracks_.clear();

		ExitMovies();
	
	#ifdef WIN32	
		TerminateQTML();
	#endif
	}

	void quicktime_input::find_audio( frame_type *frame )
	{
		int current = get_position( );
		int closest = 1 << 16;
		std::deque< audio_type_ptr >::iterator result = audios_.end( );
		std::deque< audio_type_ptr >::iterator iter;

		for ( iter = audios_.begin( ); iter != audios_.end( ); iter ++ )
		{
			int diff = current - (*iter)->position( );
			if ( std::abs( diff ) <= closest )
			{
				result = iter;
				closest = std::abs( diff );
			}
			else if ( (*iter)->position( ) > current )
			{
				break;
			}
		}

		if ( result != audios_.end( ) )
		{
			frame->set_audio( *result );
			frame->set_duration( double( ( *result )->samples( ) ) / double( ( *result )->frequency( ) ) );
		}
		
	}


	void quicktime_input::find_image( frame_type *frame )
	{
		int current = get_position( );
		int closest = 1 << 16;
		std::map< int, image_type_ptr>::iterator result = images_.end( );
		std::map< int, image_type_ptr>::iterator iter;

		for ( iter = images_.begin( ); iter != images_.end( ); iter ++ )
		{
			int diff = current - iter->second->position( );
			if ( std::abs( diff ) <= closest )
			{
				result = iter;
				closest = std::abs( diff );
			}
			else if ( iter->second->position( ) > current )
			{
				break;
			}
		}

		if ( result != images_.end( ) )
		{
			cerr << result->second->position() << "\n";
			frame->set_image( result->second );
		}
	}

	
	// Analyse tracks and get input values
	void quicktime_input::populate( )
	{

		// get movie duration (units)
		movie_duration_ = GetMovieDuration(qt_movie_);

		// get time scale (units per second)
		movie_time_scale_ = GetMovieTimeScale(qt_movie_);

		// get track count
		int track_count = GetMovieTrackCount(qt_movie_);

		// Iterate through all the available tracks & initialize the video tracks
		for( int i = 1; i <= track_count; i++ ) 
		{
			// get media and track info
			Track qt_track = GetMovieIndTrack(qt_movie_, i);
			Media qt_media = GetTrackMedia(qt_track);
			OSType qt_media_type;
			GetMediaHandlerDescription(qt_media, &qt_media_type, nil, nil);

			// create video track
			if (qt_media_type == VideoMediaType)
			{
				if ( video_index_ < 0 )
				{
					quicktime_videotrack *vtrack = new quicktime_videotrack(i, qt_movie_, qt_track, qt_media);
					if( vtrack->open_for_decode( ) )
					{
						video_tracks_.push_back(vtrack);
						video_index_ = 0;
					}
					else 
						delete vtrack;
				}
			}

		}
		

		// Iterate through all the available tracks & initialize the audio tracks
		for( int i = 1; i <= track_count; i++ ) 
		{
			// get media and track info
			Track qt_track = GetMovieIndTrack( qt_movie_, i );
			Media qt_media = GetTrackMedia( qt_track );
			OSType qt_media_type;
			GetMediaHandlerDescription( qt_media, &qt_media_type, 0, 0 );

			// create sound track
			if(qt_media_type == SoundMediaType)
			{
				quicktime_audiotrack *atrack;
				if( audio_index_ < 0 )
				{
					// if we have video also, then use the video settings to pace the audio
					if( has_video ( ) )
					{
						atrack = new quicktime_audiotrack( i, qt_movie_, qt_track, qt_media,
														   get_video_track( )->get_frame_track_list() );
						int num; int den;
						get_video_track( )->get_fps( num, den );
						atrack->set_fps( num, den );
						//video_tracks_.clear();
					}
					else
					{
						atrack = new quicktime_audiotrack( i, qt_movie_, qt_track, qt_media );
					}

					// set up audio-track & store
					if( atrack->open_for_decode( ) )
					{
						audio_index_ = 0;
						audio_tracks_.push_back( atrack );
					}
					else
						delete atrack;
				}
			}
		}

		valid_ = true;
	}


	quicktime_videotrack* quicktime_input::get_video_track() const
	{
		if( !has_video( ) )
			return NULL;
		return video_tracks_[ video_index_ ];
	}

	quicktime_audiotrack* quicktime_input::get_audio_track() const
	{
		if( !has_audio( ) )
			return NULL;
		return audio_tracks_[ audio_index_ ];
	}

