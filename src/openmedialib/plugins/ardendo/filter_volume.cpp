// Volume filter
//
// Copyright (C) 2007 Ardendo
// Released under the terms of the LGPL.
//
// #filter:volume
//
// This filter provides a very basic mechanism to control volume.

#include "precompiled_headers.hpp"
#include <limits>
#include <cmath>
#include <iostream>
#include "amf_filter_plugin.hpp"
#include "utility.hpp"

namespace aml { namespace openmedialib {

class ML_PLUGIN_DECLSPEC filter_volume : public ml::filter_type
{
	public:
		// Filter_type overloads
		explicit filter_volume( const pl::wstring & )
			: ml::filter_type( )
			, prop_enable_( pcos::key::from_string( "enable" ) )
			, prop_volume_( pcos::key::from_string( "volume" ) )
			, prop_ramp_( pcos::key::from_string( "ramp" ) )
		{
			properties( ).append( prop_enable_ = 1 );
			properties( ).append( prop_volume_ = 1.0 );
			properties( ).append( prop_ramp_ = std::vector< double >( ) );
		}

		// This provides the name of the plugin (used in serialisation)
		virtual const pl::wstring get_uri( ) const { return L"volume"; }

		// Indicates if the input will enforce a packet decode
		virtual bool requires_image( ) const { return false; }

	protected:
		// The main access point to the filter
		void do_fetch( ml::frame_type_ptr &result )
		{
			// Frame to return
			result = fetch_from_slot( );

			// Apply the volume ramp or the fixed volume
			if ( prop_enable_.value< int >( ) )
			{
				double volume = prop_volume_.value< double >( );

				if ( prop_ramp_.is_a< std::vector< double > >( ) )
				{
					std::vector< double > ramp = prop_ramp_.value< std::vector< double > >( );
    				if ( ramp.size( ) )
						apply_volume( result, volume * ramp[ 0 ], volume * ramp.back( ) );
					else
						apply_volume( result, volume, volume );
				}
				else
				{
					apply_volume( result, volume, volume );
				}
			}
		}

		pcos::property prop_enable_;
		pcos::property prop_volume_;
		pcos::property prop_ramp_;
};

ml::filter_type_ptr ML_PLUGIN_DECLSPEC create_volume( const pl::wstring &resource )
{
	return ml::filter_type_ptr( new filter_volume( resource ) );
}

} }
