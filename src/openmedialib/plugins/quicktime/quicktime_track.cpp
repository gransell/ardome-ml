// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#include <openmedialib/plugins/quicktime/quicktime_track.h>

namespace ml = olib::openmedialib::ml;
using namespace ml;

//---------------------------LIFECYCLE------------------------------------------------------------

quicktime_track::quicktime_track( int track_num,
							    Movie& qt_movie,
							    Track  qt_track,
							    Media  qt_media ):
track_num_( track_num ),
qt_parent_movie_( qt_movie ),
qt_track_( qt_track ),
qt_media_( qt_media ),
fps_num_( 25 ),
fps_den_( 1 ),
track_units_per_frame_( -1 ),
expected_frame_pos_( 0 )
{
	
}

quicktime_track::quicktime_track( int track_num,
								 Movie& qt_movie ):
track_num_( track_num ),
qt_parent_movie_( qt_movie ),
fps_num_( 25 ),
fps_den_( 1 ),
track_units_per_frame_( -1 ),
expected_frame_pos_( 0 )
{
	
}

quicktime_track::~quicktime_track( )
{
}

//--------------------------PUBLIC-----------------------------------------------------------------

bool quicktime_track::open_for_decode( )
{
	return true;
}

double quicktime_track::fps( ) const
{
	return ((double)fps_num_) / ((double)fps_den_);
}


//--------------------------PRIVATE----------------------------------------------------------------


