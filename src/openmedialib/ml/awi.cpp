// ml - A media library representation.

// Copyright (C) 2009 Ardendo
// Released under the LGPL.

#include "awi.hpp"

#define awi_header_size 8
#define awi_item_size 20
#define awi_footer_size 16

#define awi_header_size_v3 64
#define awi_item_size_v3 20
#define awi_footer_size_v3 20

namespace olib { namespace openmedialib { namespace ml {

namespace 
{
	void write( boost::uint8_t *&ptr, char *value, int len )
	{
		memcpy( ptr, value, len );
		ptr += len;
	}

	void write( boost::uint8_t *&ptr, boost::int16_t value )
	{
		*ptr ++ = boost::uint8_t( ( value >> 8 ) & 0xff );
		*ptr ++ = boost::uint8_t( value & 0xff );
	}

	void write( boost::uint8_t *&ptr, boost::int32_t value )
	{
		*ptr ++ = boost::uint8_t( ( value >> 24 ) & 0xff );
		*ptr ++ = boost::uint8_t( ( value >> 16 ) & 0xff );
		*ptr ++ = boost::uint8_t( ( value >> 8 ) & 0xff );
		*ptr ++ = boost::uint8_t( value & 0xff );
	}

	void write( boost::uint8_t *&ptr, boost::int64_t value )
	{
		*ptr ++ = boost::uint8_t( ( value >> 56 ) & 0xff );
		*ptr ++ = boost::uint8_t( ( value >> 48 ) & 0xff );
		*ptr ++ = boost::uint8_t( ( value >> 40 ) & 0xff );
		*ptr ++ = boost::uint8_t( ( value >> 32 ) & 0xff );
		*ptr ++ = boost::uint8_t( ( value >> 24 ) & 0xff );
		*ptr ++ = boost::uint8_t( ( value >> 16 ) & 0xff );
		*ptr ++ = boost::uint8_t( ( value >> 8 ) & 0xff );
		*ptr ++ = boost::uint8_t( value & 0xff );
	}
}

// AWI Index holder - provides a means to register awi header, items and footer
// records and subsequently query the index to allow quick determination of 
// I frames and GOPs.

awi_index_v2::awi_index_v2( )
	: awi_index( )
	, mutex_( )
	, position_( 0 )
	, eof_( false )
	, frames_( 0 )
	, usable_( true )
{
}
	
awi_index_v2::~awi_index_v2( )
{
}

// Set the header record
void awi_index_v2::set( const awi_header &value )
{
	boost::recursive_mutex::scoped_lock lock( mutex_ );
	header_ = value;
}

// Add an item record
void awi_index_v2::set( const awi_item &value )
{
	boost::recursive_mutex::scoped_lock lock( mutex_ );
	if ( value.length != 0 )
	{
		if ( items_.find( value.frame ) == items_.end( ) )
			frames_ += value.frames;
		items_[ value.frame ] = value;
		if ( offsets_.find( value.offset ) == offsets_.end( ) )
			offsets_[ value.offset ] = value;
	}
}

// Set the footer record
void awi_index_v2::set( const awi_footer &value )
{
	boost::recursive_mutex::scoped_lock lock( mutex_ );
	footer_ = value;
	eof_ = true;
}

// Indicates if the index is complete
bool awi_index_v2::finished( )
{
	boost::recursive_mutex::scoped_lock lock( mutex_ );
	return eof_;
}

// Determine the file offset of the GOP associated to the position
boost::int64_t awi_index_v2::find( int position )
{
	boost::recursive_mutex::scoped_lock lock( mutex_ );
	boost::int64_t byte = -1;

	if ( items_.size( ) )
	{
		std::map< boost::int32_t, awi_item >::iterator iter = items_.upper_bound( position );
		if ( iter != items_.begin( ) ) iter --;
		byte = ( *iter ).second.offset;
	}

	return byte;
}

// Approximate the number of frames in the file - the index may indicate a larger size than
// the currently available media, and we use the previous frame count [normally derived from the
// 'calculate' method based on the file size at openning] to ensure that we don't report an 
// approximation which is smaller.

int awi_index_v2::frames( int current )
{
	boost::recursive_mutex::scoped_lock lock( mutex_ );
	int result = frames_;

	// If the current value does not indicate that the index and data are in sync, then we want
	// to return a value which is greater than the current, and significantly less than the value
	// indicated by the index 
	if ( !eof_ && current < frames_ )
	{
		std::map< boost::int32_t, awi_item >::iterator iter = items_.upper_bound( frames_ - 100 );
		if ( iter != items_.begin( ) ) iter --;
		result = ( *iter ).first >= current ? ( *iter ).first + 1 : current + 1;
	}

	return result;
}

// Total number of bytes in the media that the index is aware of
boost::int64_t awi_index_v2::bytes( )
{
	boost::recursive_mutex::scoped_lock lock( mutex_ );
	std::map< boost::int32_t, awi_item >::iterator iter = -- items_.end( );
	return ( *iter ).second.offset;
}

// Derive the frame count from the file size given
int awi_index_v2::calculate( boost::int64_t size )
{
	boost::recursive_mutex::scoped_lock lock( mutex_ );
	int result = -1;
	if ( eof_ && size == bytes( ) )
	{
		result = frames_;
	}
	else if ( offsets_.size( ) )
	{
		std::map< boost::int64_t, awi_item >::iterator iter = offsets_.upper_bound( size );
		if ( iter != offsets_.begin( ) ) iter --;
		result = ( *iter ).second.frame + ( *iter ).second.frames - 1;
	}
	return result;
}

// Indicates if the index is usable (if we're writing multiple media files in parallel with 
// different aspects of the same content, we may want to only index one and use that index
// to derive frame counts in the other files - if so, use with care...)
bool awi_index_v2::usable( )
{
	boost::recursive_mutex::scoped_lock lock( mutex_ );
	return usable_;
}

// See notes on usable above
void awi_index_v2::set_usable( bool value )
{
	boost::recursive_mutex::scoped_lock lock( mutex_ );
	usable_ = value;
}

// Determine the I frame of the given position
int awi_index_v2::key_frame_of( int position )
{
	boost::recursive_mutex::scoped_lock lock( mutex_ );
	int result = -1;

	if ( items_.size( ) )
	{
		std::map< boost::int32_t, awi_item >::iterator iter = items_.upper_bound( position );
		if ( iter != items_.begin( ) ) iter --;
		result = ( *iter ).second.frame;
	}
	return result;
}

// Determine the I frame from the offset provided
int awi_index_v2::key_frame_from( boost::int64_t offset )
{
	boost::recursive_mutex::scoped_lock lock( mutex_ );
	int result = -1;
	if ( offsets_.size( ) )
	{
		std::map< boost::int64_t, awi_item >::iterator iter = offsets_.upper_bound( offset );
		if ( iter != offsets_.begin( ) ) iter --;
		result = ( *iter ).second.frame;
	}
	return result;
}

int awi_index_v2::total_frames( ) const
{
	boost::recursive_mutex::scoped_lock lock( mutex_ );
	return frames_;
}

// ----------------------------------------------------------------------------

// The parser is designed to convert a written file to a usable index object - 
// data is passed into the parser in arbitrary sized blocks of bytes (leaving 
// the acquisition of the data as a task for the user of the class to deal with).

awi_parser_v2::awi_parser_v2( ) 
	: awi_index_v2( )
{
}
	
awi_parser_v2::~awi_parser_v2( ) 
{
}
	
// Extract header, items and footer from the data passed. Any error in the data
// will result in a false return and all subsequent attempts to submit additional
// data will be rejected. Any incomplete record will be processed in conjunction
// with the next passed block.

bool awi_parser_v2::parse( const boost::uint8_t *data, const size_t length )
{
	bool result = state_ == header || state_ == item;

	// Append data to internal buffer
	if ( result )
		append( data, length );

	// We need to ensure that we have the minimum length of the smallest record
	while( result && buffer_.size( ) > 8 )
	{
		if ( ( is_item( ) && buffer_.size( ) < awi_item_size ) || ( !is_item( ) && buffer_.size( ) < awi_footer_size ) )
			break;

		switch( state_ )
		{
			case header:
				result = parse_header( );
				state_ = result ? item : error;
				break;

			case item:
				if ( buffer_.size( ) >= awi_item_size && is_item( ) )
				{
					result = parse_item( );
					state_ = result ? item : error;
				}
				else if ( buffer_.size( ) == awi_footer_size && !is_item( ) )
				{
					result = parse_footer( );
					state_ = result ? footer : error;
				}
				break;

			case footer:
			case error:
				result = false;
				break;
		}
	}

	return result;
}

// Append passed data into an internal buffer.

void awi_parser_v2::append( const boost::uint8_t *data, const size_t length )
{
	size_t current = buffer_.size( );
	buffer_.resize( current + length );
	memcpy( &buffer_[ current ], data, length );
}

// Attempt to read a header and remove it from the internal buffer

bool awi_parser_v2::parse_header( )
{
	bool result = false;

	awi_header header;
	result = read( header.id, 3 ) && read( header.ver, 1 ) && read( header.created );
	result &= !memcmp( header.id, "AWI", 3 ) && !memcmp( header.ver, "2", 1 );

	if ( result )
		set( header );

	return result;
}

// Indicates if the next record is an item.

bool awi_parser_v2::is_item( )
{
	awi_item item;
	return peek( item.reserved ) && item.reserved == 0;
}

// Attempt to parse an item and remove it from the internal buffer

bool awi_parser_v2::parse_item( )
{
	awi_item item;
	bool result = peek( item.reserved );

	if ( result && item.reserved == 0 )
		result = read( item.reserved ) && read( item.frames ) && read( item.frame ) && read( item.offset ) && read( item.length );

	if ( result )
		set( item );

	return result;
}

// Attempt to parse a footer and remove it from the internal buffer

bool awi_parser_v2::parse_footer( )
{
	bool result = false;

	awi_footer footer;
	result = read( footer.reserved ) && read( footer.closed ) && read( footer.id, 3 ) && read( footer.ver, 1 );
	result &= !memcmp( footer.id, "AWI", 3 ) && !memcmp( footer.ver, "2", 1 );

	if ( result )
		set( footer );

	return result;
}

// Read a fixed length character array from the internal buffer and remove it

bool awi_parser_v2::read( char *data, size_t length )
{
	bool result = buffer_.size( ) >= length;
	if ( result )
	{
		memcpy( data, &buffer_[ 0 ], length );
		buffer_.erase( buffer_.begin( ), buffer_.begin( ) + length );
	}
	return result;
}

// Read a 16 bit int from the internal buffer and remove it

bool awi_parser_v2::read( boost::int16_t &value )
{
	bool result = buffer_.size( ) >= 2;
	if ( result )
	{
		value = ( buffer_[ 0 ] << 8 ) | buffer_[ 1 ];
		buffer_.erase( buffer_.begin( ), buffer_.begin( ) + 2 );
	}
	return result;
}

// Read a 32 bit int from the internal buffer and remove it

bool awi_parser_v2::read( boost::int32_t &value )
{
	bool result = buffer_.size( ) >= 4;
	if ( result )
	{
		value = ( buffer_[ 0 ] << 24 ) | ( buffer_[ 1 ] << 16 ) | ( buffer_[ 2 ] << 8 ) | buffer_[ 3 ];
		buffer_.erase( buffer_.begin( ), buffer_.begin( ) + 4 );
	}
	return result;
}

// Read a 64 bit int from the internal buffer and remove it

bool awi_parser_v2::read( boost::int64_t &value )
{
	bool result = buffer_.size( ) >= 8;
	if ( result )
	{
		value = boost::int64_t( buffer_[ 0 ] ) << 56LL;
		value |= boost::int64_t( buffer_[ 1 ] ) << 48LL;
		value |= boost::int64_t( buffer_[ 2 ] ) << 40LL;
		value |= boost::int64_t( buffer_[ 3 ] ) << 32LL;
		value |= boost::int64_t( buffer_[ 4 ] ) << 24LL;
		value |= boost::int64_t( buffer_[ 5 ] ) << 16LL;
		value |= boost::int64_t( buffer_[ 6 ] ) << 8LL;
		value |= boost::int64_t( buffer_[ 7 ] );
		buffer_.erase( buffer_.begin( ), buffer_.begin( ) + 8 );
	}
	return result;
}

// Reads a 16 bit int from the buffer and leaves it in place

bool awi_parser_v2::peek( boost::int16_t &value )
{
	bool result = buffer_.size( ) >= 2;
	if ( result )
		value = ( buffer_[ 0 ] << 8 ) | buffer_[ 1 ];
	return result;
}

// ----------------------------------------------------------------------------

// The generator class provides a means to build an index (either during 
// transcoding or as part of an ingestion process).

awi_generator_v2::awi_generator_v2( )
	: awi_index_v2( )
	, current_( items_.begin( ) )
	, flushed_( 0 )
	, position_( -1 )
	, offset_( 0 )
{
	awi_header header;

	memcpy( header.id, "AWI", 3 );
	memcpy( header.ver, "2", 1 );
	header.created = 0;

	set( header );
}

// Enroll a key frame and its offset

bool awi_generator_v2::enroll( boost::int32_t position, boost::int64_t offset )
{
	bool result = state_ != error && position > position_ && offset >= offset_;

	if ( result )
	{
		if ( position_ == -1 )
		{
			position_ = position;
			offset_ = offset;
		}
		else if ( offset != offset_ )
		{
			bool refresh = false;
			awi_item item;

			if ( current_ == items_.end( ) )
				refresh = true;

			item.reserved = 0;
			item.frames = position - position_;
			item.frame = position_;
			item.offset = offset_;
			item.length = offset - offset_;
			set( item );

			if ( refresh )
				current_ = items_.end( ) != items_.begin( ) ?  -- items_.end( ) : items_.end( );

			position_ = position;
			offset_ = offset;
		}
	}
	else
	{
		state_ = error;
	}

	return result;
}

// Mark the generation as complete
bool awi_generator_v2::close( boost::int32_t position, boost::int64_t offset )
{
	bool result = state_ != error && position > position_ && offset >= offset_;

	if ( result )
	{
		result = enroll( position, offset );
		if ( result )
		{
			awi_footer footer;
			footer.reserved = -1;
			footer.closed = 0;
			memcpy( footer.id, "AWI", 3 );
			memcpy( footer.ver, "2", 1 );
			set( footer );
		}
	}
	else
	{
		state_ = error;
	}

	return result;
}

// Flush the pending data to the provided buffer
bool awi_generator_v2::flush( std::vector< boost::uint8_t > &buffer )
{
	bool result = state_ != error;

	if ( state_ != footer && result )
	{
		size_t size_h = state_ == header ? awi_header_size : 0;
		size_t size_i = ( items_.size( ) - flushed_ ) * awi_item_size;
		size_t size_f = ( finished( ) ? awi_footer_size : 0 );

		buffer.resize( size_h + size_i + size_f );

		boost::uint8_t *ptr = &buffer[ 0 ];

		if ( state_ == header )
		{
			write( ptr, header_.id, 3 );
			write( ptr, header_.ver, 1 );
			write( ptr, header_.created );
			state_ = item;
		}

		while( state_ == item && current_ != items_.end( ) )
		{
			awi_item item = ( current_ ++ )->second;
			write( ptr, item.reserved );
			write( ptr, item.frames );
			write( ptr, item.frame );
			write( ptr, item.offset );
			write( ptr, item.length );
			flushed_ ++;
		}

		if ( state_ != footer && finished( ) )
		{
			write( ptr, footer_.reserved );
			write( ptr, footer_.closed );
			write( ptr, footer_.id, 3 );
			write( ptr, footer_.ver, 1 );
			state_ = footer;
		}
	}

	return result;
}

// AWI Index holder - provides a means to register awi header, items and footer
// records and subsequently query the index to allow quick determination of 
// I frames and GOPs.

awi_index_v3::awi_index_v3( )
	: awi_index( )
	, mutex_( )
	, position_( 0 )
	, eof_( false )
	, frames_( 0 )
	, usable_( true )
{
}

	
awi_index_v3::~awi_index_v3( )
{
}
	
// Set the header record
void awi_index_v3::set( const awi_header_v3 &value )
{
	boost::recursive_mutex::scoped_lock lock( mutex_ );
	header_ = value;
}

// Add an item record
void awi_index_v3::set( const awi_item_v3 &value )
{
	boost::recursive_mutex::scoped_lock lock( mutex_ );
	if ( value.length != 0 )
	{
		if ( items_.find( value.frame ) == items_.end( ) )
			frames_ += value.frames;
		items_[ value.frame ] = value;
		if ( offsets_.find( value.offset ) == offsets_.end( ) )
			offsets_[ value.offset ] = value;
	}
}

// Set the footer record
void awi_index_v3::set( const awi_footer_v3 &value )
{
	boost::recursive_mutex::scoped_lock lock( mutex_ );
	footer_ = value;
	eof_ = true;
}

// Indicates if the index is complete
bool awi_index_v3::finished( )
{
	boost::recursive_mutex::scoped_lock lock( mutex_ );
	return eof_;
}

// Determine the file offset of the GOP associated to the position
boost::int64_t awi_index_v3::find( int position )
{
	boost::recursive_mutex::scoped_lock lock( mutex_ );
	boost::int64_t byte = -1;

	if ( items_.size( ) )
	{
		std::map< boost::int32_t, awi_item_v3 >::iterator iter = items_.upper_bound( position );
		if ( iter != items_.begin( ) ) iter --;
		byte = ( *iter ).second.offset;
	}

	return byte;
}

// Approximate the number of frames in the file - the index may indicate a larger size than
// the currently available media, and we use the previous frame count [normally derived from the
// 'calculate' method based on the file size at openning] to ensure that we don't report an 
// approximation which is smaller.

int awi_index_v3::frames( int current )
{
	boost::recursive_mutex::scoped_lock lock( mutex_ );
	int result = frames_;

	// If the current value does not indicate that the index and data are in sync, then we want
	// to return a value which is greater than the current, and significantly less than the value
	// indicated by the index 
	if ( !eof_ && current < frames_ )
	{
		std::map< boost::int32_t, awi_item_v3 >::iterator iter = items_.upper_bound( frames_ - 100 );
		if ( iter != items_.begin( ) ) iter --;
		result = ( *iter ).first >= current ? ( *iter ).first + 1 : current + 1;
	}

	return result;
}

// Total number of bytes in the media that the index is aware of
boost::int64_t awi_index_v3::bytes( )
{
	boost::recursive_mutex::scoped_lock lock( mutex_ );
	std::map< boost::int32_t, awi_item_v3 >::iterator iter = -- items_.end( );
	return ( *iter ).second.offset;
}

// Derive the frame count from the file size given
int awi_index_v3::calculate( boost::int64_t size )
{
	boost::recursive_mutex::scoped_lock lock( mutex_ );
	int result = -1;

	if ( eof_ && size == bytes( ) )
	{
		result = frames_;
	}
	else if ( offsets_.size( ) )
	{
		std::map< boost::int64_t, awi_item_v3 >::iterator iter = offsets_.upper_bound( size );
		if ( iter != offsets_.begin( ) ) iter --;
		result = ( *iter ).second.frame + ( *iter ).second.frames - 1;
	}
	return result;
}

// Indicates if the index is usable (if we're writing multiple media files in parallel with 
// different aspects of the same content, we may want to only index one and use that index
// to derive frame counts in the other files - if so, use with care...)
bool awi_index_v3::usable( )
{
	boost::recursive_mutex::scoped_lock lock( mutex_ );
	return usable_;
}

// See notes on usable above
void awi_index_v3::set_usable( bool value )
{
	boost::recursive_mutex::scoped_lock lock( mutex_ );
	usable_ = value;
}

// Determine the I frame of the given position
int awi_index_v3::key_frame_of( int position )
{
	boost::recursive_mutex::scoped_lock lock( mutex_ );
	int result = -1;

	if ( items_.size( ) )
	{
		std::map< boost::int32_t, awi_item_v3 >::iterator iter = items_.upper_bound( position );
		if ( iter != items_.begin( ) ) iter --;
		result = ( *iter ).second.frame;
	}
	return result;
}

// Determine the I frame from the offset provided
int awi_index_v3::key_frame_from( boost::int64_t offset )
{
	boost::recursive_mutex::scoped_lock lock( mutex_ );
	int result = -1;
	if ( offsets_.size( ) )
	{
		std::map< boost::int64_t, awi_item_v3 >::iterator iter = offsets_.upper_bound( offset );
		if ( iter != offsets_.begin( ) ) iter --;
		result = ( *iter ).second.frame;
	}
	return result;
}

int awi_index_v3::total_frames( ) const
{
	boost::recursive_mutex::scoped_lock lock( mutex_ );
	return frames_;
}

bool awi_index_v3::has_index_data( ) const
{
	return state_ != header && state_ != error;
}

bool awi_index_v3::get_index_data( const std::string &key, boost::uint8_t &value ) const
{
	if ( key == "video_progressive" ) value = header_.video_progressive;
	else if ( key == "video_flags" ) value = header_.video_flags;
	else if ( key == "video_sar_num" ) value = header_.video_sar_num;
	else if ( key == "video_sar_den" ) value = header_.video_sar_den;
	else return false;
	return true;
}

bool awi_index_v3::get_index_data( const std::string &key, boost::uint16_t &value ) const
{
	if ( key == "video_type" ) value = header_.video_type;
	else if ( key == "video_fps_num" ) value = header_.video_fps_num;
	else if ( key == "video_fps_den" ) value = header_.video_fps_den;
	else if ( key == "video_width" ) value = header_.video_width;
	else if ( key == "video_height" ) value = header_.video_height;
	else if ( key == "video_gop" ) value = header_.video_gop;
	else if ( key == "video_rpp" ) value = header_.video_rpp;
	else if ( key == "video_ar_num" ) value = header_.video_ar_num;
	else if ( key == "video_ar_den" ) value = header_.video_ar_den;
	else if ( key == "audio_type" ) value = header_.audio_type;
	else if ( key == "audio_channels" ) value = header_.audio_channels;
	else if ( key == "audio_bits" ) value = header_.audio_bits;
	else if ( key == "audio_store_bits" ) value = header_.audio_store_bits;
	else return false;
	return true;
}

bool awi_index_v3::get_index_data( const std::string &key, boost::uint32_t &value ) const
{
	if ( key == "video_bitrate" ) value = header_.video_bitrate;
	else if ( key == "video_chroma" ) value = header_.video_chroma;
	else if ( key == "audio_frequency" ) value = header_.audio_frequency;
	else return false;
	return true;
}

bool awi_index_v3::set_index_data( const std::string &key, const boost::uint8_t value )
{
	if ( key == "video_progressive" ) header_.video_progressive = value;
	else if ( key == "video_flags" ) header_.video_flags = value;
	else if ( key == "video_sar_num" ) header_.video_sar_num = value;
	else if ( key == "video_sar_den" ) header_.video_sar_den = value;
	else return false;
	return true;
}

bool awi_index_v3::set_index_data( const std::string &key, const boost::uint16_t value )
{
	if ( key == "video_type" ) header_.video_type = value;
	else if ( key == "video_fps_num" ) header_.video_fps_num = value;
	else if ( key == "video_fps_den" ) header_.video_fps_den = value;
	else if ( key == "video_width" ) header_.video_width = value;
	else if ( key == "video_height" ) header_.video_height = value;
	else if ( key == "video_gop" ) header_.video_gop = value;
	else if ( key == "video_rpp" ) header_.video_rpp = value;
	else if ( key == "video_ar_num" ) header_.video_ar_num = value;
	else if ( key == "video_ar_den" ) header_.video_ar_den = value;
	else if ( key == "audio_type" ) header_.audio_type = value;
	else if ( key == "audio_channels" ) header_.audio_channels = value;
	else if ( key == "audio_bits" ) header_.audio_bits = value;
	else if ( key == "audio_store_bits" ) header_.audio_store_bits = value;
	else return false;
	return true;
}

bool awi_index_v3::set_index_data( const std::string &key, const boost::uint32_t value )
{
	if ( key == "video_bitrate" ) header_.video_bitrate = value;
	else if ( key == "video_chroma" ) header_.video_chroma = value;
	else if ( key == "audio_frequency" ) header_.audio_frequency = value;
	else return false;
	return true;
}

// ----------------------------------------------------------------------------

// The parser is designed to convert a written file to a usable index object - 
// data is passed into the parser in arbitrary sized blocks of bytes (leaving 
// the acquisition of the data as a task for the user of the class to deal with).

awi_parser_v3::awi_parser_v3( ) 
	: awi_index_v3( )
{
}

	
awi_parser_v3::~awi_parser_v3( ) 
{
}
	
	
// Extract header, items and footer from the data passed. Any error in the data
// will result in a false return and all subsequent attempts to submit additional
// data will be rejected. Any incomplete record will be processed in conjunction
// with the next passed block.

bool awi_parser_v3::parse( const boost::uint8_t *data, const size_t length )
{
	bool result = state_ == header || state_ == item;

	// Append data to internal buffer
	if ( result )
		append( data, length );

	// We need to ensure that we have the minimum length of the smallest record
	while( result && buffer_.size( ) > 8 )
	{
		if ( ( is_item( ) && buffer_.size( ) < awi_item_size ) || ( !is_item( ) && buffer_.size( ) < awi_footer_size ) )
			break;

		switch( state_ )
		{
			case header:
				// Make sure that we have enough data for the header
				if ( buffer_.size( ) > awi_header_size_v3 )
				{
					result = parse_header( );
					state_ = result ? item : error;
				}
				else
				{
					// Need more data sp break the loop
					result = false;
				}
				break;

			case item:
				if ( buffer_.size( ) >= awi_item_size_v3 && is_item( ) )
				{
					result = parse_item( );
					state_ = result ? item : error;
				}
				else if ( buffer_.size( ) == awi_footer_size_v3 && !is_item( ) )
				{
					result = parse_footer( );
					state_ = result ? footer : error;
				}
				break;

			case footer:
			case error:
				result = false;
				break;
		}
	}

	return result;
}

// Append passed data into an internal buffer.

void awi_parser_v3::append( const boost::uint8_t *data, const size_t length )
{
	size_t current = buffer_.size( );
	buffer_.resize( current + length );
	memcpy( &buffer_[ current ], data, length );
}

// Attempt to read a header and remove it from the internal buffer

bool awi_parser_v3::parse_header( )
{
	bool result = false;

	awi_header_v3 header;
	result = read( header.id, 3 ) && read( header.ver, 1 ) && read( header.created );
	result &= !memcmp( header.id, "AWI", 3 ) && !memcmp( header.ver, "3", 1 );

	if ( result )
	{
		result = read( header.wrapper, 4 ) && 
			read( header.video_type ) &&
			read( header.video_progressive ) &&
			read( header.video_flags ) &&
			read( header.video_fps_num ) &&
			read( header.video_fps_den ) &&
			read( header.video_bitrate ) &&
			read( header.video_width ) &&
			read( header.video_height ) &&
			read( header.video_chroma ) &&
			read( header.video_gop ) &&
			read( header.video_rpp ) &&
			read( header.video_ar_num ) &&
			read( header.video_ar_den ) &&
			read( header.video_sar_num ) &&
			read( header.video_sar_den ) &&
			read( header.reserved1, 6 ) &&
			read( header.audio_type ) &&
			read( header.audio_channels ) &&
			read( header.audio_bits ) &&
			read( header.audio_store_bits ) &&
			read( header.audio_frequency ) &&
			read( header.reserved2, 4 );
		
		if ( result )
			set( header );
	}

	return result;
}

// Indicates if the next record is an item.

bool awi_parser_v3::is_item( )
{
	awi_item_v3 item;
	return peek( item.type ) && ( ( item.type & 0xfffc ) == 0);
}

// Attempt to parse an item and remove it from the internal buffer

bool awi_parser_v3::parse_item( )
{
	awi_item_v3 item;
	bool result = peek( item.type );

	if ( result && ( ( item.type & 0xfffc ) == 0) )
		result = read( item.type ) && read( item.frames ) && read( item.frame ) && read( item.offset ) && read( item.length );

	if ( result )
		set( item );

	return result;
}

// Attempt to parse a footer and remove it from the internal buffer

bool awi_parser_v3::parse_footer( )
{
	bool result = false;

	awi_footer_v3 footer;
	result = read( footer.type ) && read( footer.reserved1, 6 ) && read( footer.closed ) && read( footer.max_gop ) && read( footer.footer_flags ) && read( footer.id, 3 ) && read( footer.ver, 1 );
	result &= !memcmp( footer.id, "AWI", 3 ) && !memcmp( footer.ver, "3", 1 );

	if ( result )
		set( footer );

	return result;
}

// Read a fixed length character array from the internal buffer and remove it

bool awi_parser_v3::read( char *data, size_t length )
{
	bool result = buffer_.size( ) >= length;
	if ( result )
	{
		memcpy( data, &buffer_[ 0 ], length );
		buffer_.erase( buffer_.begin( ), buffer_.begin( ) + length );
	}
	return result;
}
	
bool awi_parser_v3::read( boost::uint8_t &value )
{
	char ret;
	if ( read( &ret, 1 ) )
	{
		value = static_cast<boost::uint8_t> ( ret );
		return true;
	}
	return false;
}

// Read a 16 bit int from the internal buffer and remove it

bool awi_parser_v3::read( boost::int16_t &value )
{
	bool result = buffer_.size( ) >= 2;
	if ( result )
	{
		value = ( buffer_[ 0 ] << 8 ) | buffer_[ 1 ];
		buffer_.erase( buffer_.begin( ), buffer_.begin( ) + 2 );
	}
	return result;
}

bool awi_parser_v3::read( boost::uint16_t &value )
{
	boost::int16_t ret;
	if ( read( ret ) )
	{
		value = static_cast<boost::uint16_t> ( ret );
		return true;
	}
	return false;
}
	
// Read a 32 bit int from the internal buffer and remove it

bool awi_parser_v3::read( boost::int32_t &value )
{
	bool result = buffer_.size( ) >= 4;
	if ( result )
	{
		value = ( buffer_[ 0 ] << 24 ) | ( buffer_[ 1 ] << 16 ) | ( buffer_[ 2 ] << 8 ) | buffer_[ 3 ];
		buffer_.erase( buffer_.begin( ), buffer_.begin( ) + 4 );
	}
	return result;
}

bool awi_parser_v3::read( boost::uint32_t &value )
{
	boost::int32_t ret;
	if ( read( ret ) )
	{
		value = static_cast<boost::uint32_t> ( ret );
		return true;
	}
	return false;
}
	
// Read a 64 bit int from the internal buffer and remove it

bool awi_parser_v3::read( boost::int64_t &value )
{
	bool result = buffer_.size( ) >= 8;
	if ( result )
	{
		value = boost::int64_t( buffer_[ 0 ] ) << 56LL;
		value |= boost::int64_t( buffer_[ 1 ] ) << 48LL;
		value |= boost::int64_t( buffer_[ 2 ] ) << 40LL;
		value |= boost::int64_t( buffer_[ 3 ] ) << 32LL;
		value |= boost::int64_t( buffer_[ 4 ] ) << 24LL;
		value |= boost::int64_t( buffer_[ 5 ] ) << 16LL;
		value |= boost::int64_t( buffer_[ 6 ] ) << 8LL;
		value |= boost::int64_t( buffer_[ 7 ] );
		buffer_.erase( buffer_.begin( ), buffer_.begin( ) + 8 );
	}
	return result;
}

// Reads a 16 bit int from the buffer and leaves it in place

bool awi_parser_v3::peek( boost::int16_t &value )
{
	bool result = buffer_.size( ) >= 2;
	if ( result )
		value = ( buffer_[ 0 ] << 8 ) | buffer_[ 1 ];
	return result;
}

// ----------------------------------------------------------------------------

// The generator class provides a means to build an index (either during 
// transcoding or as part of an ingestion process).

awi_generator_v3::awi_generator_v3( )
	: awi_index_v3( )
	, current_( items_.begin( ) )
	, flushed_( 0 )
	, position_( -1 )
	, offset_( 0 )
{
	awi_header_v3 header;

	memcpy( header.id, "AWI", 3 );
	memcpy( header.ver, "2", 1 );
	header.created = 0;

	set( header );
}

// Enroll a key frame and its offset

bool awi_generator_v3::enroll( boost::int32_t position, boost::int64_t offset )
{
	bool result = state_ != error && position > position_ && offset >= offset_;

	if ( result )
	{
		if ( position_ == -1 )
		{
			position_ = position;
			offset_ = offset;
		}
		else if ( offset != offset_ )
		{
			bool refresh = false;
			awi_item_v3 item;

			if ( current_ == items_.end( ) )
				refresh = true;

			item.type = 0;
			item.frames = position - position_;
			item.frame = position_;
			item.offset = offset_;
			item.length = offset - offset_;
			set( item );

			if ( refresh )
				current_ = items_.end( ) != items_.begin( ) ?  -- items_.end( ) : items_.end( );

			position_ = position;
			offset_ = offset;
		}
	}
	else
	{
		state_ = error;
	}

	return result;
}

// Mark the generation as complete
bool awi_generator_v3::close( boost::int32_t position, boost::int64_t offset )
{
	bool result = state_ != error && position > position_ && offset >= offset_;

	if ( result )
	{
		result = enroll( position, offset );
		if ( result )
		{
			awi_footer_v3 footer;
			footer.type = 0xffff;
			footer.closed = 0;
			memcpy( footer.id, "AWI", 3 );
			memcpy( footer.ver, "2", 1 );
			set( footer );
		}
	}
	else
	{
		state_ = error;
	}

	return result;
}

// Flush the pending data to the provided buffer
bool awi_generator_v3::flush( std::vector< boost::uint8_t > &buffer )
{
	bool result = state_ != error;

	if ( state_ != footer && result )
	{
		size_t size_h = state_ == header ? awi_header_size_v3 : 0;
		size_t size_i = ( items_.size( ) - flushed_ ) * awi_item_size_v3;
		size_t size_f = ( finished( ) ? awi_footer_size_v3 : 0 );

		buffer.resize( size_h + size_i + size_f );

		boost::uint8_t *ptr = &buffer[ 0 ];

		if ( state_ == header )
		{
			write( ptr, header_.id, 3 );
			write( ptr, header_.ver, 1 );
			write( ptr, header_.created );
			state_ = item;
		}

		while( state_ == item && current_ != items_.end( ) )
		{
			awi_item_v3 item = ( current_ ++ )->second;
			write( ptr, item.type );
			write( ptr, item.frames );
			write( ptr, item.frame );
			write( ptr, item.offset );
			write( ptr, item.length );
			flushed_ ++;
		}

		if ( state_ != footer && finished( ) )
		{
			write( ptr, footer_.type );
			write( ptr, footer_.closed );
			write( ptr, footer_.id, 3 );
			write( ptr, footer_.ver, 1 );
			state_ = footer;
		}
	}

	return result;
}

} } }

