// Copyright (C) 2010 Vizrt
// Released under the LGPL.

#include "frame_lazy.hpp"
#include <opencorelib/cl/enforce_defines.hpp>
#include <openmedialib/ml/utilities.hpp>
#include <openmedialib/ml/fix_stream.hpp>

namespace pl = olib::openpluginlib;
namespace ml = olib::openmedialib::ml;
namespace il = olib::openmedialib::ml;
namespace pcos = olib::openpluginlib::pcos;
namespace cl = olib::opencorelib;

static pl::pcos::key key_length_ = pl::pcos::key::from_string( "length" );

namespace olib { namespace openmedialib { namespace ml { namespace decode {

//########################################################################################

frame_lazy::filter_pool_holder::filter_pool_holder( filter_pool *pool )
	: pool_( pool )
{ }

frame_lazy::filter_pool_holder::~filter_pool_holder()
{
}

ml::filter_type_ptr frame_lazy::filter_pool_holder::filter()
{
	filter_ = filter_ ? filter_ : the_shared_filter_pool::Instance().filter_obtain( pool_ );
	return filter_;
}

void frame_lazy::filter_pool_holder::release( )
{
	if ( filter_ )
		the_shared_filter_pool::Instance().filter_release( filter_, pool_ );
	filter_ = ml::filter_type_ptr( );
}

//########################################################################################


frame_lazy::frame_lazy( const frame_type_ptr &other, int frames, filter_pool *pool_token, bool validated )
	: ml::frame_type( other.get( ) )
	, parent_( other )
	, frames_( frames )
	, pool_holder_( new filter_pool_holder( pool_token ) )
	, validated_( validated )
	, evaluated_( 0 )
{
	ARENFORCE( other->get_position() < frames )(other->get_position())(frames);
}

frame_lazy::~frame_lazy( )
{
}

void frame_lazy::evaluate( component comp )
{
	filter_type_ptr filter;
	evaluated_ |= comp;
	if ( pool_holder_ && ( filter = pool_holder_->filter( ) ) )
	{
		ml::frame_type_ptr other = parent_;

		{
			ml::input_type_ptr pusher = filter->fetch_slot( 0 );

			if ( pusher == 0 )
			{
				pusher = ml::create_input( L"pusher:" );
				pusher->property_with_key( key_length_ ) = frames_;
				filter->connect( pusher );
				filter->sync( );
			}
			else if ( pusher->get_uri( ) == L"pusher:" && pusher->get_frames( ) != frames_ )
			{
				pusher->property_with_key( key_length_ ) = frames_;
				filter->sync( );
			}

			// Do any codec specific modifications of the packet to make it decodable
			ml::fix_stream( other );

			pusher->push( other );
			filter->seek( other->get_position( ) );
			other = filter->fetch( );

			ARENFORCE( !other->in_error() );

			//Empty pusher in case filter didn't read from it due to caching
			pusher->fetch( );
		}

		if ( comp == eval_image )
		{
			image_ = other->get_image( );
			alpha_ = other->get_alpha( );
		}

		audio_ = other->get_audio( );
		//properties_ = other->properties( );
		if ( comp == eval_stream )
		{
			stream_ = other->get_stream( );
		}
		pts_ = other->get_pts( );
		duration_ = other->get_duration( );
		//sar_num_ = other->get_sar_num( );
		//sar_den_ = other->get_sar_den( );
		fps_num_ = other->get_fps_num( );
		fps_den_ = other->get_fps_den( );
		exceptions_ = other->exceptions( );
		queue_ = other->queue( );

		//We will not have any use for the filter now
		pool_holder_->release();
	}
}

/// Provide a shallow copy of the frame (and all attached frames)
frame_type_ptr frame_lazy::shallow( ) const
{
	ml::frame_type_ptr clone( new frame_lazy( this, parent_->shallow() ) );
	return clone;
}

/// Provide a deep copy of the frame (and all attached frames)
frame_type_ptr frame_lazy::deep( )
{
	frame_type_ptr result = parent_->deep( );
	result->set_image( get_image( ) );
	return result;
}

/// Indicates if the frame has an image
bool frame_lazy::has_image( )
{
	return !( evaluated_ & eval_image ) || image_;
}

/// Indicates if the frame has audio
bool frame_lazy::has_audio( )
{
	return ( pool_holder_ && parent_->has_audio( ) ) || audio_;
}

/// Set the image associated to the frame.
void frame_lazy::set_image( olib::openmedialib::ml::image_type_ptr image, bool decoded )
{
	frame_type::set_image( image, decoded );
	evaluated_ = eval_image | eval_stream;
	pool_holder_.reset();
	if( parent_ )
		parent_->set_image( image, decoded );
}

/// Get the image associated to the frame.
olib::openmedialib::ml::image_type_ptr frame_lazy::get_image( )
{
	if( !( evaluated_ & eval_image ) ) evaluate( eval_image );
	return image_;
}

/// Set the audio associated to the frame.
void frame_lazy::set_audio( audio_type_ptr audio, bool decoded )
{
	frame_type::set_audio( audio, decoded );
	//We need to set the audio object on the inner frame as well to avoid
	//our own audio object being overwritten incorrectly on an evaluate() call
	if( parent_ )
		parent_->set_audio( audio, decoded );
}

/// Get the audio associated to the frame.
audio_type_ptr frame_lazy::get_audio( )
{
	return audio_;
}

void frame_lazy::set_stream( stream_type_ptr stream )
{
	frame_type::set_stream( stream );
	if( parent_ )
		parent_->set_stream( stream );
}

stream_type_ptr frame_lazy::get_stream( )
{
	if( !( evaluated_ & eval_stream ) ) evaluate( eval_stream );
	return stream_;
}

void frame_lazy::set_position( int position )
{
	frame_type::set_position( position );
}


frame_lazy::frame_lazy( const frame_lazy *org, const frame_type_ptr &other )
	: ml::frame_type( org )
	, parent_( other )
	, frames_( org->frames_ )
	, pool_holder_( org->pool_holder_ )
	, validated_( org->validated_ )
	, evaluated_( org->evaluated_ )
{
}

//########################################################################################

} } } }

