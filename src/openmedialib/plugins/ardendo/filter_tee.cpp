// Tee filter
//
// Copyright (C) 2009 Ardendo
// Released under the terms of the LGPL.
//
// #filter:tee
// 
// Allow complex graphs to be connected at multiple points to the same input.
// This is used in conjunction with a pusher: input in the following way:
//
// <input> colour: pusher: filter:compositor filter:store store=out.dv filter:tee 
//
// The graph can then be continued, adding additional effects, or providing
// addition tee graphs which are encoding to another form of output.

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
			, position_( 0 )
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
			// Only do this while enabled and avoid pushing duplicates
			if ( last_frame_ && last_frame_->get_position( ) == get_position( ) )
			{
				result = last_frame_;
				process_output( result );
			}
			else if ( prop_enable_.value< int >( ) )
			{
				// Keep a queue of 12 frames here and ensure the pushers have some material to work with
				while ( queue_.size( ) < 12 && position_ < get_frames( ) )
				{
					fetch_slot( 0 )->seek( position_ ++ );
					ml::frame_type_ptr frame = fetch_slot( 0 )->fetch( );
					present_to_pushers( frame );
					queue_.push_back( frame );
				}

				// Get the currently requested frame and process the queue accordingly
				if ( queue_.size( ) )
				{
					result = queue_[ 0 ];
					queue_.pop_front( );
					process_output( result );
				}
			}
			else
			{
				result = fetch_from_slot( 0 );
			}

			last_frame_ = result;
		}

	private:
		// Pass the frame to each of the pushers
		void present_to_pushers( ml::frame_type_ptr frame )
		{
			// For each input, collect pushers
			std::vector < ml::input_type_ptr > pushers;
			for ( size_t i = 1; i < slot_count( ); i ++ )
				collect( pushers, fetch_slot( i ) );

			// Push the frame to each discovered pusher
			for( std::vector < ml::input_type_ptr >::iterator iter = pushers.begin( ); iter != pushers.end( ); iter ++ )
				( *iter )->push(  frame->shallow( ) );
		}

		// Pull a frame through the subgraph
		void process_output( ml::frame_type_ptr result )
		{
			// Seek and fetch from each input
			for ( size_t i = 1; i < slot_count( ); i ++ )
			{
				fetch_slot( i )->seek( result->get_position( ) );
				ml::frame_type_ptr fr = fetch_slot( i )->fetch( );
				if ( fr )
					std::cerr << "request for " << result->get_position( ) << " got " << fr->get_position( ) << std::endl;
				else
					std::cerr << "request for " << result->get_position( ) << " got nothing" << std::endl;
				if (fr && fr->in_error())
				{
					for (int i=0; i<fr->exceptions().size(); i++)
					{
						result->push_exception(fr->exceptions()[i].first, fr->exceptions()[i].second);
					}
				}
			}
		}

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
		int position_;
		std::deque< ml::frame_type_ptr > queue_;
		ml::frame_type_ptr last_frame_;
};

ml::filter_type_ptr ML_PLUGIN_DECLSPEC create_tee( const pl::wstring &resource )
{
	return ml::filter_type_ptr( new filter_tee( ) );
}

} }
