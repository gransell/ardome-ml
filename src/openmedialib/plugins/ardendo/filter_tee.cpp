// Tee filter
//
// Copyright (C) 2009 Ardendo

#include "precompiled_headers.hpp"
#include "amf_filter_plugin.hpp"
#include "utility.hpp"

namespace aml { namespace openmedialib {

static pcos::key key_length_( pcos::key::from_string( "length" ) );

class ML_PLUGIN_DECLSPEC filter_tee : public ml::filter_type
{
	public:
		filter_tee( )
			: ml::filter_type( )
			, prop_enable_( pcos::key::from_string( "enable" ) )
			, prop_slots_( pcos::key::from_string( "slots" ) )
			, last_frame_( )
		{
			properties( ).append( prop_enable_ = 1 );
			properties( ).append( prop_slots_ = 2 );
		}

		virtual const size_t slot_count( ) const { return size_t( prop_slots_.value< int >( ) ); }

		// Indicates if the input will enforce a packet decode
		virtual bool requires_image( ) const { return false; }

		virtual const pl::wstring get_uri( ) const { return L"tee"; }

	protected:
		void do_fetch( ml::frame_type_ptr &result )
		{
			acquire_values( );

			// Fetch input frame or reuse last frame
			if ( !last_frame_ || last_frame_->get_position( ) != get_position( ) )
				result = fetch_from_slot( 0 );
			else
				result = last_frame_;

			// Only do this while enabled and avoid pushing duplicates
			if ( prop_enable_.value< int >( ) && result != last_frame_ )
			{
				// For each input, collect pushers
				std::vector < ml::input_type_ptr > pushers;
				for ( size_t i = 1; i < slot_count( ); i ++ )
					collect( pushers, fetch_slot( i ) );

				// Push the frame to each discovered pusher
				for( std::vector < ml::input_type_ptr >::iterator iter = pushers.begin( ); iter != pushers.end( ); iter ++ )
					( *iter )->push(  ml::frame_type::shallow_copy( result ) );

				// Seek and fetch from each input
				for ( size_t i = 1; i < slot_count( ); i ++ )
				{
					fetch_slot( i )->seek( get_position( ) );
					fetch_slot( i )->fetch( );
				}
			}

			last_frame_ = result;
		}

	private:
		// Walk the graph and look for pusher inputs - nested tee's require that only the 0th
		// slot is walked.

		void collect( std::vector< ml::input_type_ptr > &pushers, ml::input_type_ptr graph )
		{
			if ( graph )
			{

				if ( graph->get_uri( ) == L"pusher:" )
				{
					graph->properties( ).get_property_with_key( key_length_ ) = get_frames( );
					pushers.push_back( graph );
				}

				for( size_t i = 0; i < graph->slot_count( ); i ++ )
				{
					collect( pushers, graph->fetch_slot( i ) );
					if ( graph->get_uri( ) == L"tee" )
						break;
				}
			}
		}

		pcos::property prop_enable_;
		pcos::property prop_slots_;
		ml::frame_type_ptr last_frame_;
};

ml::filter_type_ptr ML_PLUGIN_DECLSPEC create_tee( const pl::wstring &resource )
{
	return ml::filter_type_ptr( new filter_tee( ) );
}

} }
