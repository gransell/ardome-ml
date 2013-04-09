// Frame List filter
//
// Copyright (C) 2013 Ardendo
// Released under the terms of the LGPL.
//
// #filter:frame_list
//
// Fetches the frames specified in "frames" configuration from its input.
//
// Example:
//
// file.mpg
// filter:frame_list frames="2 : 10 : 1 : 20"
//
// Will reorder the frames from the input to the new order 2, 10, 1, 20

#include "precompiled_headers.hpp"
#include "amf_filter_plugin.hpp"
#include "utility.hpp"

#include <iostream>

namespace aml { namespace openmedialib {

class ML_PLUGIN_DECLSPEC filter_frame_list : public ml::filter_type
    {
	public:
	    filter_frame_list( const std::wstring & )
		: filter_type( )
		  , prop_frames_( pcos::key::from_string( "frames" ) )
	{
	    properties( ).append( prop_frames_ = std::vector<int>() );
	}

	    // Indicates if the input will enforce a packet decode
	    virtual bool requires_image( ) const { return true; }

	    virtual int get_frames( ) const
	    {
		return prop_frames_.value< std::vector<int> >().size();
	    }

	    virtual const std::wstring get_uri( ) const { return L"frame_list"; }

	protected:
	    void do_fetch( ml::frame_type_ptr &result )
	    {
		ml::input_type_ptr input = this->fetch_slot( );

		if ( input && input->get_frames( ) > 0 )
		{
		    const std::vector< int >& frames = prop_frames_.value< std::vector< int > >( );
		    int position_to_get = frames[ this->get_position( ) ];

		    // check that input has the number of frames as requested by current frame in frame list
		    ARENFORCE_MSG( input->get_frames( ) >=  position_to_get  , "Input does not have frame specified in frame list (input frames: %1% < frame requested: %2%) ") ( input->get_frames( ) ) ( position_to_get ) ;

		    input->seek( position_to_get );
		    result = input->fetch( );
		    result->set_position( get_position( ) );
		}
	    }

	private:
	    pl::pcos::property prop_frames_;
    };

ml::filter_type_ptr ML_PLUGIN_DECLSPEC create_frame_list( const std::wstring &resource )
{
	return ml::filter_type_ptr( new filter_frame_list( resource ) );
}

} }
