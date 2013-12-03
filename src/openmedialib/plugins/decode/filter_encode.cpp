// decode - A decode plugin to ml.
//
// Copyright (C) 2010 Vizrt
// Released under the LGPL.

#include "filter_encode.hpp"
#include "frame_lazy.hpp"
#include <opencorelib/cl/enforce_defines.hpp>
#include <opencorelib/cl/profile.hpp>
#include <openmedialib/ml/utilities.hpp>

static pl::pcos::key key_force_ = pl::pcos::key::from_string( "force" );
static pl::pcos::key key_width_ = pl::pcos::key::from_string( "width" );
static pl::pcos::key key_height_ = pl::pcos::key::from_string( "height" );
static pl::pcos::key key_fps_num_ = pl::pcos::key::from_string( "fps_num" );
static pl::pcos::key key_fps_den_ = pl::pcos::key::from_string( "fps_den" );
static pl::pcos::key key_sar_num_ = pl::pcos::key::from_string( "sar_num" );
static pl::pcos::key key_sar_den_ = pl::pcos::key::from_string( "sar_den" );
static pl::pcos::key key_interlace_ = pl::pcos::key::from_string( "interlace" );
static pl::pcos::key key_bit_rate_ = pl::pcos::key::from_string( "bit_rate" );
static pl::pcos::key key_instances_ = pl::pcos::key::from_string( "instances" );


namespace olib { namespace openmedialib { namespace ml { namespace decode {


filter_encode::filter_encode( )
		: filter_encode_type( )
		, prop_filter_( pl::pcos::key::from_string( "filter" ) )
		, prop_profile_( pl::pcos::key::from_string( "profile" ) )
		, prop_instances_( pl::pcos::key::from_string( "instances" ) )
		, prop_enable_( pl::pcos::key::from_string( "enable" ) )
		, decoder_( )
		, gop_encoder_( )
		, is_long_gop_( false )
		, last_frame_( )
		, profile_to_encoder_mappings_( )
		, stream_validation_( false )
{
	// Default to something. Will be overriden anyway as soon as we init
	properties( ).append( prop_filter_ = std::wstring( L"mcencode" ) );
	// Default to something. Should be overriden anyway.
	properties( ).append( prop_profile_ = std::wstring( L"vcodecs/avcintra100" ) );
	properties( ).append( prop_instances_ = 4 );

	// Enabled by default
	properties( ).append( prop_enable_ = 1 );
	
	the_shared_filter_pool::Instance( ).add_pool( this );
}

filter_encode::~filter_encode( )
{
	the_shared_filter_pool::Instance( ).remove_pool( this );
}

// Indicates if the input will enforce a packet decode
bool filter_encode::requires_image( ) const { return false; }

const std::wstring filter_encode::get_uri( ) const { return std::wstring( L"encode" ); }

const size_t filter_encode::slot_count( ) const { return 1; }


ml::filter_type_ptr filter_encode::filter_create( )
{
	boost::recursive_mutex::scoped_lock lck( mutex_ );
	// Create the encoder that we have mapped from our profile
	ml::filter_type_ptr encode = ml::create_filter( prop_filter_.value< std::wstring >( ) );
	ARENFORCE_MSG( encode, "Failed to create encoder" )( prop_filter_.value< std::wstring >( ) );

	// Set the profile property on the encoder
	ARENFORCE_MSG( encode->properties( ).get_property_with_string( "profile" ).valid( ),
		"Encode filter must have a profile property" );
	encode->properties( ).get_property_with_string( "profile" ) = prop_profile_.value< std::wstring >( );

	if ( encode->properties( ).get_property_with_key( key_instances_ ).valid( ) )
		encode->properties( ).get_property_with_key( key_instances_ ) = prop_instances_.value< int >( );
	
	return encode;
}

ml::filter_type_ptr filter_encode::filter_obtain( )
{
	boost::recursive_mutex::scoped_lock lck( mutex_ );
	ml::filter_type_ptr result = gop_encoder_;
	if ( !result )
	{
		if ( decoder_.size( ) )
		{
			result = decoder_.back( );
			decoder_.pop_back( );
		}
		else
		{
			result = filter_create( );
		}

		if( result->properties( ).get_property_with_key( key_force_ ).valid( ) ) 
			result->properties( ).get_property_with_key( key_force_ ) = prop_force_.value< int >( );
		if( result->properties( ).get_property_with_key( key_width_ ).valid( ) ) 
			result->properties( ).get_property_with_key( key_width_ ) = prop_width_.value< int >( );
		if( result->properties( ).get_property_with_key( key_height_ ).valid( ) ) 
			result->properties( ).get_property_with_key( key_height_ ) = prop_height_.value< int >( );
		if( result->properties( ).get_property_with_key( key_fps_num_ ).valid( ) ) 
			result->properties( ).get_property_with_key( key_fps_num_ ) = prop_fps_num_.value< int >( );
		if( result->properties( ).get_property_with_key( key_fps_den_ ).valid( ) ) 
			result->properties( ).get_property_with_key( key_fps_den_ ) = prop_fps_den_.value< int >( );
		if( result->properties( ).get_property_with_key( key_sar_num_ ).valid( ) ) 
			result->properties( ).get_property_with_key( key_sar_num_ ) = prop_sar_num_.value< int >( );
		if( result->properties( ).get_property_with_key( key_sar_den_ ).valid( ) ) 
			result->properties( ).get_property_with_key( key_sar_den_ ) = prop_sar_den_.value< int >( );
		if( result->properties( ).get_property_with_key( key_interlace_ ).valid( ) ) 
			result->properties( ).get_property_with_key( key_interlace_ ) = prop_interlace_.value< int >( );
		if( result->properties( ).get_property_with_key( key_bit_rate_ ).valid( ) ) 
			result->properties( ).get_property_with_key( key_bit_rate_ ) = prop_bit_rate_.value< int >( );
	}
	return result;
}

void filter_encode::filter_release( ml::filter_type_ptr filter )
{
	boost::recursive_mutex::scoped_lock lck( mutex_ );
	if ( filter != gop_encoder_ )
		decoder_.push_back( filter );
}

void filter_encode::push( ml::input_type_ptr input, ml::frame_type_ptr frame )
{
	if ( input && frame )
	{
		if ( input->get_uri( ) == L"pusher:" )
			input->push( frame );
		for( size_t i = 0; i < input->slot_count( ); i ++ )
			push( input->fetch_slot( i ), frame );
	}
}

bool filter_encode::create_pushers( )
{
	if( is_long_gop_ )
	{
		ARLOG_DEBUG3( "Creating gop_encoder in encode filter" );
		gop_encoder_ = filter_create( );
		if( gop_encoder_->properties( ).get_property_with_key( key_force_ ).valid( ) ) 
			pl::pcos::assign< int >( gop_encoder_->properties( ), key_force_, prop_force_.value< int >( ) );
		gop_encoder_->connect( fetch_slot( 0 ) );
		
		return gop_encoder_.get( ) != 0;
	}
	return false;
}

void filter_encode::do_fetch( frame_type_ptr &frame )
{
	if ( prop_enable_.value<int>() == 0)
	{
		frame = fetch_from_slot( );
		return;
	}

	int frameno = get_position( );

	if ( last_frame_ == 0 )
	{
		initialize_encoder_mapping( );
		create_pushers( );
	}

	if ( last_frame_ && last_frame_->get_position( ) == get_position( ) )
	{
		frame = last_frame_->shallow();
	}
	else if ( gop_encoder_ )
	{
		gop_encoder_->seek( frameno );
		frame = gop_encoder_->fetch( );
	}
	else
	{
		ml::filter_type_ptr graph;
		frame = fetch_from_slot( );
		frame->set_position( get_position( ) );

		// Check the first frame here so that any unspecified properties are picked up at this point
		if ( last_frame_ == 0 )
		{
			matching( frame );
			ARENFORCE_MSG( valid( frame ), "Invalid frame for encoder" );
		}

		bool validate = matching( frame );
		if ( validate && stream_validation_ )
			validate = true;

		frame = ml::frame_type_ptr( new frame_lazy( frame, get_frames( ), this, validate ) );
	}

	// Keep a reference to the last frame in case of a duplicated request
	last_frame_ = frame->shallow();
}

void filter_encode::initialize_encoder_mapping( )
{
	// Load the profile that contains the mappings between codec string and codec filter name
	profile_to_encoder_mappings_ = cl::profile_load( "encoder_mappings" );
	
	// First figure out what encoder to use based on our profile
	ARENFORCE_MSG( profile_to_encoder_mappings_, "Need mappings from profile string to name of encoder" );
	
	std::string prof = cl::str_util::to_string( prop_profile_.value< std::wstring >( ).c_str( ) );
	cl::profile::list::const_iterator it = profile_to_encoder_mappings_->find( prof );
	ARENFORCE_MSG( it != profile_to_encoder_mappings_->end( ), "Failed to find a apropriate encoder" )( prof );

	ARLOG_DEBUG3( "Will use encode filter \"%1%\" for profile \"%2%\"" )( it->value )( prop_profile_.value< std::wstring >( ) );
	
	prop_filter_ = std::wstring( cl::str_util::to_wstring( it->value ) );
	
	// Now load the actual profile to find out what video codec string we will produce
	cl::profile_ptr encoder_profile = cl::profile_load( prof );
	ARENFORCE_MSG( encoder_profile, "Failed to load encode profile" )( prof );
	
	cl::profile::list::const_iterator vc_it = encoder_profile->find( "stream_codec_id" );
	ARENFORCE_MSG( vc_it != encoder_profile->end( ), "Profile is missing a stream_codec_id" );

	video_codec_ = vc_it->value;

	vc_it = encoder_profile->find( "video_gop_size" );
	if( vc_it != encoder_profile->end() && vc_it->value != "1" )
	{
		ARLOG_DEBUG3( "Video codec profile %1% is long gop, since video_gop_size is %2%" )( prof )( vc_it->value );
		is_long_gop_ = true;
	}

	vc_it = encoder_profile->find( "stream_validation" );
	stream_validation_ = is_long_gop_ || ( vc_it != encoder_profile->end() && vc_it->value == "1" );
	if ( stream_validation_ )
	{
		ARLOG_DEBUG3( "Encoder plugin is responsible for validating the stream" );
	}
}

} } } }
