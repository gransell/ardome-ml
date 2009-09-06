// Misplaced loop filter (needs a proper home)
//
// Copyright (C) 2007 Ardendo
#include "precompiled_headers.hpp"
#include "amf_filter_plugin.hpp"

namespace aml { namespace openmedialib {

class ML_PLUGIN_DECLSPEC filter_loop : public ml::filter_type
{
	public:
		filter_loop( const pl::wstring & )
			: ml::filter_type( )
		{
		}

		// Indicates if the input will enforce a packet decode
		virtual bool requires_image( ) const { return false; }

		virtual const pl::wstring get_uri( ) const { return L"loop"; }

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

ml::filter_type_ptr ML_PLUGIN_DECLSPEC create_loop( const pl::wstring &resource )
{
	return ml::filter_type_ptr( new filter_loop( resource ) );
}

} }