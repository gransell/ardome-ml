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
	boost::uint16_t video_rate;
    boost::uint16_t video_rate_rational;
	boost::uint32_t video_bitrate;
	boost::uint16_t video_width;
	boost::uint16_t video_height;
	boost::uint32_t video_chroma;
	boost::uint16_t video_gop;
	boost::uint16_t video_rrp;
	boost::uint16_t video_aspect;
	boost::uint16_t video_aspect_rational;
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

/// Parsing state enumeration
enum awi_state
{
	header,
	item,
	footer,
	error
};

class ML_DECLSPEC awi_index
{
	public:
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
        
        // Methods for querying and setting index information
//        virtual bool has_index_data( ) const { return false; }
//        
//        //virtual bool get_index_data( std::string key, boost::uint16_t &value ) { return false; }
//        virtual bool get_index_data( std::string key, boost::uint32_t &value ) const { return false; }
//        
//        //virtual bool set_index_data( std::string key, boost::uint16_t value ) { return false; }
//        virtual bool set_index_data( std::string key, boost::uint32_t value ) { return false; }
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
		awi_state state_;
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
        
        // Methods for querying and setting index information
//        virtual bool has_index_data( ) const { return true; }
//        
//        //virtual bool get_index_data( std::string key, boost::uint16_t &value ) { return false; }
//        virtual bool get_index_data( std::string key, boost::uint32_t &value ) const;
//        
//        //virtual bool set_index_data( std::string key, boost::uint16_t value ) { return false; }
//        virtual bool set_index_data( std::string key, boost::uint32_t value );
        
	protected:
		boost::recursive_mutex mutex_;
		int position_;
		bool eof_;
		int frames_;
		bool usable_;

		awi_header_v3 header_;
		std::map< boost::int32_t, awi_item_v3 > items_;
		std::map< boost::int64_t, awi_item_v3 > offsets_;
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
		awi_state state_;
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
		bool enroll( boost::int32_t position, boost::int64_t offset );
		bool close( boost::int32_t position, boost::int64_t offset );
		bool flush( std::vector< boost::uint8_t > &buffer );

	private:
		awi_state state_;
		std::map< boost::int32_t, awi_item_v3 >::iterator current_;
		size_t flushed_;
		boost::int32_t position_;
		boost::int64_t offset_;
};

} } }

#endif
