// Transport filter
//
// Copyright (C) 2008 Ardendo
// Released under the terms of the LGPL.
//
// #filter:transport
//
// A transport filter to allow independent play out control over subgraphs.

#include "precompiled_headers.hpp"
#include "amf_filter_plugin.hpp"
#include "utility.hpp"

namespace aml { namespace openmedialib {

class ML_PLUGIN_DECLSPEC filter_transport : public ml::filter_type
{
	public:
		filter_transport( )
			: ml::filter_type( )
			, prop_enable_( pcos::key::from_string( "enable" ) )
			, prop_speed_( pcos::key::from_string( "speed" ) )
			, prop_position_( pcos::key::from_string( "position" ) )
			, prop_loop_( pcos::key::from_string( "loop" ) )
			, prop_silence_( pcos::key::from_string( "silence" ) )
			, last_position_( -1 )
		{
			properties( ).append( prop_enable_ = 1 );
			properties( ).append( prop_position_ = 0 );
			properties( ).append( prop_speed_ = 1 );
			properties( ).append( prop_loop_ = 1 );
			properties( ).append( prop_silence_ = 0 );
		}

		// Indicates if the input will enforce a packet decode
		virtual bool requires_image( ) const { return false; }

		virtual const std::wstring get_uri( ) const { return L"transport"; }

		virtual int get_frames( ) const 
		{ 
			if ( fetch_slot( ) && fetch_slot( )->get_frames( ) )
				return get_position( ) + fetch_slot( )->get_frames( ) - ( prop_position_.value< int >( ) % fetch_slot( )->get_frames( ) ) + 1; 
			else
				return 0;
		}

	protected:
		void do_fetch( ml::frame_type_ptr &result )
		{
			acquire_values( );

			if ( prop_enable_.value< int >( ) )
			{
				ml::input_type_ptr input = fetch_slot( );
				if ( input && input->get_frames( ) )
				{
					input->seek( prop_position_.value< int >( ) % input->get_frames( ) );
					result = input->fetch( );
					if ( result )
					{
						prop_position_ = prop_position_.value< int >( ) + prop_speed_.value< int >( );

						if ( !prop_loop_.value< int >( ) )
						{
							if ( prop_position_.value< int >( ) < 0 )
								prop_position_ = 0;
							else if ( prop_position_.value< int >( ) >= input->get_frames( ) )
								prop_position_ = input->get_frames( ) - 1;
						}

						if ( prop_silence_.value< int >( ) || prop_speed_.value< int >( ) == 0 || result->get_position( ) == last_position_ )
						{
							result = result->shallow( );
							result->set_audio( ml::audio_type_ptr( ) );
						}

						last_position_ = result->get_position( );
					}
				}
			}
			else
			{
				result = fetch_from_slot( );
				prop_position_ = result->get_position( );
				last_position_ = result->get_position( );
			}

			result->set_position( get_position( ) );
		}

	private:
		pcos::property prop_enable_;
		pcos::property prop_speed_;
		pcos::property prop_position_;
		pcos::property prop_loop_;
		pcos::property prop_silence_;
		int last_position_;
};

ml::filter_type_ptr ML_PLUGIN_DECLSPEC create_transport( const std::wstring &resource )
{
	return ml::filter_type_ptr( new filter_transport( ) );
}

} }
