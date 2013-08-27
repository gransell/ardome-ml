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

class ML_PLUGIN_DECLSPEC filter_tee : public ml::filter_simple
{
	public:
		filter_tee( )
			: ml::filter_simple( )
			, prop_enable_( pcos::key::from_string( "enable" ) )
			, prop_slots_( pcos::key::from_string( "slots" ) )
			, prop_enforce_fps_( pcos::key::from_string( "enforce_fps" ) )
			, prop_preroll_( pcos::key::from_string( "preroll" ) )
			, position_( 0 )
		{
			properties( ).append( prop_enable_ = 1 );
			properties( ).append( prop_slots_ = 2 );
			properties( ).append( prop_enforce_fps_ = 0 );
			properties( ).append( prop_preroll_ = 25 );
		}

		virtual const size_t slot_count( ) const { return size_t( prop_slots_.value< int >( ) ); }

		// Indicates if the input will enforce a packet decode
		virtual bool requires_image( ) const { return false; }

		virtual const std::wstring get_uri( ) const { return L"tee"; }

	protected:
		void do_fetch( ml::frame_type_ptr &result )
		{
			if ( prop_enable_.value< int >( ) )
			{
				// Keep a queue of preroll frames here and ensure the pushers have some material to work with
				while ( int( queue_.size( ) ) < prop_preroll_.value< int >( ) && position_ < get_frames( ) )
				{
					fetch_slot( 0 )->seek( position_ ++ );
					ml::frame_type_ptr frame = fetch_slot( 0 )->fetch( )->shallow( );
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
		}

		virtual void sync( )
		{
			fetch_slot( 0 )->sync( );
			sync_frames( );

			ml::frame_type_ptr temp = fetch_slot( 0 )->fetch( );

			// For each input, collect pushers, update their duration (via collect)
			// and then sync up the slots that are connected to them
			std::vector < ml::input_type_ptr > pushers;
			for ( size_t i = 1; i < slot_count( ); i ++ )
			{
				if ( prop_enforce_fps_.value< int >( ) && fetch_slot( i )->get_uri( ) != L"frame_rate" )
				{
					ml::filter_type_ptr conform = ml::create_filter( L"conform" );
					conform->property( "image" ).set( 0 );
					conform->property( "audio" ).set( 1 );
					conform->connect( fetch_slot( i ), 0 );
					ml::filter_type_ptr frame_rate = ml::create_filter( L"frame_rate" );
					frame_rate->property( "check_on_connect" ).set( 0 );
					frame_rate->property( "fps_num" ).set( temp->get_fps_num( ) );
					frame_rate->property( "fps_den" ).set( temp->get_fps_den( ) );
					frame_rate->connect( conform, 0 );
					connect( frame_rate, i );
				}
				collect( pushers, fetch_slot( i ) );
				fetch_slot( i )->sync( );
			}
		}

	private:
		// Pass the frame to each of the pushers
		void present_to_pushers( ml::frame_type_ptr frame )
		{
			// For each input, collect pushers
			std::vector < ml::input_type_ptr > pushers;
			for ( size_t i = 1; i < slot_count( ); i ++ )
			{
				collect( pushers, fetch_slot( i ) );
				ARENFORCE( frame->get_position() < fetch_slot(i)->get_frames() );
			}

			// Push the frame to each discovered pusher
			for( std::vector < ml::input_type_ptr >::iterator iter = pushers.begin( ); iter != pushers.end( ); ++iter )
				( *iter )->push(  frame->shallow( ) );
		}

		// Pull a frame through the subgraph
		void process_output( ml::frame_type_ptr &result )
		{
			// Seek and fetch from each input
			for ( size_t i = 1; i < slot_count( ); i ++ )
			{
				fetch_slot( i )->seek( result->get_position( ) );
				ml::frame_type_ptr fr = fetch_slot( i )->fetch( );
				if (fr && fr->in_error())
				{
					for ( size_t i=0; i<fr->exceptions().size(); i++)
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
				if ( graph->get_uri( ) == L"pusher:" || graph->get_uri( ) == L"nudger:" )
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
		pcos::property prop_enforce_fps_;
		pcos::property prop_preroll_;
		int position_;
		std::deque< ml::frame_type_ptr > queue_;
};

ml::filter_type_ptr ML_PLUGIN_DECLSPEC create_tee( const std::wstring &resource )
{
	return ml::filter_type_ptr( new filter_tee( ) );
}

} }
