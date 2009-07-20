// Muxer filter
//
// Copyright (C) 2007 Ardendo
//
#include "precompiled_headers.hpp"
#include "amf_filter_plugin.hpp"
#include "utility.hpp"

#include <iostream>

namespace aml { namespace openmedialib {

class ML_PLUGIN_DECLSPEC filter_muxer : public ml::filter_type
{
	public:
		// Filter_type overloads
		explicit filter_muxer( )
			: ml::filter_type( )
			, prop_slot_( pcos::key::from_string( "slot" ) )
		{
			properties( ).append( prop_slot_ = -1 );
		}

		// Indicates if the input will enforce a packet decode
		virtual bool requires_image( ) const { return false; }

		// This provides the name of the plugin (used in serialisation)
		virtual const pl::wstring get_uri( ) const { return L"muxer"; }

		virtual const size_t slot_count( ) const { return 2; }

		virtual int get_frames( ) const
		{
			int result = 0;
			size_t i = 0;
			while ( i < slot_count( ) )
			{
				if ( fetch_slot( i ) )
					result = result == 0 || fetch_slot( i )->get_frames( ) < result ? fetch_slot( i )->get_frames( ) : result;
				i ++;
			}
			return result;
		}

	protected:
		// The main access point to the filter
		void do_fetch( ml::frame_type_ptr &result )
		{
			acquire_values( );

			int slot = prop_slot_.value< int >( );

			if ( slot == -1 )
			{
				result = fetch_from_slot( 0, false );
				ml::frame_type_ptr overlay = fetch_from_slot( 1, false );

				if ( result && overlay && overlay->get_audio( ) )
				{
					result->set_audio( overlay->get_audio( ) );
					join_peaks( result, overlay );
				}
			}
			else
			{
				result = fetch_from_slot( slot, false );
			}

			if ( result )
				result->set_position( get_position( ) );
		}

	private:
		pl::pcos::property prop_slot_;
};

ml::filter_type_ptr ML_PLUGIN_DECLSPEC create_muxer( const pl::wstring & )
{
	return ml::filter_type_ptr( new filter_muxer( ) );
}

} }
