// Repeat filter
//
// Copyright (C) 2012 VizRT
// Released under the LGPL.
//
// #filter:repeat
//
// Repeats the input multiple times using the requested method.
//
// To repeat the connected input 3 times, the following could be used:
//
// file.mpeg
// filter:repeat type=repeat count=3
//
// To have the input alternate between forward and reverse play:
//
// file.mpeg
// filter:repeat type=pingpong count=2
//
// To have the input alternate between reverse and forward play:
//
// file.mpeg
// filter:repeat type=pongping count=2
//
// Notes:
//
// Interlaced inputs have the field order reversed during the reverse playout
// parts of the pingpong/pongping types.
//
// To disable the filter, a count <= 1 is sufficient.

#include "precompiled_headers.hpp"
#include "amf_filter_plugin.hpp"
#include "utility.hpp"

namespace aml { namespace openmedialib {

class ML_PLUGIN_DECLSPEC filter_repeat : public ml::filter_type
{
	public:
		filter_repeat( const std::wstring & )
		: prop_type_( pcos::key::from_string( "type" ) )
		, prop_count_( pcos::key::from_string( "count" ) )
		, src_frames_( 0 )
		{
			types_[ L"repeat" ] = -1;
			types_[ L"pongping" ] = 0;
			types_[ L"pingpong" ] = 1;
			properties( ).append( prop_type_ = std::wstring( L"repeat" ) );
			properties( ).append( prop_count_ = 2 );
		}

		// Indicates if the input will enforce a packet decode
		bool requires_image( ) const { return false; }

		int get_frames( ) const
		{
			const int count = prop_count_.value< int >( );
			return count > 0 && src_frames_ != std::numeric_limits< int >::max( ) ? src_frames_ * count : src_frames_;
		}

		const std::wstring get_uri( ) const { return L"repeat"; }

		void on_slot_change( ml::input_type_ptr input, int )
		{  
			reverse_ = ml::input_type_ptr( );
		}

	protected:
		void do_fetch( ml::frame_type_ptr &result )
		{
			const std::wstring type = prop_type_.value< std::wstring >( );
			ARENFORCE_MSG( types_.find( type ) != types_.end( ), "Invalid type specified %s" )( type );

			ml::input_type_ptr input = fetch_slot( );
			ARENFORCE_MSG( input, "No input connected to repeat filter" );

			if ( src_frames_ == 0 ) sync( );

			const int reversed = types_[ type ];
			const int position = get_position( );
			const bool active = src_frames_ != std::numeric_limits< int >::max( ) && prop_count_.value< int >( ) >= 1;

			if ( active )
				result = obtain_frame( input, position, reversed );
			else
				result = input->fetch( position );
		}

		void sync_frames( )
		{
			src_frames_ = fetch_slot( ) ? fetch_slot( )->get_frames( ) : 0;
		}

	private:
		ml::frame_type_ptr obtain_frame( ml::input_type_ptr input, const int position, const int reversed )
		{
			ml::frame_type_ptr result;
			int src_frame = position % src_frames_;
			int section = position / src_frames_;
			if ( !reverse_ ) reverse_ = reverse( input );
			if ( section % 2 == reversed )
				result = reverse_->fetch( src_frame );
			else
				result = input->fetch( src_frame );
			ARENFORCE_MSG( result, "Unable to obtain a frame %d from the connected input" )( src_frame );
			result = result->shallow( );
			result->set_position( position );
			return result;
		}

		ml::input_type_ptr reverse( ml::input_type_ptr input )
		{
			ml::frame_type_ptr sample = input->fetch( );
			ARENFORCE_MSG( sample, "Unable to obtain a sample frame from the connected input" );

			if ( sample->has_image( ) && sample->get_image( )->field_order( ) != ml::image::progressive )
			{
				ml::image::field_order_flags order = sample->get_image( )->field_order( );
				ml::filter_type_ptr field_order = ml::create_filter( L"field_order" );
				field_order->property( "order" ) = int( order == ml::image::top_field_first ? ml::image::bottom_field_first : ml::image::top_field_first );
				field_order->connect( input );
				input = field_order;
			}

			ml::filter_type_ptr clip = ml::create_filter( L"clip" );
			clip->property( "in" ) = -1;
			clip->property( "out" ) = 0;
			clip->connect( input );

			clip->sync( );

			return clip;
		}

		pl::pcos::property prop_type_;
		pl::pcos::property prop_count_;
		int src_frames_;
		ml::input_type_ptr reverse_;
		std::map< std::wstring, int > types_;
};

ml::filter_type_ptr ML_PLUGIN_DECLSPEC create_repeat( const std::wstring &resource )
{
	return ml::filter_type_ptr( new filter_repeat( resource ) );
}

} }
