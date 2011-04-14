// Step filter
//
// Copyright (C) 2009 Ardendo
// Released under the terms of the LGPL.
//
// #filter:step
//
// Steps through the connected input at the specified speed.
//
// Example:
//
// file.mpg
// filter:step speed=2
//
// reduces the length the graph by half and shows every second frame from
// file.mpg.

#include "precompiled_headers.hpp"
#include "amf_filter_plugin.hpp"
#include "utility.hpp"

#include <iostream>

namespace aml { namespace openmedialib {

class ML_PLUGIN_DECLSPEC filter_step : public ml::filter_type
{
	public:
		filter_step( const pl::wstring & )
			: filter_type( )
			, prop_speed_( pcos::key::from_string( "speed" ) )
		{
			properties( ).append( prop_speed_ = 2 );
		}

		// Indicates if the input will enforce a packet decode
		virtual bool requires_image( ) const { return false; }

		virtual int get_frames( ) const
		{
			if ( prop_speed_.value< int >( ) != 0 )
				return ml::filter_type::get_frames( ) / prop_speed_.value< int >( );
			return ml::filter_type::get_frames( );
		}

		virtual const opl::wstring get_uri( ) const { return L"step"; }

	protected:
		void do_fetch( ml::frame_type_ptr &result )
		{
			acquire_values( );

			ml::input_type_ptr input = fetch_slot( );

			if ( input && input->get_frames( ) > 0 )
			{
				int position = get_position( );
				if ( prop_speed_.value< int >( ) != 0 )
					position *= prop_speed_.value< int >( );
				input->seek( position );
				result = input->fetch( );
				result->set_position( get_position( ) );
			}
		}

	private:
		pl::pcos::property prop_speed_;
};

ml::filter_type_ptr ML_PLUGIN_DECLSPEC create_step( const pl::wstring &resource )
{
	return ml::filter_type_ptr( new filter_step( resource ) );
}

} }
