// ml - A media library representation.

// Copyright (C) 2009 Ardendo
// Released under the LGPL.

#ifndef OPENMEDIALIB_AWI_INC_
#define OPENMEDIALIB_AWI_INC_

#include <openmedialib/ml/config.hpp>

#include <map>
#include <vector>

#include <boost/cstdint.hpp>
#include <boost/thread.hpp>

namespace olib { namespace openmedialib { namespace ml {

/// Header record - specifies version and date of creation
struct awi_header
{
	char id[ 3 ];
	char ver[ 1 ];
	boost::int32_t created;
};

/// Current (v2) item
struct awi_item
{
	boost::int16_t reserved;
	boost::int16_t frames;
	boost::int32_t frame;
	boost::int64_t offset;
	boost::int32_t length; 
};

/// Current (v2) footer
struct awi_footer
{
	boost::int64_t reserved;
	boost::int32_t closed;
	char id[ 3 ];
	char ver[ 1 ];
};

/// Proposed v3 footer - normalises the footer record to be the same length
/// as item/detail, and has the same logical means of identifying the type 
/// type = -1
struct awi_footer_v3
{
	boost::int16_t type;
	char id[ 3 ];
	char ver[ 1 ];
	boost::int32_t closed;
	char padding[ 10 ];
};

/// Parsing state enumeration
enum awi_state
{
	header,
	item,
	footer,
	error
};

/// Index holder class - currently assumes v2. Allows look ups by position ->
/// file offset and file offset -> position.

class ML_DECLSPEC awi_index
{
	public:
		awi_index( );
		void set( const awi_header &value );
		void set( const awi_item &value );
		void set( const awi_footer &value );
		bool finished( );
		boost::int64_t find( int position );
		int frames( int current );
		boost::int64_t bytes( );
		int calculate( boost::int64_t size );
		bool usable( );
		void set_usable( bool value );
		int key_frame_of( int position );
		int key_frame_from( boost::int64_t offset );

	protected:
		boost::recursive_mutex mutex_;
		int position_;
		bool eof_;
		int frames_;
		bool usable_;

		awi_header header_;
		std::map< boost::int32_t, awi_item > items_;
		std::map< boost::int64_t, awi_item > offsets_;
		awi_footer footer_;
};

/// Parser for a binary buffers of index data - the physical reading from a
/// file/url is left to the user of the parser - it only needs to read 
/// contiguous chunks (arbitary size) from a source and pass this to the 
/// parse method.

class ML_DECLSPEC awi_parser : public awi_index
{
	public:
		awi_parser( );
		bool parse( const boost::uint8_t *data, const size_t length );

	private:
		void append( const boost::uint8_t *data, const size_t length );
		bool parse_header( );
		bool is_item( );
		bool parse_item( );
		bool parse_footer( );
		bool read( char *data, size_t length );
		bool read( boost::int16_t &value );
		bool read( boost::int32_t &value );
		bool read( boost::int64_t &value );
		bool peek( boost::int16_t &value );

		std::vector< boost::uint8_t > buffer_;
		awi_state state_;
};

/// Generator for an index - as with the parser, physical storage is left
/// to the user of the generator to implement - the user needs to 'enroll'
/// each key frame and its offset and close the generator when it reaches the
/// end of the file. The pending data to be written is extracted via the flush 
/// method (which it can do periodically, or after each enroll and the close
/// call to support growing file use).

class ML_DECLSPEC awi_generator : public awi_index
{
	public:
		awi_generator( );
		bool enroll( boost::int32_t position, boost::int64_t offset );
		bool close( boost::int32_t position, boost::int64_t offset );
		bool flush( std::vector< boost::uint8_t > &buffer );

	private:
		awi_state state_;
		std::map< boost::int32_t, awi_item >::iterator current_;
		size_t flushed_;
		boost::int32_t position_;
		boost::int64_t offset_;
};

} } }

#endif
