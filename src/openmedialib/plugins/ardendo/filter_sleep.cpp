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
			, start_( boost::get_system_time( ) )
		{
			properties( ).append( prop_ms_ = 0 );
		}

		// Indicates if the input will enforce a packet decode
		virtual bool requires_image( ) const { return false; }

		virtual const pl::wstring get_uri( ) const { return L"sleep"; }

	protected:
		void do_fetch( ml::frame_type_ptr &result )
		{
			boost::system_time now = boost::get_system_time( );
			int milliseconds = prop_ms_.value< int >( );

			if ( milliseconds == 0 && now < start_ )
				sleeper_.current_thread_sleep( start_ - now );
			else
				start_ = now;
	
			result = fetch_from_slot( );
			ARENFORCE_MSG( result, "No frame obtained from input" );
			ARENFORCE_MSG( result->fps( ) > 0.0, "Invalid frame rate of %f" )( result->fps( ) );

			start_ = start_ + boost::posix_time::milliseconds( 1000.0 / result->fps( ) );
			
			if ( milliseconds )
				sleeper_.current_thread_sleep( boost::posix_time::milliseconds( milliseconds ) );
		}

		pcos::property prop_ms_;
		olib::opencorelib::thread_sleeper sleeper_;
		boost::system_time start_;
};

ml::filter_type_ptr ML_PLUGIN_DECLSPEC create_sleep( const pl::wstring &resource )
{
	return ml::filter_type_ptr( new filter_sleep( resource ) );
}

} }
