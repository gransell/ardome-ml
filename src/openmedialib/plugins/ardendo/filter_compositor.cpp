// Compositor filter
//
// Copyright (C) 2007 Ardendo
// Released under the LGPL.
//
// #filter:compositor
//
// Provides an n-ary compositing, mixing and muxing operation.
//
// Inputs are connected between 0 and 'slots - 1'.
//
// Slot 0 holds the background on to which all other frames are composited. It 
// dictates the image resolution and sample aspect ratio which all other inputs
// will be conformed to.
//
// All input sources provide frames which carry any or all of the following:
//
// x = relative x coordinate (floating point - default: 0.0)
// y = relative y coordinate (floating point - default: 1.0)
// z = z order for compositing order (int)
// w = relative w coordinate (floating point - default: 1.0)
// h = relative h coordinate (floating point - default: 1.0)
//
// If the background carries these properties, they are ignored, but passed out
// from the compositor (and hence could be used by another downstream compositor).
//
// Audio will automatically be mixed in z order and the result will be deposited
// on the each frame fetched.
//
// Example:
//
// colour:
// file1.mpg
// filter:lerp @@x=0.0 @@w=0.5
// file2.mpg
// filter:lerp @@x=0.5 @@w=0.5
// filter:compositor slots=3
//
// Will play both file1.mpg and file2.mpg side by side in the same window.

#include "precompiled_headers.hpp"
#include "amf_filter_plugin.hpp"
#include "utility.hpp"

#include <openmedialib/ml/stream.hpp>

#include <iostream>
#include <list>
#include <map>

namespace aml { namespace openmedialib {

typedef std::vector< ml::frame_type_ptr >::iterator frame_vec_it;
typedef std::vector< ml::frame_type_ptr >::reverse_iterator frame_vec_rev_it;

static pl::pcos::key key_z_( pcos::key::from_string( "z" ) );
static pl::pcos::key key_frame_rescale_cb_( pcos::key::from_string( "frame_rescale_cb" ) );
static pl::pcos::key key_image_rescale_cb_( pcos::key::from_string( "image_rescale_cb" ) );
static pl::pcos::key key_is_background_( pcos::key::from_string( "is_background" ) );
static pl::pcos::key key_background_( pcos::key::from_string( "background" ) );
static pl::pcos::key key_slots_( pcos::key::from_string( "slots" ) );
static pl::pcos::key key_priority_frame_( pcos::key::from_string( "priority_frame" ) );
static pl::pcos::key key_in_( pcos::key::from_string( "in" ) );
static pl::pcos::key key_track_item_in_( pcos::key::from_string( "track_item_in" ) );
static pl::pcos::key key_track_item_duration_( pcos::key::from_string( "track_item_duration" ) );
static pl::pcos::key key_events_( pcos::key::from_string( "events" ) );
static pl::pcos::key key_vitc_image_( pcos::key::from_string( "vitc_image" ) );
static pl::pcos::key key_active_on_timeline_( pcos::key::from_string( "active_on_timeline" ) );
static pl::pcos::key key_source_timecode_( pcos::key::from_string( "source_timecode" ) );

static pl::pcos::key key_x_( pcos::key::from_string( "x" ) );
static pl::pcos::key key_y_( pcos::key::from_string( "y" ) );
static pl::pcos::key key_w_( pcos::key::from_string( "w" ) );
static pl::pcos::key key_h_( pcos::key::from_string( "h" ) );
static pl::pcos::key key_mix_( pcos::key::from_string( "mix" ) );
static pl::pcos::key key_mode_( pcos::key::from_string( "mode" ) );

template < typename T > T get_prop( ml::frame_type_ptr frame, pl::pcos::key key, T value )
{
	pl::pcos::property prop = frame->properties( ).get_property_with_key( key );
	if ( prop.valid( ) && prop.is_a< T >( ) )
		return prop.value< T >( );
	return value;
}

//Returns whether or not the given property key is one that is
//"consumed" by the compositor filter (i.e. used to control the
//actual compositing), and thus should not be relayed to the
//resulting frame. This is important if there is another compositor
//further downstream in the graph to avoid applying the same
//compositing there as well.
bool is_consumed_property( const pl::pcos::key &k )
{
	return (
		k == key_x_ || k == key_y_ ||
		k == key_w_ || k == key_h_ ||
		k == key_mix_ || k == key_mode_ ||
		k == key_is_background_ || k == key_background_
	);
}

class priority_node
{
	public:
		priority_node( ml::input_type_ptr input )
			: input_( input )
			, in_( 0 )
			, out_( -1 )
		{
		}

		void set_in_out( int in, int out )
		{
			if ( out_ - in_ != out - in )
				requested_.erase( requested_.begin( ), requested_.end( ) );
			in_ = in;
			out_ = out;
		}

		int in( ) const
		{
			return in_;
		}

		int out( ) const
		{
			return out_;
		}

		int duration( ) const
		{
			return out_ - in_;
		}

		ml::input_type_ptr node( )
		{
			return input_;
		}

		void request( int absolute )
		{
			int relative = get_next_required( absolute );
			if ( relative != -1 )
			{
				PL_LOG( pl::level::info, boost::format( "Requesting %d from %s (%d to %d)" ) 
										 % relative
										 % olib::opencorelib::str_util::to_string( node( )->get_uri( ) )
										 % in( )
										 % out( ) );

				input_->properties( ).get_property_with_key( key_priority_frame_ ) = relative;
				requested_.push_back( relative );
			}
		}

		bool complete( ) const
		{
			return int( requested_.size( ) ) == duration( );
		}

		inline bool in_range( int position )
		{
			return position >= in_ && position < out_;
		}

	private:
		int get_next_required( int )
		{
			// TODO: Replace with a better selection mechanism - left right evaluation is ok for testing
			return !complete( ) ? int( requested_.size( ) ) : -1;
		}

	private:
		ml::input_type_ptr input_;
		int in_;
		int out_;
		std::list< int > requested_;
};

typedef boost::shared_ptr< priority_node > priority_node_ptr;

class priority_list
{
	public:
		typedef std::map< ml::input_type_ptr, priority_node_ptr > node_map;
		typedef std::vector< ml::input_type_ptr > node_list;
		typedef std::vector< priority_node_ptr > priority_items;
		typedef std::map< int, priority_items > index_list;

		priority_list( )
		{ }

		// Walk the graph to regenerate the map and index
		void register_slots( ml::input_type *compositor )
		{
			// We'll construct a new map for the current state
			node_map new_map;

			// We'll collect 'events' here too (basically, just in points of offsets)
			std::vector< int > events;

			for ( size_t i = 0; i < compositor->slot_count( ); i ++ )
			{
				ml::input_type_ptr slot = compositor->fetch_slot( i );

				if ( slot )
				{
					int in = 0;
					int out = slot->get_frames( );
					node_list nodes;

					// Analyse the subgraph
					analyse_graph( slot, nodes, in, out );

					// Update the new map accordingly
					update_map( new_map, nodes, in, out );

					// Propagate in/out to all slots which want it
					propagate_duration( slot, in, out );

					// Collect events
					events.push_back( in );
				}
			}

			// Update the compositor events property
			compositor->properties( ).get_property_with_key( key_events_ ) = events;

			// Update the working map
			map_ = new_map;

			// Convert to the index list
			convert_to_index( );
		}

		// Seek requested to the position
		void seek( int position )
		{
			// TODO: this logic should go into the thread
			priority_items nearest;

			find_nearest( nearest, position );

			for ( priority_items::iterator iter = nearest.begin( ); iter != nearest.end( ); ++iter )
			{
				( *iter )->request( position );
			}
		}

	private:
		// Locate all objects which have priority frame logic
		void analyse_graph( ml::input_type_ptr input, node_list &nodes, int &in, int &out )
		{
			if ( input )
			{
				const std::wstring uri = input->get_uri( );

				// We want to avoid inputs which are are threaded
				// TODO: Introduce a generic test for this
				if ( uri == L"threader" || uri == L"compositor" || uri == L"distributor" )
					return;

				// We need to determine the real in point of this clip
				if ( uri == L"offset" )
					in = input->properties( ).get_property_with_key( key_in_ ).value< int >( );

				// Collect all inputs and filters which have the priority frame id
				if ( input->properties( ).get_property_with_key( key_priority_frame_ ).valid( ) )
					nodes.push_back( input );

				// Now analyse the rest of the graph
				for ( size_t i = 0; i < input->slot_count( ); i ++ )
					analyse_graph( input->fetch_slot( i ), nodes, in, out );
			}
		}

		// Update the map
		void update_map( node_map &new_map, node_list &nodes, int in, int out )
		{
			for ( node_list::iterator iter = nodes.begin( ); iter != nodes.end( ); ++iter )
			{
				if ( new_map.find( *iter ) == new_map.end( ) )
				{
					// Create or acquire as need be
					if ( map_.find( *iter ) == map_.end( ) )
						new_map[ *iter ] = priority_node_ptr( new priority_node( *iter ) );
					else
						new_map[ *iter ] = map_[ *iter ];

					// Refresh in/out points
					new_map[ *iter ]->set_in_out( in, out );

					// TODO: Additional property modification checks when acquired
				}
				else
				{
					// This indicates that the same filter exists at two points in the graph.
					// This isn't strictly an error situation so can be (relatively) safely 
					// ignored. For now, just log the case as a warning.
					PL_LOG( pl::level::warning, boost::format( "Duplicate priority node %s" ) % olib::opencorelib::str_util::to_string( ( *iter )->get_uri( ) ) );
				}
			}
		}

		// Pass in and duration down to the filters which want it
		void propagate_duration( ml::input_type_ptr input, int in, int out )
		{
			for ( size_t i = 0; i < input->slot_count( ); i ++ )
			{
				ml::input_type_ptr slot = input->fetch_slot( i );

				if ( slot )
				{
					if ( slot->properties( ).get_property_with_key( key_track_item_in_ ).valid( ) )
						slot->properties( ).get_property_with_key( key_track_item_in_ ) = in;
					if ( slot->properties( ).get_property_with_key( key_track_item_duration_ ).valid( ) )
						slot->properties( ).get_property_with_key( key_track_item_duration_ ) = out - in;

					propagate_duration( slot, in, out );
				}
			}
		}

		// Convert the map to the index
		void convert_to_index( )
		{
			list_.erase( list_.begin( ), list_.end( ) );

			for ( node_map::iterator iter = map_.begin( ); iter != map_.end( ); ++iter )
			{
				if ( list_.find( iter->second->in( ) ) == list_.end( ) )
					list_[ iter->second->in( ) ] = priority_items( );

				PL_LOG( pl::level::info, boost::format( "Priority %d: %s (%d to %d)" )
										 % list_.size( )
										 % olib::opencorelib::str_util::to_string( iter->second->node( )->get_uri( ) )
										 % iter->second->in( )
										 % iter->second->out( ) );

				list_[ iter->second->in( ) ].push_back( iter->second );
			}
		}

		// Check if the item list has any active items for position
		bool in_range( priority_items &items, int position )
		{
			bool result = false;
			if ( !items.empty( ) )
			{
				for ( priority_items::iterator item = items.begin( ); !result && item != items.end( ); ++item )
					result = ( *item )->in_range( position );
			}
			return result;
		}

		// Find all items in the list which are active for the absolute position provided
		void find_nearest( priority_items &nearest, int position )
		{
			// TODO: replace with something a bit more intelligent (core::span)
			index_list::iterator iter = list_.begin( );

			while ( iter != list_.end( ) )
			{
				for ( priority_items::iterator item = iter->second.begin( ); item != iter->second.end( ); ++item )
					if ( !( *item )->complete( ) && ( ( *item )->in_range( position ) || iter->first > position ) )
						nearest.push_back( *item );
				++iter;
				if ( iter == list_.end( ) || ( !nearest.empty( ) && !in_range( iter->second, position ) ) )
					break;
			}
		}

		node_map map_;
		index_list list_;
};

static bool sort_on_z_order( const ml::frame_type_ptr &lhs, const ml::frame_type_ptr &rhs )
{
	const pcos::property &lhs_prop = lhs->properties( ).get_property_with_key( key_z_ );
	const pcos::property &rhs_prop = rhs->properties( ).get_property_with_key( key_z_ );
	if ( lhs_prop.valid( ) && rhs_prop.valid( ) )
		return lhs_prop.value< double >( ) < rhs_prop.value< double >( );
	else if ( rhs_prop.valid( ) )
		return 0 < rhs_prop.value< double >( );
	else if ( lhs_prop.valid( ) )
		return lhs_prop.value< double >( ) < 0;
	return false;
}

#define const_compositor const_cast< filter_compositor * >

class ML_PLUGIN_DECLSPEC filter_compositor : public ml::filter_type
{
	public:
		// Filter_type overloads
		explicit filter_compositor( const std::wstring &resource )
			: ml::filter_type( )
			, resource_( resource )
			, prop_enable_( pcos::key::from_string( "enable" ) )
			, prop_slots_( pcos::key::from_string( "slots" ) )
			, prop_mono_( pcos::key::from_string( "mono" ) )
			, prop_track_( pcos::key::from_string( "track" ) )
			, prop_mc_( pcos::key::from_string( "mc" ) )
			, prop_mode_( pcos::key::from_string( "mode" ) )
			, prop_deferred_( pcos::key::from_string( "deferred" ) )
			, prop_events_( pcos::key::from_string( "events" ) )
			, prop_update_state_( pcos::key::from_string( "update_state" ) )
			, prop_ignore_pf_( pcos::key::from_string( "ignore_pf" ) )
			, obs_update_state_( new fn_observer< filter_compositor >( const_compositor( this ), &filter_compositor::update_state ) )
			, obs_slots_( new fn_observer< filter_compositor >( const_compositor( this ), &filter_compositor::update_slots ) )
			, pusher_0_( )
			, pusher_1_( )
			, composite_( )
			, priority_list_( )
			, total_frames_( 0 )
		{
			properties( ).append( prop_enable_ = 1 );
			properties( ).append( prop_slots_ = 2 );
			properties( ).append( prop_mono_ = 0 );
			properties( ).append( prop_track_ = 0 );
			properties( ).append( prop_mc_ = 1 );
			properties( ).append( prop_deferred_ = 0 );
			properties( ).append( prop_events_ = std::vector< int >( ) );
			properties( ).append( prop_update_state_ = 0 );
			properties( ).append( prop_ignore_pf_ = 0 );

			prop_update_state_.attach( obs_update_state_ );
			prop_slots_.attach( obs_slots_ );

			pusher_0_ = ml::create_input( L"pusher:" );
			pusher_1_ = ml::create_input( L"pusher:" );
			composite_ = ml::create_filter( L"composite" );

			pusher_0_->init( );
			pusher_1_->init( );
			composite_->init( );

			composite_->connect( pusher_0_, 0 );
			composite_->connect( pusher_1_, 1 );

			pcos::property rx( pcos::key::from_string( "@rx" ) );
			pcos::property ry( pcos::key::from_string( "@ry" ) );
			pcos::property rw( pcos::key::from_string( "@rw" ) );
			pcos::property rh( pcos::key::from_string( "@rh" ) );
			pcos::property mix( pcos::key::from_string( "@mix" ) );
			pcos::property mode( pcos::key::from_string( "@mode" ) );

			composite_->properties( ).append( rx = std::wstring( L"@@x:0" ) );
			composite_->properties( ).append( ry = std::wstring( L"@@y:0" ) );
			composite_->properties( ).append( rw = std::wstring( L"@@w:1" ) );
			composite_->properties( ).append( rh = std::wstring( L"@@h:1" ) );
			composite_->properties( ).append( mix = std::wstring( L"@@mix:1" ) );
			composite_->properties( ).append( mode = std::wstring( L"@@mode:fill" ) );
		}

		// Indicates if the input will enforce a packet decode
		virtual bool requires_image( ) const 
		{ return prop_enable_.value< int >( ) == 1 && prop_deferred_.value< int >( ) == 0; }

		// This provides the name of the plugin (used in serialisation)
		virtual const std::wstring get_uri( ) const { return resource_; }

		virtual const size_t slot_count( ) const { return prop_slots_.value< int >( ); }

		virtual int get_frames( ) const
		{
			return total_frames_;
		}

		virtual void seek( const int position, const bool relative = false )
		{
			ml::filter_type::seek( position, relative );
			priority_list_.seek( get_position( ) );
		}

		void update_state( )
		{
			priority_list_.register_slots( this );
		}

		void update_slots( )
		{
			connect( ml::input_type_ptr( ), prop_slots_.value< int >( ) );
		}

	protected:
		// The main access point to the filter
		void do_fetch( ml::frame_type_ptr &result )
		{
			if ( prop_enable_.value< int >( ) )
			{
				size_t i = prop_track_.value< int >( ) == 0 ? 1 : 0;

				// Stores valid frames from each connected slot
				std::vector< ml::frame_type_ptr > frames;
				int position = get_position( );

				// Fetch the frames
				while( i < slot_count( ) )
				{
					if ( in_range( position, i ) )
					{
						ml::frame_type_ptr overlay = fetch_from_slot( i, false );
						if ( overlay && ( overlay->has_image( ) || overlay->get_audio( ) ) )
							frames.push_back( overlay );
					}
					i ++;
				}
	
				// Sort by the z order
				std::sort( frames.begin( ), frames.end( ), sort_on_z_order );
	
				// Background frame
				if ( prop_track_.value< int >( ) == 0 && fetch_slot( 0 ) )
				{
					result = fetch_from_slot( 0, false );
				}
				else if ( !frames.empty( ) )
				{
					result = frames[ 0 ];
					frames.erase( frames.begin( ) );
				}

				// Make sure we have a result frame
				if ( result == 0 )
					result = ml::frame_type_ptr( new ml::frame_type( ) );

				// Provide basic audio mixing capabilities. This needs to be done before the video
				// composite logic below, since that might exclude video frames which are obscured,
				// but their audio still needs to be heard.
				ml::audio_type_ptr audio;
				if( result->has_audio( ) )
				{
					audio = result->get_audio( );
				}

				for( frame_vec_it iter = frames.begin( ); iter != frames.end( ); ++iter )
				{
					if ( audio && ( *iter )->get_audio( ) )
						audio = ml::audio::mixer( audio, ( *iter )->get_audio( ) );
					else if ( !audio && ( *iter )->get_audio( ) )
						audio = ( *iter )->get_audio( );
				}

				// Indicates if background can actually be removed
				bool is_background = get_prop< int >( result, key_is_background_, 0 ) == 1;

				if ( result && is_background )
				{
					//Go through the frames from top to bottom z order and see if one of them
					//completely obscures everything under it by matching the background
					//dimensions and by having no alpha channel or alpha mix value.
					//If it does, we can save time by not compositing anything below it,
					//since it'll be invisible anyway. If the frame has an encoded video
					//stream attached to it, this also allows us to reuse that as-is without
					//any re-encoding.
					for( frame_vec_rev_it rev_it = frames.rbegin(); rev_it != frames.rend(); ++rev_it )
					{
						const ml::frame_type_ptr &frame = *rev_it;

						// We need to check if the next frame is the background and whether it has has alpha
						frame_vec_rev_it check = rev_it;

						// Determine if the next frame is the background and check if it has alpha
						bool next_frame_bg_alpha = false;
						if ( ( ++ check ) == frames.rend( ) )
							next_frame_bg_alpha = result->has_alpha( );

						//If we didn't explicitly set a sar value for the background,
						//we'll inherit the one from the first input source
						if ( result->get_sar_num() == -1 && frame->get_sar_num() != -1 )
							result->set_sar(frame->get_sar_num(), frame->get_sar_den());

						if ( frame->width( ) == result->width( ) &&
							 frame->height( ) == result->height( ) &&
							 frame->get_sar_num( ) == result->get_sar_num( ) &&
							 frame->get_sar_den( ) == result->get_sar_den( ) && 
							 get_prop< double >( frame, key_x_, 0.0 ) == 0.0 &&
							 get_prop< double >( frame, key_y_, 0.0 ) == 0.0 &&
							 get_prop< double >( frame, key_w_, 1.0 ) == 1.0 &&
							 get_prop< double >( frame, key_h_, 1.0 ) == 1.0 &&
							 get_prop< double >( frame, key_mix_, 1.0 ) == 1.0 &&
							 matching_modes( frame, result ) &&
							 ( !frame->has_alpha( ) || next_frame_bg_alpha ) )
						{
							//This frame completely obscures everything below it.
							//We make this the new background frame and remove all the frames
							//below it in the z-ordered frame list.

							ARLOG_DEBUG7( "Foreground match %d %s" )( get_position( ) )( frame->pf() );
							if ( frame->pf() == result->pf() || prop_ignore_pf_.value< int >( ) )
								result = frame;
							else
								result = ml::frame_convert( frame, result->pf( ) );
							
							frames.erase( frames.begin(), rev_it.base() );
							break;
						}
						else
						{
							ARLOG_DEBUG7( "Foreground mismatch %d" )( get_position( ) );
						}
					}
				}

				// Ignore deferred when running in muxer mode
				bool deferred = prop_deferred_.value< int >( ) != 0 && resource_ == L"compositor";
				pl::pcos::property vitc( key_vitc_image_ );

				// If we're not in deferred mode, we need to convert to a format which is supported now
				ml::image::MLPixelFormat original_pf = result->ml_pixel_format( );

				// Avoid pixel format convert if we're not going to composite anything below anyway
				if ( !frames.empty() )
				{
					if ( !deferred && original_pf != ml::image::composite_pf( original_pf ) )
						result = ml::frame_convert( ml::rescale_object_ptr( ), result, ml::image::composite_pf( original_pf ) );
				}

				// Composite in bottom to top z order
				for( frame_vec_it iter = frames.begin( ); iter != frames.end( ); ++iter )
				{
					if ( ( *iter )->has_image( ) )
					{
						if( deferred )
						{
							//Deferred mode; just add this frame to the queue on the root frame.
							//The store is responsible for compositing them.
							result->push( *iter );
						}
						else
						{
							//Non-deferred mode. Do the composite ourselves.
							pusher_0_->push( ml::frame_type_ptr( ) );
							pusher_1_->push( ml::frame_type_ptr( ) );
							pusher_0_->push( result );
							pusher_1_->push( *iter );
							composite_->seek( get_position( ) );
							result = composite_->fetch( );
						}

						//Relay all properties
						pcos::key_vector keys = (*iter)->properties().get_keys();
						const pcos::property_container &props = (*iter)->properties();
						pcos::key_vector::const_iterator cit( keys.begin()), eit(keys.end());
						for( ; cit != eit; ++cit )
						{
							if( !is_consumed_property( *cit ) )
							{
								pcos::property pcos_prop = props.get_property_with_key(*cit);
								if( !pcos_prop.valid() ) continue;
								result->properties( ).append( pcos_prop );
							}
						}
					}

					/* We set the VITC based on the active_on_timeline property and
					 * the z-order. If the active_on_timeline property is not used
					 * the z-order will be used alone. */ 
					bool is_active_on_timeline = false;
					if  ((*iter )->property_with_key( key_active_on_timeline_ ).valid( )) {
						is_active_on_timeline = (*iter )->property_with_key( key_active_on_timeline_ ).value< int >() == 1;
					} else {
						// If no active_on_timeline is set, just fall back to true
						// and let the z order rule.
						is_active_on_timeline = true;
					}

					if ( ( is_active_on_timeline && ( *iter )->property_with_key( key_vitc_image_ ).valid( ) ) ) {
						vitc = ( *iter )->property_with_key( key_vitc_image_ ).value< ml::image_type_ptr >( );
					}
				}

				if ( audio )
					result->set_audio( audio );

				if ( vitc.is_a< ml::image_type_ptr >( ) )
				{
					if ( result->property_with_key( key_vitc_image_ ).valid( ) )
						result->property_with_key( key_vitc_image_ ) = vitc.value< ml::image_type_ptr >( );
					else
						result->properties( ).append( vitc );
				}

				if ( deferred && !frames.empty() && prop_track_.value<int>() == 0 )
				{
					if ( !result->properties( ).get_property_with_key( key_background_ ).valid( ) )
					{
						pl::pcos::property background( key_background_ );
						result->properties().append( background = 1 );
						pl::pcos::property slots( key_slots_ );
						result->properties().append( slots = 1 + int( frames.size( ) ) );
					}
					else
					{
						result->properties( ).get_property_with_key( key_slots_ ) = 
							result->properties( ).get_property_with_key( key_slots_ ).value< int >( ) + int( frames.size( ) );
					}
				}

				// Convert it back to the original pf if it's changed
				if ( !deferred && original_pf != result->ml_pixel_format( ) )
					result = ml::frame_convert( ml::rescale_object_ptr( ), result, original_pf );
			}
			else
			{
				result = fetch_from_slot( prop_mono_.value< int >( ) );
			}

			if ( result )
				result->set_position( get_position( ) );
		}

		virtual void sync_frames( )
		{
			ranges_.erase( ranges_.begin( ), ranges_.end( ) );
			total_frames_ = 0;
			size_t i = prop_track_.value< int >( ) == 0 ? 1 : 0;
			if ( i != 0 )
				ranges_.push_back( std::pair< int, int >( -1, -1 ) );
			while ( i < slot_count( ) )
			{
				ml::input_type_ptr input = fetch_slot( i );
				if ( input )
				{
					total_frames_ = input->get_frames( ) > total_frames_ ? input->get_frames( ) : total_frames_;
					ml::input_type_ptr offset = find_offset( input );
					if ( offset )
						ranges_.push_back( std::pair< int, int >( offset->property_with_key( key_in_ ).value< int >( ), offset->fetch_slot( 0 )->get_frames( ) ) );
					else
						ranges_.push_back( std::pair< int, int >( -1, -1 ) );
				}
				else
				{
					ranges_.push_back( std::pair< int, int >( 0, 0 ) );
				}
				i ++;
			}
		}

		bool matching_modes( ml::frame_type_ptr frame1, ml::frame_type_ptr frame2 )
		{
			std::wstring mode1 = get_prop< std::wstring >( frame1, key_mode_, std::wstring( L"fill" ) );
			std::wstring mode2 = get_prop< std::wstring >( frame2, key_mode_, std::wstring( L"fill" ) );

			if( mode1 == mode2 )
			{
				return true;
			}
			else if( ( mode1 == L"ardendo" && mode2 == L"fill" ) || 
					( mode1 == L"fill" && mode2 == L"ardendo" ) )
			{
				return true;
			}

			return false;
		}

		inline ml::input_type_ptr find_offset( ml::input_type_ptr input )
		{
			ml::input_type_ptr result;
			while( !result && input->slot_count( ) == 1 && input->fetch_slot( 0 ) )
			{
				if ( input->get_uri( ) == L"offset" )
					result = input;
				else
					input = input->fetch_slot( 0 );
			}
			return result;
		}

		inline bool in_range( int position, size_t i )
		{
			if ( i < ranges_.size( ) )
			{
				std::pair< int, int > &pair = ranges_[ i ];
				if ( pair.first == 0 && pair.second == 0 )
					return false;
				if ( pair.first == -1 && pair.second == -1 )
					return true;
				position -= pair.first;
				return position >= 0 && position < pair.second;
			}
			return true;
		}

		std::wstring resource_;
		pcos::property prop_enable_;
		pcos::property prop_slots_;
		pcos::property prop_mono_;
		pcos::property prop_track_;
		pcos::property prop_mc_;
		pcos::property prop_mode_;
		pcos::property prop_deferred_;
		pcos::property prop_events_;
		pcos::property prop_update_state_;
		pcos::property prop_ignore_pf_;
		boost::shared_ptr< pl::pcos::observer > obs_update_state_;
		boost::shared_ptr< pl::pcos::observer > obs_slots_;
		ml::input_type_ptr pusher_0_;
		ml::input_type_ptr pusher_1_;
		ml::filter_type_ptr composite_;
		priority_list priority_list_;
		int total_frames_;
		std::vector< std::pair< int, int > > ranges_;
};

ml::filter_type_ptr ML_PLUGIN_DECLSPEC create_compositor( const std::wstring &resource )
{
	return ml::filter_type_ptr( new filter_compositor( resource ) );
}

} }
