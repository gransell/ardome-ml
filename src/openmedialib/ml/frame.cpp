#include <iostream>
#include "frame.hpp"
#include "stream.hpp"
#include "audio_block.hpp"
#include <deque>
#include <boost/rational.hpp>

namespace pcos = olib::openpluginlib::pcos;


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
	, audio_block_( )
{ }

frame_type::frame_type( const frame_type *other )
	: stream_( other->stream_ )
	, image_( other->image_ )
	, alpha_( other->alpha_ )
	, audio_( other->audio_ )
	, pts_( other->pts_ )
	, position_( other->position_ )
	, duration_( other->duration_ )
	, sar_num_( other->sar_num_ )
	, sar_den_( other->sar_den_ )
	, fps_num_( other->fps_num_ )
	, fps_den_( other->fps_den_ )
	, exceptions_( other->exceptions_ )
	, audio_block_( other->audio_block_ )
{
	std::auto_ptr< pcos::property_container > clone( other->properties_.clone() );
	properties_ = *clone.get( );
	audio_ = other->audio_;
	for ( std::deque< ml::frame_type_ptr >::const_iterator iter = other->queue_.begin( ); iter != other->queue_.end( ) ; ++iter )
		queue_.push_back( ( *iter )->shallow( ) );
}

frame_type::~frame_type( ) 
{ }

frame_type_ptr frame_type::shallow( ) const
{
	return frame_type_ptr( new frame_type( this ) );
}

frame_type_ptr frame_type::deep( )
{
	frame_type_ptr result;
	frame_type *copy = new frame_type( this );
	std::auto_ptr< pcos::property_container > clone( properties_.clone() );
	copy->properties_ = *clone.get( );
	if ( image_ )
		copy->image_ = ml::image_type_ptr( image_->clone( ) );
	if ( alpha_ )
		copy->alpha_ = ml::image_type_ptr( alpha_->clone( ) );
	if ( audio_ )
		copy->audio_ = audio_->clone( );
	result = frame_type_ptr( copy );
	return result;
}

pcos::property_container &frame_type::properties( ) { return properties_; }
pcos::property frame_type::property( const char *name ) const { return properties_.get_property_with_string( name ); }

bool frame_type::has_image( )
{
	return image_ != ml::image_type_ptr( );
}

bool frame_type::has_alpha( ) const
{
	return alpha_ || image::pixfmt_has_alpha( ml_pixel_format( ) );
}

bool frame_type::has_audio( )
{
	return audio_ != audio_type_ptr( );
}

// Set the stream component on the frame
// Note: we extract the sar here to allow modification via the filter:sar mechanism
void frame_type::set_stream( stream_type_ptr stream ) 
{ 
	stream_ = stream; 
	if ( stream )
	{
		sar_num_ = stream->sar( ).num;
		sar_den_ = stream->sar( ).den;
	}
}

stream_type_ptr frame_type::get_stream( ) { return stream_; }

void frame_type::set_image( ml::image_type_ptr image, bool decoded )
{
	image_ = image;

	//Set sample aspect ratio from the image if valid
	if( image_ && image_->get_sar_num( ) != -1 )
	{
		sar_num_ = image_->get_sar_num( );
		sar_den_ = image_->get_sar_den( );
	}

	//Destroy the existing video stream, since it is not a correct
	//representation of the image anymore
	if( !decoded && image && stream_ && stream_->id() == stream_video )
	{
		stream_.reset();
	}
}

ml::image_type_ptr frame_type::get_image( ) 
{ 
	return image_; 
}

void frame_type::set_alpha( ml::image_type_ptr image ) { alpha_ = image; }
ml::image_type_ptr frame_type::get_alpha( ) { return alpha_; }

void frame_type::set_audio( audio_type_ptr audio, bool decoded )
{
	audio_ = audio;
	//Destroy the existing audio block, since it is not a correct
	//representation of the audio anymore (if the decoded flag is
	//set, the caller guarantees that the audio object is the decoded
	//result of the audio block).
	if( !decoded && audio )
	{
		audio_block_.reset();
	}
}

audio_type_ptr frame_type::get_audio( ) { return audio_; }
void frame_type::set_pts( double pts ) { pts_ = pts; }
double frame_type::get_pts( ) const { return pts_; }
void frame_type::set_position( int position ) { position_ = position; }
int frame_type::get_position( ) const { return position_; }
void frame_type::set_duration( double duration ) { duration_ = duration; }
double frame_type::get_duration( ) const { return duration_; }
void frame_type::set_sar( int num, int den ) { 
	sar_num_ = num; 
	sar_den_ = den; 
	if ( image_ ) 
	{
		image_->set_sar_num( num );
		image_->set_sar_den( den );
	}
}

void frame_type::get_sar( int &num, int &den ) const 
{ 
	num = sar_num_; den = sar_den_; 
}

void frame_type::set_fps( int num, int den ) 
{
	boost::rational< int > fps = boost::rational< int >( num, den );
	fps_num_ = fps.numerator( ); 
	fps_den_ = fps.denominator( ); 
}

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
		for ( std::deque< frame_type_ptr >::iterator iter = queue.begin( ); iter != queue.end( ); ++iter )
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

void frame_type::clear_exceptions( )
{
	if ( in_error( ) )
		exceptions_.erase( exceptions_.begin( ), exceptions_.end( ) );
}

/// Indicates the container format if known ("" if unknown or n/a)
std::string frame_type::container( ) const
{
	if ( stream_ )
	{
		return stream_->container( );
	}
	else if ( audio_block_ && !audio_block_->tracks.empty() )
	{
		//Use the first stream on the first track of the audio block
		const audio::track_type &trk = audio_block_->tracks.begin( )->second;
		if ( !trk.packets.empty( ) )
			return trk.packets.begin()->second->container( );
	}
	return "";
}

/// Indicates the input's video codec if known ("" if unknown or n/a)
std::string frame_type::video_codec( ) const
{
	if ( stream_ && stream_->id( ) == ml::stream_video )
		return stream_->codec( );
	return "";
}

int frame_type::width( ) const
{
	if ( image_ )
		return image_->width( );
	else if ( stream_ )
		return stream_->size( ).width;
	return 0;
}

int frame_type::height( ) const
{
	if ( image_ )
		return image_->height( );
	else if ( stream_ )
		return stream_->size( ).height;
	return 0;
}

image::MLPixelFormat frame_type::ml_pixel_format( ) const
{
	image::MLPixelFormat result = image::ML_PIX_FMT_NONE;
	if ( image_ )
		result = image_->ml_pixel_format( );
	else if ( stream_ )
		result = stream_->ml_pixel_format( );
	return result;
}

bool frame_type::is_yuv_planar( ) const
{
	return ml::image::is_pixfmt_planar( ml_pixel_format( ) );
}

t_string frame_type::pf( ) const
{
	if ( image_ )
		return image_->pf( );
	else if ( stream_ )
		return stream_->pf( );
	return _CT("");
}

ml::image::field_order_flags frame_type::field_order( ) const
{
	if ( image_ )
		return image_->field_order( );
	else if ( stream_ )
		return stream_->field_order( );
	return ml::image::top_field_first;
}

/// Indicates the input's audio codec if known ("" if unknown or n/a)
std::string frame_type::audio_codec( ) const
{
	if ( stream_ && stream_->id( ) == ml::stream_audio )
		return stream_->codec( );
	return "";
}

/// Obtain frequency from packet or audio
int frame_type::frequency( ) const
{
	if ( audio_ )
		return audio_->frequency( );
	else if ( stream_ )
		return stream_->frequency( );
	return 0;
}

/// Obtain channels from packet or audio
int frame_type::channels( ) const
{
	if ( audio_ )
		return audio_->channels( );
	else if ( stream_ )
		return stream_->channels( );
	return 0;
}

/// Obtain samples from packet or audio
int frame_type::samples( ) const
{
	if ( audio_ )
		return audio_->samples( );
	else if ( stream_ )
		return stream_->samples( );
	return 0;
}

} } }

