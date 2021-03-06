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

// A struct to allow all frame position information to be stored
struct awi_detail
{
	awi_detail( ) { offset = 0; length = 0; }
	awi_detail( boost::int64_t o, boost::int32_t l ) { offset = o; length = l; }
	boost::int64_t offset;
	boost::int32_t length; 
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

/// Header record - specifies version and date of creation
struct awi_header_v3
{
	char id[ 3 ];
	char ver[ 1 ];
	boost::int32_t created;
	char wrapper[ 4 ];
	boost::uint16_t video_type;
	boost::uint8_t video_progressive;
	boost::uint8_t video_flags;
	boost::uint16_t video_fps_num;
	boost::uint16_t video_fps_den;
	boost::uint32_t video_bitrate;
	boost::uint16_t video_width;
	boost::uint16_t video_height;
	boost::uint32_t video_chroma;
	boost::uint16_t video_gop;
	boost::uint16_t video_rpp;
	boost::uint16_t video_ar_num;
	boost::uint16_t video_ar_den;
	boost::uint8_t video_sar_num;
	boost::uint8_t video_sar_den;
	char reserved1[ 6 ];
	boost::uint16_t audio_type;
	boost::uint16_t audio_channels;
	boost::uint16_t audio_bits;
	boost::uint16_t audio_store_bits;
	boost::uint32_t audio_frequency;
	char reserved2[ 4 ];
};

/// Current (v3) item
struct awi_item_v3
{
	boost::int16_t type;
	boost::int16_t frames;
	boost::int32_t frame;
	boost::int64_t offset;
	boost::int32_t length; 
};

/// Current (v3) footer
struct awi_footer_v3
{
	boost::int16_t type;
	char reserved1[ 6 ];
	boost::int32_t closed;
	boost::int16_t max_gop;
	boost::int16_t footer_flags;
	char id[ 3 ];
	char ver[ 1 ];
};

struct awi_header_v4
{
	boost::uint16_t type;
	char id[ 3 ];
	char ver[ 1 ];
	boost::int32_t created;
	char reserved[ 10 ]; //Pad out to match the item size
};

struct awi_item_v4
{
	boost::uint16_t type;
	boost::int16_t frames;
	boost::int32_t frame;
	boost::int64_t offset;
	boost::int32_t length; 
};

struct awi_footer_v4
{
	boost::uint16_t type;
	boost::int32_t closed;
	char id[ 3 ];
	char ver[ 1 ];
	char reserved[ 10 ]; //Pad out to match the item size
};

const boost::uint16_t AWI_V4_TYPE_HEADER = 0;
const boost::uint16_t AWI_V4_TYPE_FOOTER = 65535;
const boost::uint16_t AWI_V4_TYPE_VIDEO = 1;
//Audio streams are numbered from 2 and up
const boost::uint16_t AWI_V4_TYPE_AUDIO_FIRST = 2;
const boost::uint16_t AWI_V4_TYPE_AUDIO_LAST = AWI_V4_TYPE_AUDIO_FIRST + 15;

/// Parsing state enumeration
namespace  awi_state
{
	enum type {
		header,
		item,
		footer,
		error
	};
}

class ML_DECLSPEC awi_index
{
	public:
		awi_index( )
		: state_( awi_state::header )
		{
		}

		virtual ~awi_index( ) { }
		virtual bool finished( ) = 0;
		virtual boost::int64_t find( int position ) = 0;
		virtual int frames( int current ) = 0;
		virtual boost::int64_t bytes( ) = 0;
		virtual int calculate( boost::int64_t size ) = 0;
		virtual bool usable( ) = 0;
		virtual void set_usable( bool value ) = 0;
		virtual int key_frame_of( int position ) = 0;
		virtual int key_frame_from( boost::int64_t offset ) = 0;

		// Method to obtain the length of a packet
		virtual int length( int position ) const = 0;

		// Method to obtain the offset of a packet
		virtual boost::int64_t offset( int position ) const = 0;

		// AWI V4 introduces a audio and video indexing - an instance of the index can be used for a single component
		// and for backward compatability reasons, we need to know which type the index represents
		virtual const boost::uint16_t type( ) const { return 0; }

		// This should be reimplemented on the parser implementations
		virtual void read( ) { }

		// We need to be able to determine if the index is valid, particularly in a parser instance and after a read
		// NB: this might not report itself sanely in the case where the index is an empty file (ie: no header available)
		// and it's difficult to know if that's a case of 'not yet' or 'never'
		virtual bool valid( ) { return state_ != awi_state::error; }

		// This should be implemented in all subclasses to report the total number of frames which the index knows about
		// NB: this should *never* be used to determine the number of frames in the media unless both index derived file
		// size and open data file size match - it should only be used to supplement the valid method in the case of 
		// verifying a particular parser instance for use (eg: valid( ) && total_frames( ) > 0 - with that, we can be
		// sure that the the awi index matches and we should have some frames to work with.
		virtual int total_frames( ) const = 0;
 
		// Methods for querying and setting index information
		virtual bool has_index_data( ) const { return false; }
		virtual bool get_index_data( const std::string &, boost::uint8_t & ) const { return false; }
		virtual bool get_index_data( const std::string &, boost::uint16_t & ) const { return false; }
		virtual bool get_index_data( const std::string &, boost::uint32_t & ) const { return false; }
		virtual bool set_index_data( const std::string &, const boost::uint8_t ) { return false; }
		virtual bool set_index_data( const std::string &, const boost::uint16_t ) { return false; }
		virtual bool set_index_data( const std::string &, const boost::uint32_t ) { return false; }

	protected:
		awi_state::type state_;
};

/// Index holder class - currently assumes v2. Allows look ups by position ->
/// file offset and file offset -> position.

class ML_DECLSPEC awi_index_v2 : public awi_index
{
	public:
		awi_index_v2( );
		virtual ~awi_index_v2( );
		void set( const awi_header &value );
		void set( const awi_item &value );
		void set( const awi_footer &value );
		virtual bool finished( );
		virtual boost::int64_t find( int position );
		virtual int frames( int current );
		virtual boost::int64_t bytes( );
		virtual int calculate( boost::int64_t size );
		virtual bool usable( );
		virtual void set_usable( bool value );
		virtual int key_frame_of( int position );
		virtual int key_frame_from( boost::int64_t offset );
		virtual int total_frames( ) const;
		virtual int length( int position ) const;
		virtual boost::int64_t offset( int position ) const;

	protected:
		mutable boost::recursive_mutex mutex_;
		int position_;
		bool eof_;
		int frames_;
		bool usable_;

		awi_header header_;
		std::map< boost::int32_t, awi_item > items_;
		std::map< boost::int64_t, awi_item > offsets_;
		std::map< boost::int32_t, awi_detail > details_;
		awi_footer footer_;
};

/// Parser for a binary buffers of index data - the physical reading from a
/// file/url is left to the user of the parser - it only needs to read 
/// contiguous chunks (arbitary size) from a source and pass this to the 
/// parse method.

class ML_DECLSPEC awi_parser_v2 : public awi_index_v2
{
	public:
		awi_parser_v2( );
		virtual ~awi_parser_v2( );
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
};

/// Generator for an index - as with the parser, physical storage is left
/// to the user of the generator to implement - the user needs to 'enroll'
/// each key frame and its offset and close the generator when it reaches the
/// end of the file. The pending data to be written is extracted via the flush 
/// method (which it can do periodically, or after each enroll and the close
/// call to support growing file use).

class ML_DECLSPEC awi_generator_v2 : public awi_index_v2
{
	public:
		awi_generator_v2( );
		/// Enroll a position and offset into the index. The length field should only be used
		/// if you will know the length for all positions enrolled in the index.
		bool enroll( boost::int32_t position, boost::int64_t offset, boost::int32_t length = 0 );
		// Optionally, all frames can be added via the detail method (used by the offset and 
		// length methods)
		bool detail( boost::int32_t position, boost::int64_t offset, boost::int32_t length );
		bool close( boost::int32_t position, boost::int64_t offset );
		bool flush( std::vector< boost::uint8_t > &buffer );

	private:
		std::map< boost::int32_t, awi_item >::iterator current_;
		size_t flushed_;
		boost::int32_t position_;
		boost::int64_t offset_;
		bool known_packet_length_;
};

/// Index holder class - currently assumes v2. Allows look ups by position ->
/// file offset and file offset -> position.

class ML_DECLSPEC awi_index_v3 : public awi_index
{
	public:
		awi_index_v3( );
		virtual ~awi_index_v3( );
		void set( const awi_header_v3 &value );
		void set( const awi_item_v3 &value );
		void set( const awi_footer_v3 &value );
		virtual bool finished( );
		virtual boost::int64_t find( int position );
		virtual int frames( int current );
		virtual boost::int64_t bytes( );
		virtual int calculate( boost::int64_t size );
		virtual bool usable( );
		virtual void set_usable( bool value );
		virtual int key_frame_of( int position );
		virtual int key_frame_from( boost::int64_t offset );
		virtual int total_frames( ) const;
		virtual int length( int position ) const;
		virtual boost::int64_t offset( int position ) const;

		virtual bool has_index_data( ) const;
		virtual bool get_index_data( const std::string &, boost::uint8_t & ) const;
		virtual bool get_index_data( const std::string &, boost::uint16_t & ) const;
		virtual bool get_index_data( const std::string &, boost::uint32_t & ) const;
		virtual bool set_index_data( const std::string &, const boost::uint8_t );
		virtual bool set_index_data( const std::string &, const boost::uint16_t );
		virtual bool set_index_data( const std::string &, const boost::uint32_t );

	protected:
		mutable boost::recursive_mutex mutex_;
		int position_;
		bool eof_;
		int frames_;
		bool usable_;

		awi_header_v3 header_;
		std::map< boost::int32_t, awi_item_v3 > items_;
		std::map< boost::int64_t, awi_item_v3 > offsets_;
		std::map< boost::int32_t, awi_detail > details_;
		awi_footer_v3 footer_;
};

/// Parser for a binary buffers of index data - the physical reading from a
/// file/url is left to the user of the parser - it only needs to read 
/// contiguous chunks (arbitary size) from a source and pass this to the 
/// parse method.

class ML_DECLSPEC awi_parser_v3 : public awi_index_v3
{
	public:
		awi_parser_v3( );
		virtual ~awi_parser_v3( );
		bool parse( const boost::uint8_t *data, const size_t length );

	private:
		void append( const boost::uint8_t *data, const size_t length );
		bool parse_header( );
		bool is_item( );
		bool parse_item( );
		bool parse_footer( );
		bool read( char *data, size_t length );
		bool read( boost::uint8_t &value );
		bool read( boost::int16_t &value );
		bool read( boost::uint16_t &value );
		bool read( boost::int32_t &value );
		bool read( boost::uint32_t &value );
		bool read( boost::int64_t &value );
		bool peek( boost::int16_t &value );

		std::vector< boost::uint8_t > buffer_;
};

/// Generator for an index - as with the parser, physical storage is left
/// to the user of the generator to implement - the user needs to 'enroll'
/// each key frame and its offset and close the generator when it reaches the
/// end of the file. The pending data to be written is extracted via the flush 
/// method (which it can do periodically, or after each enroll and the close
/// call to support growing file use).

class ML_DECLSPEC awi_generator_v3 : public awi_index_v3
{
	public:
		awi_generator_v3( );
		/// Enroll a position and offset into the index. The length field should only be used
		/// if you will know the length for all positions enrolled in the index.
		bool enroll( boost::int32_t position, boost::int64_t offset, boost::int32_t length = 0 );
		// Optionally, all frames can be added via the detail method (used by the offset and 
		// length methods)
		bool detail( boost::int32_t position, boost::int64_t offset, boost::int32_t length );
		bool close( boost::int32_t position, boost::int64_t offset );
		bool flush( std::vector< boost::uint8_t > &buffer );

	private:
		std::map< boost::int32_t, awi_item_v3 >::iterator current_;
		std::map< boost::int32_t, awi_item_v3 >::iterator current_audio_;
		size_t flushed_;
		boost::int32_t position_;
		boost::int64_t offset_;
		bool known_packet_length_;
};

class ML_DECLSPEC awi_index_v4 : public awi_index
{
	public:
		awi_index_v4( boost::uint16_t entry_type );
		virtual ~awi_index_v4( );
		void set( const awi_header_v4 &value );
		void set( const awi_item_v4 &value );
		void set( const awi_footer_v4 &value );
		virtual bool finished( );
		virtual boost::int64_t find( int position );
		virtual int frames( int current );
		virtual boost::int64_t bytes( );
		virtual int calculate( boost::int64_t size );
		virtual bool usable( );
		virtual void set_usable( bool value );
		virtual int key_frame_of( int position );
		virtual int key_frame_from( boost::int64_t offset );
		virtual int total_frames( ) const;
		virtual const boost::uint16_t type( ) const { return entry_type_; }
		virtual int length( int position ) const;
		virtual boost::int64_t offset( int position ) const;

	protected:
		mutable boost::recursive_mutex mutex_;
		int position_;
		bool eof_;
		int frames_;
		bool usable_;

		awi_header_v4 header_;
		std::map< boost::int32_t, awi_item_v4 > items_;
		std::map< boost::int64_t, awi_item_v4 > offsets_;
		std::map< boost::int32_t, awi_detail > details_;
		awi_footer_v4 footer_;
		boost::uint16_t entry_type_;
};

class ML_DECLSPEC awi_parser_v4 : public awi_index_v4
{
	public:
		awi_parser_v4( boost::uint16_t entry_type_to_read );
		virtual ~awi_parser_v4( );
		bool parse( const boost::uint8_t *data, const size_t length );

	private:
		void append( const boost::uint8_t *data, const size_t length );
		bool entry_is_type( boost::uint16_t type );
		bool parse_header( );
		bool parse_item( );
		bool parse_footer( );
		bool read( char *data, size_t length );
		bool read( boost::int16_t &value );
		bool read( boost::uint16_t &value );
		bool read( boost::int32_t &value );
		bool read( boost::int64_t &value );
		bool peek( boost::uint16_t &value );
		void skip( boost::uint16_t bytes );

		std::vector< boost::uint8_t > buffer_;
		boost::uint16_t entry_type_to_read_;
};

class ML_DECLSPEC awi_generator_v4 : public awi_index_v4
{
	public:
		awi_generator_v4( boost::uint16_t type_to_write, bool complete = true );
		/// Enroll a position and offset into the index. The length field should only be used
		/// if you will know the length for all positions enrolled in the index.
		/// this is used to locate key frames, so only I-frames should be enrolled
		bool enroll( boost::int32_t position, boost::int64_t offset, boost::int32_t length = 0 );
		// Optionally, all frames can be added via the detail method (used by the offset and 
		// length methods)
		bool detail( boost::int32_t position, boost::int64_t offset, boost::int32_t length );
		bool close( boost::int32_t position, boost::int64_t offset );
		bool flush( std::vector< boost::uint8_t > &buffer );

	private:
		std::map< boost::int32_t, awi_item_v4 >::iterator current_;
		size_t flushed_;
		boost::int32_t position_;
		boost::int64_t offset_;
		boost::uint16_t type_to_write_;
		bool complete_;
		bool known_packet_length_;
};

} } }

#endif
