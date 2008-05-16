// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#include <openmedialib/plugins/quicktime/quicktime_store.h>

// OL INCLUDES
#include <openpluginlib/pl/utf8_utils.hpp>

namespace ml = olib::openmedialib::ml;
using namespace ml;

//---------------------------LIFECYCLE------------------------------------------------------------

quicktime_store::quicktime_store( const opl::wstring &resource, const frame_type_ptr &frame ):
store_type( ),
video_index_( -1 ),
audio_index_( -1 ),
movie_resource_num_( 0 ),
qt_movie_( NULL ),
prop_enable_video_( pcos::key::from_string( "enable_video" ) ),
prop_vfourcc_( pcos::key::from_string( "vfourcc" ) ),
prop_width_( pcos::key::from_string( "width" ) ),
prop_height_( pcos::key::from_string( "height" ) ),
prop_aspect_ratio_( pcos::key::from_string( "aspect_ratio" ) ),
prop_fps_num_( pcos::key::from_string( "fps_num" ) ),
prop_fps_den_( pcos::key::from_string( "fps_den" ) ),
prop_video_bit_rate_( pcos::key::from_string( "video_bit_rate" ) ),
prop_frequency_( pcos::key::from_string( "frequency" ) ),
prop_channels_( pcos::key::from_string( "channels" ) ),
prop_enable_audio_( pcos::key::from_string( "enable_audio" ) )
{
	// initialize quick-time media layer
	#ifdef WIN32
		if(InitializeQTML(0)!=noErr)
			return;
	#endif


	// initialize the movie toolbox
	if (EnterMovies() != noErr)
		return;

	OSStatus err;

	// create a movie spec from the file name
	#ifdef WIN32
		err = NativePathNameToFSSpec( const_cast<char*>( opl::to_string( resource ).c_str( ) ), &movie_file_spec_, 0 );
	#else
		FSRef ref;
		err = FSPathMakeRef( ( const UInt8* ) opl::to_string( resource ).c_str( ), &ref, 0 );
		err = FSGetCatalogInfo( &ref, kFSCatInfoNone, NULL, NULL, &movie_file_spec_, NULL );
	#endif

	// create destination
	err = CreateMovieFile( &movie_file_spec_,
						   FOUR_CHAR_CODE('TVOD'),
						   smCurrentScript,
						   createMovieFileDeleteCurFile | createMovieFileDontCreateResFile,
						   &movie_resource_num_,
						   &qt_movie_ );

	if(err != noErr) 
		return;
	
	// set properties
	properties( ).append( prop_enable_video_ = frame->get_image( ) != 0 ? 1 : 0 );
	properties( ).append( prop_enable_audio_ = frame->get_audio( ) != 0 ? 1 : 0 );

	if ( prop_enable_video_.value< int >( ) == 1 )
	{
		prop_width_ = frame->get_image( )->width( );
		prop_height_ = frame->get_image( )->height( );
		int num;
		int den;
		frame->get_fps( num, den );
		prop_fps_num_ = num;
		prop_fps_den_ = den;
	}

	properties( ).append( prop_vfourcc_ = opl::wstring( L"avc1" ) ); // default codec is h264
	properties( ).append( prop_video_bit_rate_ = 400000 );

	// set audio properties
	properties( ).append( prop_frequency_ = 0 );
	properties( ).append( prop_channels_ = 0 );

	if ( prop_enable_audio_.value< int >( ) == 1 )
	{
		prop_frequency_ = frame->get_audio( )->frequency( );
		prop_channels_ = frame->get_audio( )->channels( );
	}


}

quicktime_store::~quicktime_store( )
{
	cleanup( );
}

//--------------------------PUBLIC-----------------------------------------------------------------

bool quicktime_store::init( )
{
	// create video track
	if(  prop_enable_video_.value< int >( ) == 1  )
	{
		quicktime_videotrack* video_track = new quicktime_videotrack( 0, 
																	  qt_movie_,
																	  prop_width_.value< int >( ), 
																	  prop_height_.value< int >( ),
																	  prop_fps_num_.value< int >( ), 
																	  prop_fps_den_.value< int >( ),
																	  1,
																	  1);

		// initialise
		if( !video_track->open_for_encode( opl::to_string( prop_vfourcc_.value< opl::wstring >( ) ),
											prop_video_bit_rate_.value< int >( ) ) )
			return false;

		video_tracks_.push_back( video_track );
		video_index_ = video_tracks_.size( )-1;
	}

	// create audio track
	if(  prop_enable_audio_.value< int >( ) == 1  )
	{
		quicktime_audiotrack* audio_track_ = new quicktime_audiotrack( qt_movie_,
																	   prop_frequency_.value< int >( ), 
																	   prop_channels_.value< int >( ) );

		// initialise
		if( !audio_track_->open_for_encode( ) )
			return false;

		audio_tracks_.push_back( audio_track_ );
		audio_index_ = audio_tracks_.size( )-1;
	}

	return true;
}

bool quicktime_store::push( frame_type_ptr frame )
{
	// encode to the video track
	if( frame->get_image( ) && has_video( ) )
	{
		if( !get_video_track( )->encode( frame->get_image( ) ) )
			return false;
	}

	// encode to the audio track
	if( frame->get_audio( ) && has_audio( ) )
	{
		if( !get_audio_track( )->encode( frame->get_audio( ) ) )
			return false;
	}

	return true;
}

	
void quicktime_store::complete( )
{ 
	if( has_video( ) )
		get_video_track()->finish_encode( );

	if( has_audio( ) )
		get_audio_track()->finish_encode( );
}

	
//--------------------------PRIVATE----------------------------------------------------------------

void quicktime_store::cleanup( )
{
	for(int i=0; i < video_tracks_.size(); ++i)
		delete video_tracks_[i];

	short res = movieInDataForkResID;
	AddMovieResource( qt_movie_, movie_resource_num_, &res, movie_file_spec_.name );
//	ExitMovies();

	CloseMovieFile( movie_resource_num_ );
	DisposeMovie( qt_movie_ );
	
	#ifdef WIN32	
		TerminateQTML();
	#endif

}

quicktime_audiotrack* quicktime_store::get_audio_track() const
{
	if( !has_audio( ) )
		return NULL;

	return audio_tracks_[ audio_index_ ];
}

quicktime_videotrack* quicktime_store::get_video_track() const
{
	if( !has_video( ) )
		return NULL;

	return video_tracks_[ video_index_ ];
}

