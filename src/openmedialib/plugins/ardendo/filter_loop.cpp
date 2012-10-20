// Loop filter
//
// Copyright (C) 2007 Ardendo
// Released under the terms of the LGPL.
//
// #filter:loop
//
// Similar to offset - provides a way to connect subgraphs to a compositor 
// or mixer and have the inputs loop for the length of the longest input
// on that connected node.

#include "precompiled_headers.hpp"
#include "amf_filter_plugin.hpp"

namespace aml { namespace openmedialib {

class ML_PLUGIN_DECLSPEC filter_loop : public ml::filter_simple
{
	public:
		filter_loop( const std::wstring & )
			: ml::filter_simple( )
		{
		}

		// Indicates if the input will enforce a packet decode
		virtual bool requires_image( ) const { return false; }

		virtual const std::wstring get_uri( ) const { return L"loop"; }

	protected:
		void do_fetch( ml::frame_type_ptr &result )
		{
			acquire_values( );

			ml::input_type_ptr input = fetch_slot( );

			if ( input )
			{
				input->seek( get_position( ) % input->get_frames( ) );
				result = input->fetch( );
				if ( result )
					result->set_position( get_position( ) );
			}
		}
};

ml::filter_type_ptr ML_PLUGIN_DECLSPEC create_loop( const std::wstring &resource )
{
	return ml::filter_type_ptr( new filter_loop( resource ) );
}

} }
