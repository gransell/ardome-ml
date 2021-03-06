// null store - useful for testing
//
// Copyright (C) 2007 Ardendo
// Released under the LGPL.
//
// #store:null:
//
// Produces a report of the pushed frame to stderr. Returns false on a push to
// simplify the generation of a report on a single frame in a normal push to 
// store loop, but the return can be ignored and the store is fine for multiple
// use.

#include "precompiled_headers.hpp"
#include "amf_filter_plugin.hpp"
#include "utility.hpp"
#include <iostream>

namespace aml { namespace openmedialib {

class ML_PLUGIN_DECLSPEC store_null : public ml::store_type
{
	public:
		store_null( ) 
			: ml::store_type( )
			, prop_enabled_( pcos::key::from_string( "enabled" ) )
			, prop_count_( pcos::key::from_string( "count" ) )
			, prop_deferrable_( pcos::key::from_string( "deferrable" ) )
			, prop_evaluate_( pcos::key::from_string( "evaluate" ) )
		{
			properties( ).append( prop_enabled_ = 1 );
			properties( ).append( prop_count_ = 1 );
			properties( ).append( prop_deferrable_ = 0 );
			properties( ).append( prop_evaluate_ = 1 );
		}

		virtual ~store_null( )
		{ }

		virtual bool push( ml::frame_type_ptr frame )
		{
			if ( frame && prop_enabled_.value< int >( ) )
			{
				std::cout << "--------------------------------------------------------------------------------" << std::endl;
				std::deque< ml::frame_type_ptr > queue = ml::frame_type::unfold( frame );
				for( std::deque< ml::frame_type_ptr >::iterator iter = queue.begin( ); iter != queue.end( ); ++iter )
				{
					bool evaluate = prop_evaluate_.value< int >( );
					frame_report_basic( *iter, evaluate );
					frame_report_image( *iter, evaluate );
					frame_report_alpha( *iter );
					frame_report_audio( *iter );
					frame_report_props( *iter );
				}
			}
			else if ( prop_enabled_.value< int >( ) )
			{
				std::cerr << "null: No frame received.\n" << std::endl;
			}
			prop_count_.set< int >( prop_count_.value< int >( ) - 1 );
			return prop_count_.value< int >( ) != 0;
		}

		virtual ml::frame_type_ptr flush( )
		{ return ml::frame_type_ptr( ); }

		virtual void complete( )
		{ }

	protected:
		pcos::property prop_enabled_;
		pcos::property prop_count_;
		pcos::property prop_deferrable_;
		pcos::property prop_evaluate_;
};

ml::store_type_ptr ML_PLUGIN_DECLSPEC create_store_null( )
{
	return ml::store_type_ptr( new store_null( ) );
}

} }
