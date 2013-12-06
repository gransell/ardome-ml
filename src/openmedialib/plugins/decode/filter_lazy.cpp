// decode - A decode plugin to ml.
//
// Copyright (C) 2010 Vizrt
// Released under the LGPL.

#include "filter_lazy.hpp"
#include "frame_lazy.hpp"
#include <openmedialib/ml/utilities.hpp>

namespace olib { namespace openmedialib { namespace ml { namespace decode {

filter_lazy::filter_lazy( const std::wstring &spec )
	: filter_type( )
	, spec_( spec )
{
	filter_ = ml::create_filter( spec_.substr( 5 ) );
	properties_ = filter_->properties( );
}

filter_lazy::~filter_lazy( )
{
}

// Indicates if the input will enforce a packet decode
bool filter_lazy::requires_image( ) const { return false; }

const std::wstring filter_lazy::get_uri( ) const { return spec_; }

const size_t filter_lazy::slot_count( ) const { return filter_ ? filter_->slot_count( ) : 1; }

bool filter_lazy::is_thread_safe( ) { return filter_ ? filter_->is_thread_safe( ) : false; }

void filter_lazy::on_slot_change( input_type_ptr input, int slot )
{
	if ( filter_ )
	{
		filter_->connect( input, slot );
		filter_->sync( );
	}
}

int filter_lazy::get_frames( ) const
{
	return filter_ ? filter_->get_frames( ) : 0;
}

ml::filter_type_ptr filter_lazy::filter_obtain( )
{
	boost::recursive_mutex::scoped_lock lck( mutex_ );
	if ( filters_.size( ) == 0 )
		filters_.push_back( filter_create( ) );

	ml::filter_type_ptr result = filters_.back( );
	filters_.pop_back( );

	pcos::key_vector keys = properties_.get_keys( );
	for( pcos::key_vector::iterator it = keys.begin( ); it != keys.end( ); ++it )
	{
		if ( result->property_with_key( *it ).valid( ) )
			result->property_with_key( *it ).set_from_property( properties_.get_property_with_key( *it ) );
		else
		{
			pl::pcos::property prop( *it );
			prop.set_from_property( properties_.get_property_with_key( *it ) );
			result->properties( ).append( prop );
		}
	}

	return result;
}

ml::filter_type_ptr filter_lazy::filter_create( )
{
	return ml::create_filter( spec_.substr( 5 ) );
}

void filter_lazy::filter_release( ml::filter_type_ptr filter )
{
	boost::recursive_mutex::scoped_lock lck( mutex_ );
	filters_.push_back( filter );
}

void filter_lazy::do_fetch( frame_type_ptr &frame )
{
	frame = fetch_from_slot( 0 );
	if ( filter_ )
	{
		boost::shared_ptr< filter_lazy > shared_self = 
			boost::dynamic_pointer_cast< filter_lazy >( shared_from_this( ) );
		frame = ml::frame_type_ptr( new frame_lazy( frame, get_frames( ), shared_self ) );
	}
}

void filter_lazy::sync_frames( )
{
	if ( filter_ )
		filter_->sync( );
}

} } } }

