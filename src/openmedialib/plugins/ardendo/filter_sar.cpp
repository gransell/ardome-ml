// Sample Aspect Ratio filter
//
// Copyright (C) 2007 Ardendo
// Released under the terms of the LGPL.
//
// #filter:sar
//
// This filter provides a very basic mechanism to allow sample aspect ratio
// to be overridden.

#include "precompiled_headers.hpp"
#include "amf_filter_plugin.hpp"

namespace aml { namespace openmedialib {

class ML_PLUGIN_DECLSPEC filter_sar : public ml::filter_type
{
	public:
		// Filter_type overloads
		explicit filter_sar( const pl::wstring & )
			: ml::filter_type( )
			, prop_enable_( pcos::key::from_string( "enable" ) )
			, prop_sar_num_( pcos::key::from_string( "sar_num" ) )
			, prop_sar_den_( pcos::key::from_string( "sar_den" ) )
		{
			properties( ).append( prop_enable_ = 1 );
			properties( ).append( prop_sar_num_ = 59 );
			properties( ).append( prop_sar_den_ = 54 );
		}

		// Indicates if the input will enforce a packet decode
		virtual bool requires_image( ) const { return false; }

		// This provides the name of the plugin (used in serialisation)
		virtual const pl::wstring get_uri( ) const { return L"sar"; }

	protected:
		// The main access point to the filter
		void do_fetch( ml::frame_type_ptr &result )
		{
			// Acquire values
			acquire_values( );

			// Frame to return
			result = fetch_from_slot( );

			// Handle the frame with image case
			if ( prop_enable_.value< int >( ) && result && result->has_image( ) )
				result->set_sar( prop_sar_num_.value< int >( ), prop_sar_den_.value< int >( ) );
		}

	protected:
		pcos::property prop_enable_;
		pcos::property prop_sar_num_;
		pcos::property prop_sar_den_;
};

ml::filter_type_ptr ML_PLUGIN_DECLSPEC create_sar( const pl::wstring &resource )
{
	return ml::filter_type_ptr( new filter_sar( resource ) );
}

} }
