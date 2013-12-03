// decode - A decode plugin to ml.
//
// Copyright (C) 2010 Vizrt
// Released under the LGPL.

#include "filter_decode.hpp"
#include "filter_analyse.hpp"
#include "frame_lazy.hpp"
#include <opencorelib/cl/str_util.hpp>
#include <opencorelib/cl/uuid_16b.hpp>
#include <opencorelib/cl/profile.hpp>
#include <opencorelib/cl/enforce_defines.hpp>
#include <openmedialib/ml/utilities.hpp>
#include <openmedialib/ml/audio_types.hpp>
#include <openmedialib/ml/audio_block.hpp>

static pl::pcos::key key_length_ = pl::pcos::key::from_string( "length" );

namespace olib { namespace openmedialib { namespace ml { namespace decode {


filter_decode::filter_decode( )
	: filter_type( )
	, prop_inner_threads_( pl::pcos::key::from_string( "inner_threads" ) )
	, prop_filter_( pl::pcos::key::from_string( "filter" ) )
	, prop_audio_filter_( pl::pcos::key::from_string( "audio_filter" ) )
	, prop_scope_( pl::pcos::key::from_string( "scope" ) )
	, prop_source_uri_( pl::pcos::key::from_string( "source_uri" ) )
	, decoder_( )
	, codec_to_decoder_( )
	, initialized_( false )
	, total_frames_( 0 )
	, prefetched_frame_( )
	, precharged_frames_( 0 )
	, estimated_gop_size_( 0 )
{
	properties( ).append( prop_inner_threads_ = -1 ); // -N resolves to #cores/(-1*N), e.g. -2 will resolve to 4 on an 8 core machine
	properties( ).append( prop_filter_ = std::wstring( L"mcdecode" ) );
	properties( ).append( prop_audio_filter_ = std::wstring( L"" ) );
	properties( ).append( prop_scope_ = std::wstring( cl::str_util::to_wstring( cl::uuid_16b().to_hex_string( ) ) ) );
	properties( ).append( prop_source_uri_ = std::wstring( L"" ) );
	
	// Load the profile that contains the mappings between codec string and codec filter name
	codec_to_decoder_ = cl::profile_load( "codec_mappings" );
	
	the_shared_filter_pool::Instance( ).add_pool( this );
}

filter_decode::~filter_decode( )
{
	the_shared_filter_pool::Instance( ).remove_pool( this );
}

		// Indicates if the input will enforce a packet decode
bool filter_decode::requires_image( ) const
{ 
	return false; 
}

const std::wstring filter_decode::get_uri( ) const 
{ 
	return std::wstring( L"decode" ); 
}

const size_t filter_decode::slot_count( ) const 
{ 
	return 1; 
}

void filter_decode::sync( )
{
	total_frames_ = 0;

	if ( gop_decoder_ )
	{
		gop_decoder_->sync( );
		total_frames_ = gop_decoder_->get_frames( );

		// Avoid smeared frames due to incomplete sources
		if ( !fetch_slot( 0 )->complete( ) )
			total_frames_ -= 2;
	}
	else if ( fetch_slot( 0 ) )
	{
		fetch_slot( 0 )->sync( );
		total_frames_ = fetch_slot( 0 )->get_frames( );
	}
}


ml::filter_type_ptr filter_decode::filter_create( const std::wstring& codec_filter )
{
	boost::recursive_mutex::scoped_lock lck( mutex_ );
	ml::input_type_ptr fg = ml::create_input( L"pusher:" );
	fg->property( "length" ) = get_frames( );
	
	// FIXME: Create audio decode filter
	ml::filter_type_ptr decode = ml::create_filter( codec_filter );
	ARENFORCE_MSG( decode, "Could not get a valid decoder filter" );
	if ( decode->property( "threads" ).valid( ) ) 
		decode->property( "threads" ) = prop_inner_threads_.value< int >( );
	if ( decode->property( "scope" ).valid( ) ) 
		decode->property( "scope" ) = prop_scope_.value< std::wstring >( );
	if ( decode->property( "source_uri" ).valid( ) ) 
		decode->property( "source_uri" ) = prop_source_uri_.value< std::wstring >( );
	if ( decode->property( "decode_video" ).valid( ) )
		decode->property( "decode_video" ) = 1;
	
	decode->connect( fg );
	decode->sync( );
	
	return decode;
}

ml::filter_type_ptr filter_decode::filter_obtain( )
{
	boost::recursive_mutex::scoped_lock lck( mutex_ );
	ml::filter_type_ptr result;
	
	if ( decoder_.size( ) == 0 )
	{
		result = filter_create( prop_filter_.value< std::wstring >( ) );
		decoder_.push_back( result );
		ARLOG_INFO( "Creating decoder. This = %1%, source_uri = %2%, scope = %3%" )( this )( prop_source_uri_.value< std::wstring >( ) )( prop_scope_.value< std::wstring >( ) );
	}
	
	result = decoder_.back( );
	decoder_.pop_back( );

	return result;
}

void filter_decode::filter_release( ml::filter_type_ptr filter )
{
	boost::recursive_mutex::scoped_lock lck( mutex_ );
	decoder_.push_back( filter );
}

bool filter_decode::determine_decode_use( ml::frame_type_ptr &frame )
{
	if( frame && frame->get_stream() && frame->get_stream( )->estimated_gop_size( ) != 1 )
	{
		ml::filter_type_ptr analyse = ml::filter_type_ptr( new filter_analyse( ) );
		analyse->connect( fetch_slot( 0 ) );
		gop_decoder_ = ml::create_filter( prop_filter_.value< std::wstring >( ) );
		if ( gop_decoder_->property( "threads" ).valid( ) ) 
			gop_decoder_->property( "threads" ) = prop_inner_threads_.value< int >( );
		if ( gop_decoder_->property( "scope" ).valid( ) ) 
			gop_decoder_->property( "scope" ) = prop_scope_.value< std::wstring >( );
		if ( gop_decoder_->property( "source_uri" ).valid( ) ) 
			gop_decoder_->property( "source_uri" ) = prop_source_uri_.value< std::wstring >( );
		if ( gop_decoder_->property( "decode_video" ).valid( ) )
			gop_decoder_->property( "decode_video" ) = 1;
		
		gop_decoder_->connect( analyse );
		gop_decoder_->sync( );
		return true;
	}
	return false;
}

void filter_decode::do_fetch( frame_type_ptr &frame )
{
	if ( last_frame_ == 0 )
	{
		frame = fetch_from_slot( );
		
		// If there is nothing to decode just return frame
		if( !frame->get_stream( ) && !frame->audio_block( ) )
			return;

		if( !initialized_ )
		{
			this->resolve_inner_threads_prop( );

			if( frame->get_stream( ) )
				initialize_video( frame );
			
			if( frame->audio_block( ) )
				initialize_audio( frame );
			initialized_ = true;
		}
		
		determine_decode_use( frame );

		//Set last_frame_ here, since get_frames() below needs it to be able
		//to estimate the GOP size.
		last_frame_ = frame->shallow();
	}

	int frameno = get_position( );
	frameno += (int)precharged_frames_;

	if ( gop_decoder_ )
	{
		gop_decoder_->seek( frameno );
		frame = gop_decoder_->fetch( );
	}
	else 
	{
		if ( !frame ) frame = fetch_from_slot( );
		if ( frame && frame->get_stream( ) )
			frame = ml::frame_type_ptr( new frame_lazy( frame, get_frames( ), this ) );
	}
	
	// If the frame has an audio block we need to decode that so pass it to the
	// audio decoder
	if( audio_decoder_ )
	{
		frame_type_ptr audio_frame = perform_audio_decode( frame );
		frame->set_audio( audio_frame->get_audio( ), true );
	}

	frame->set_position( get_position() );

	last_frame_ = frame->shallow();
}

int filter_decode::get_frames( ) const 
{
	int frames = filter_type::get_frames( );

	ARLOG_DEBUG4( "filter:decode::get_frames() says %1% frames, subtracting %2% precharged frames." )( frames )( precharged_frames_ );

	frames -= precharged_frames_;

	input_type_ptr slot = fetch_slot( 0 );

	if ( slot->has_valid_duration( ) && slot->get_valid_duration( ) != -1 ) {
		frames = slot->get_valid_duration( );
		ARLOG_DEBUG4( "filter:decode::get_frames() found valid_duration property with value %1%, using it." )( frames );
	}

	// Check if the source is complete. If it isn't, we'll
	// subtract a gop from the duration in case of rollout.
	if ( !slot->complete( ) && estimated_gop_size() != 1 && frames != 0 && frames != INT_MAX )
		frames -= estimated_gop_size( );

	return frames;
}

void filter_decode::initialize_video( ml::frame_type_ptr &first_frame )
{
	ARENFORCE_MSG( codec_to_decoder_, "Need mappings from codec string to name of decoder" );
	ARENFORCE_MSG( first_frame && first_frame->get_stream( ), "No frame or no stream on frame" );

	last_frame_ = first_frame->shallow();

	precharged_frames_ = 0;
	input_type_ptr slot = fetch_slot( 0 );
	if ( slot->has_first_valid_frame( ) )
		precharged_frames_ = slot->get_first_valid_frame( );

	cl::profile::list::const_iterator it = codec_to_decoder_->find( first_frame->get_stream( )->codec( ) );
	ARENFORCE_MSG( it != codec_to_decoder_->end( ), "Failed to find a apropriate codec" )( first_frame->get_stream( )->codec( ) );
	
	ARLOG_DEBUG( "Stream identifier is %1%. Using decode filter %2%" )( first_frame->get_stream( )->codec( ) )( it->value );
	
	prop_filter_ = std::wstring( cl::str_util::to_wstring( it->value ) );
}

void filter_decode::initialize_audio( ml::frame_type_ptr &first_frame )
{
	// If the user has set the audio_filter property to something meaningfull
	// then we honor that.
	if( prop_audio_filter_.value< std::wstring >() == L"" )
	{
		ARENFORCE_MSG( codec_to_decoder_, "Need mappings from codec string to name of decoder" );
		ARENFORCE_MSG( first_frame && first_frame->audio_block( ), "No frame or no stream on frame" );
		
		ml::audio::block_type_ptr audio_block = first_frame->audio_block();
		ARENFORCE_MSG( audio_block->tracks.size(), "No tracks in audio block" );
		ARENFORCE_MSG( audio_block->tracks.begin()->second.packets.size(),
					   "No packets in first track in audio block" );
		
		std::string codec = audio_block->tracks.begin()->second.packets.begin( )->second->codec( );
		cl::profile::list::const_iterator it = codec_to_decoder_->find( codec );
		ARENFORCE_MSG( it != codec_to_decoder_->end( ), "Failed to find a apropriate codec" )( codec );
		
		ARLOG_INFO( "Stream itentifier is %1%. Using decode filter %2%" )( codec )( it->value );
		
		prop_audio_filter_ = std::wstring( cl::str_util::to_wstring( it->value ) );
	}
	
	ARENFORCE_MSG( audio_decoder_ = filter_create( prop_audio_filter_.value< std::wstring >( ) ),
				   "Failed to create audio decoder filter." )( prop_audio_filter_.value< std::wstring >( ) );
	if( audio_decoder_->property( "decode_video" ).valid( ) )
		audio_decoder_->property( "decode_video" ) = 0;
}


frame_type_ptr filter_decode::perform_audio_decode( const frame_type_ptr& frame )
{
	input_type_ptr pusher = audio_decoder_->fetch_slot( 0 );
	pusher->property_with_key( key_length_ ) = get_frames( );
	audio_decoder_->sync( );
	pusher->push( frame );
	audio_decoder_->seek( get_position( ) );
	return audio_decoder_->fetch( );
}

int filter_decode::estimated_gop_size( ) const
{
	if ( estimated_gop_size_ <= 0 )
	{
		frame_type_ptr frame = last_frame_;
		if ( !frame )
		{
			//We need a frame to be able to draw any conclusions
			//about gop size.
			input_type_ptr input = fetch_slot(0);
			ARENFORCE( input );
			ARENFORCE( frame = input->fetch() );
		}

		stream_type_ptr stream = frame->get_stream( );
		int gop = 0;

		if ( stream )
			gop = stream->estimated_gop_size( );

		if ( gop <= 0 )
		{
			// Guess gop size = framerate / 2
			int fps = ::ceil( frame->fps() );
			gop = fps / 2;
		}

		estimated_gop_size_ = gop;
	}

	return estimated_gop_size_;
}

void filter_decode::resolve_inner_threads_prop( )
{
	if ( prop_inner_threads_.value< int >( ) < 0 )
	{
		prop_inner_threads_ = std::max( int(boost::thread::hardware_concurrency()) / ( -1 * prop_inner_threads_.value< int >( ) ) , 1 );
	}

}


} } } }

