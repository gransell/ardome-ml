
#include <iostream>
#include "frame.hpp"
#include <deque>

namespace pcos = olib::openpluginlib::pcos;
namespace il = olib::openimagelib::il;

namespace olib { namespace openmedialib { namespace ml {

frame_type::frame_type( ) 
	: properties_( )
	, image_( )
	, alpha_( )
	, audio_( )
	, pts_( 0.0 )
	, position_( 0 )
	, duration_( 0.0 )
	, sar_num_( 1 )
	, sar_den_( 1 )
	, fps_num_( 25 )
	, fps_den_( 1 )
	, queue_( )
{ }

frame_type::~frame_type( ) 
{ }

frame_type_ptr frame_type::shallow_copy( const frame_type_ptr &other )
{
	frame_type_ptr result;
	if ( other )
	{
		frame_type *copy = new frame_type( );
		std::auto_ptr< pcos::property_container > clone( other->properties_.clone() );
		copy->properties_ = *clone.get( );
		copy->image_ = other->image_;
		copy->alpha_ = other->alpha_;
		if ( other->audio_ )
		{
			typedef audio< unsigned char, pcm16 > pcm16;
			audio_type *aud = new audio_type( pcm16( other->audio_->frequency( ), other->audio_->channels( ), other->audio_->samples( ) ) );
			memcpy( aud->data( ), other->audio_->data( ), aud->size( ) );
			copy->audio_ = ml::audio_type_ptr( aud );
		}
		copy->pts_ = other->pts_;
		copy->position_ = other->position_;
		copy->duration_ = other->duration_;
		copy->sar_num_ = other->sar_num_;
		copy->sar_den_ = other->sar_den_;
		copy->fps_num_ = other->fps_num_;
		copy->fps_den_ = other->fps_den_;
		for ( std::deque< ml::frame_type_ptr >::iterator iter = other->queue_.begin( ); iter != other->queue_.end( ) ; iter ++ )
			copy->queue_.push_back( shallow_copy( *iter ) );
		copy->exceptions_ = other->exceptions_;
		result = frame_type_ptr( copy );
	}
	return result;
}

frame_type_ptr frame_type::deep_copy( const frame_type_ptr &other )
{
	frame_type_ptr result;
	if ( other )
	{
		frame_type *copy = new frame_type( );
		std::auto_ptr< pcos::property_container > clone( other->properties_.clone() );
		copy->properties_ = *clone.get( );
		if ( other->image_ )
			copy->image_ = il::image_type_ptr( other->image_->clone( ) );
		if ( other->alpha_ )
			copy->alpha_ = il::image_type_ptr( other->alpha_->clone( ) );
		if ( other->audio_ )
		{
			typedef audio< unsigned char, pcm16 > pcm16;
			audio_type *aud = new audio_type( pcm16( other->audio_->frequency( ), other->audio_->channels( ), other->audio_->samples( ) ) );
			memcpy( aud->data( ), other->audio_->data( ), aud->size( ) );
			copy->audio_ = ml::audio_type_ptr( aud );
		}
		copy->pts_ = other->pts_;
		copy->position_ = other->position_;
		copy->duration_ = other->duration_;
		copy->sar_num_ = other->sar_num_;
		copy->sar_den_ = other->sar_den_;
		copy->fps_num_ = other->fps_num_;
		copy->fps_den_ = other->fps_den_;
		for ( std::deque< ml::frame_type_ptr >::iterator iter = other->queue_.begin( ); iter != other->queue_.end( ) ; iter ++ )
			copy->queue_.push_back( shallow_copy( *iter ) );
		copy->exceptions_ = other->exceptions_;
		result = frame_type_ptr( copy );
	}
	return result;
}

pcos::property_container frame_type::properties( ) { return properties_; }
pcos::property frame_type::property( const char *name ) const { return properties_.get_property_with_string( name ); }

void frame_type::set_image( il::image_type_ptr image ) { image_ = image; }
il::image_type_ptr frame_type::get_image( ) { return image_; }
void frame_type::set_alpha( il::image_type_ptr image ) { alpha_ = image; }
il::image_type_ptr frame_type::get_alpha( ) { return alpha_; }
void frame_type::set_audio( audio_type_ptr audio ) { audio_ = audio; }
audio_type_ptr frame_type::get_audio( ) { return audio_; }
void frame_type::set_pts( double pts ) { pts_ = pts; }
double frame_type::get_pts( ) const { return pts_; }
void frame_type::set_position( int position ) { position_ = position; }
int frame_type::get_position( ) const { return position_; }
void frame_type::set_duration( double duration ) { duration_ = duration; }
double frame_type::get_duration( ) const { return duration_; }
void frame_type::set_sar( int num, int den ) { sar_num_ = num; sar_den_ = den; }
void frame_type::get_sar( int &num, int &den ) const { num = sar_num_; den = sar_den_; }
void frame_type::set_fps( int num, int den ) { fps_num_ = num; fps_den_ = den; }
void frame_type::get_fps( int &num, int &den ) const { num = fps_num_; den = fps_den_; }

int frame_type::get_fps_num( ) { int num, den; get_fps( num, den ); return num; }
int frame_type::get_fps_den( ) { int num, den; get_fps( num, den ); return den; }
int frame_type::get_sar_num( ) { int num, den; get_sar( num, den ); return num; }
int frame_type::get_sar_den( ) { int num, den; get_sar( num, den ); return den; }

double frame_type::aspect_ratio( )
{
	int num, den;
	get_sar( num, den );
	if ( num == 0 )
		return ( double )get_image( )->width( ) / get_image( )->height( );
	else
		return ( ( double ) num / den ) * ( ( double )get_image( )->width( ) / get_image( )->height( ) );
}

double frame_type::fps( ) const
{
	int num, den;
	get_fps( num, den );
	return den != 0 ? double( num ) / double( den ) : 1;
}

double frame_type::sar( ) const
{
	int num, den;
	get_sar( num, den );
	return den != 0 ? double( num ) / double( den ) : 1;
}

void frame_type::push( frame_type_ptr frame )
{
	if ( frame )
		queue_.push_back( frame );
}

frame_type_ptr frame_type::pop( )
{
	frame_type_ptr result;

	if ( queue_.size( ) )
	{
		result = queue_[ queue_.size( ) - 1 ]->pop( );
		if ( !result )
		{
			result = queue_[ queue_.size( ) - 1 ];
			queue_.erase( -- queue_.end( ) );
		}
	}

	return result;
}

std::deque< frame_type_ptr > frame_type::unfold( frame_type_ptr frame )
{
	std::deque< frame_type_ptr > queue;

	if ( frame )
	{
		frame_type_ptr f = frame->pop( );

		while( f )
		{
			queue.push_front( f );
			f = frame->pop( );
		}

		queue.push_front( frame );
	}

	return queue;
}

frame_type_ptr frame_type::fold( std::deque< frame_type_ptr > &queue )
{
	frame_type_ptr result;

	if ( queue.size( ) )
	{
		result = queue[ 0 ];
		queue.pop_front( );
		for ( std::deque< frame_type_ptr >::iterator iter = queue.begin( ); iter != queue.end( ); iter ++ )
			result->push( *iter );
	}

	return result;
}

void frame_type::push_exception( exception_ptr exception, input_type_ptr input )
{ 
	exceptions_.push_back( exception_item( exception, input ) ); 
}

exception_list frame_type::exceptions( ) const
{ 
	return exceptions_; 
}

const bool frame_type::in_error( ) const
{ 
	return exceptions_.size( ) != 0; 
}

} } }
