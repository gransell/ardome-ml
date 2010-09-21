// Sleep filter
//
// Copyright (C) 2007 Ardendo
// Released under the terms of the LGPL.
//
// #filter:mono
//
// All this filter does is sleep for a specified milliseconds in the do_fetch method

#include "precompiled_headers.hpp"
#include "amf_filter_plugin.hpp"

#include <opencorelib/cl/time_helpers.hpp>

namespace aml { namespace openmedialib {

class ML_PLUGIN_DECLSPEC filter_sleep : public ml::filter_simple
{
	public:
		filter_sleep( const pl::wstring & )
			: ml::filter_simple( )
			, prop_ms_( pcos::key::from_string( "ms" ) )
			, sleeper_( )
		{
			properties( ).append( prop_ms_ = 0 );
		}

		// Indicates if the input will enforce a packet decode
		virtual bool requires_image( ) const { return false; }

		virtual const pl::wstring get_uri( ) const { return L"sleep"; }

	protected:
		void do_fetch( ml::frame_type_ptr &result )
		{
			result = fetch_from_slot( );
			
			int milliseconds = prop_ms_.value< int >( );
			if ( milliseconds )
			{
				sleeper_.current_thread_sleep( boost::posix_time::milliseconds( milliseconds ) );
			}
		}

		pcos::property prop_ms_;
		olib::opencorelib::thread_sleeper sleeper_;
};

ml::filter_type_ptr ML_PLUGIN_DECLSPEC create_sleep( const pl::wstring &resource )
{
	return ml::filter_type_ptr( new filter_sleep( resource ) );
}

} }
