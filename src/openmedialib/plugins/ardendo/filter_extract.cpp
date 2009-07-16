#include "precompiled_headers.hpp"
#include "amf_filter_plugin.hpp"
#include "utility.hpp"

#include <iostream>

namespace amf { namespace openmedialib {

class ML_PLUGIN_DECLSPEC filter_extract : public ml::filter_type
{
	public:
		filter_extract( const pl::wstring & )
			: filter_type( )
			, prop_frames_( pcos::key::from_string( "frames" ) )
		{
			properties( ).append( prop_frames_ = 180 );
		}

		// Indicates if the input will enforce a packet decode
		virtual bool requires_image( ) const { return false; }

		virtual int get_frames( ) const
		{
			return prop_frames_.value< int >( );
		}

		virtual const opl::wstring get_uri( ) const { return L"extract"; }

	protected:
		void do_fetch( ml::frame_type_ptr &result )
		{
			acquire_values( );

			ml::input_type_ptr input = fetch_slot( );

			if ( input && input->get_frames( ) > 0 )
			{
				int length = input->get_frames( );
				int position = 0;
				if ( prop_frames_.value< int >( ) > 1 )
					position = int( ( double( get_position( ) ) / ( prop_frames_.value< int >( ) - 1 ) ) * length );
				input->seek( position );
				result = input->fetch( );
				result->set_position( get_position( ) );
			}
		}

	private:
		pl::pcos::property prop_frames_;
};

ml::filter_type_ptr ML_PLUGIN_DECLSPEC create_extract( const pl::wstring &resource )
{
	return ml::filter_type_ptr( new filter_extract( resource ) );
}

} }
