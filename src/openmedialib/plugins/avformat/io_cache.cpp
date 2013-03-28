// Basic AVIO extended functionality for stream handling

// Copyright (C) 2012 Vizrt
// Released under the LGPL.

/// This source file provides buffered access to non-seekable, non-cached media
/// inputs such as streams which are delivered in a realtime manner via udp, 
/// tcp, http, rstp or others.
///
/// In the general case of reading such streams, the io mechanism used is 
/// liable to lose data, especially immediately after the initialisation of 
/// the input which typically works like:
///
/// * read a block of data
/// * analyse data
/// * demux, decode and deliver frames until the block is depleted
/// * request more data and repeat
///
/// Depending on how long the the first 3 steps, the 4th may occur some time
/// after the data has been dispatched and in many of these specific cases, 
/// there is no way to obtain those missing packets.
///
/// This structure works around the issue by continuously reading from the 
/// source url and delivering blocks of data into a ring structure and 
/// susbquently wraps the ring via the AVIO api provided by ffmpeg to provide 
/// sequential read and close functionality.
///
/// To trigger the usage of this functionality, it is sufficient to decorate
/// the url in the following manner:
///
/// <aml-demuxer>:ring:<url>
///
/// For example, to provide access to a url provided by a Vu+ Duo satellite
/// receiever via an avformat input:
///
/// avformat:ring:http://bm750.local:8001/1:0:19:1B1D:802:2:11A0000:0:0:0:
///
/// NOTE: this does not provide support for cached file reading since the 
/// reading will stop once the queue structure is full - it is only
/// applicable for realtime sources and it assumes realtime processing of
/// those sources.
///
/// TODO: The queue must be provided with a maximum size, block size and 
/// timeout associated to the read. These three parameters control how large
/// the queue is allowed to grow, how much data is requested in each read 
/// of the source and how long the AVIO will wait for new packets in the case
/// that it depletes the queue. These are hard coded as 4096, 32768 and 250ms
/// respectively, but should be configurable through the decorated url.

#include <openmedialib/ml/io.hpp>
#include <opencorelib/cl/lru.hpp>
#include <boost/thread.hpp>
#include <boost/cstdint.hpp>
#include <deque>
#include <iostream>

extern "C" {
#include <libavutil/avstring.h>
}

namespace cl = olib::opencorelib;
namespace ml = olib::openmedialib::ml;

namespace olib { namespace openmedialib { namespace ml { namespace ring {

// All data is initially read into blocks
typedef std::vector< boost::uint8_t > block_type;

// For convenience, these are wrapped as shared pointers
typedef boost::shared_ptr< block_type > block_type_ptr;

/// The queue_type class provides the basic interface through which the comms
/// reader and the avio context implementations communicate. 
///
/// The comms reader uses acquire_empty to obtain an empty buffer, reads into
/// it and returns a full buffer via return_full.
///
/// The avio context acquires a full buffer via acquire_full and returns it
/// for subsequent re-use via the return_empty method.
///
/// Implementations of the queue_type can determine the nature of the queue
/// involved - ie: for real time, an acquire_empty should fail when the 
/// full count reaches a user specified upper limit - for non-realtime
/// the acquire_empty can block until a subsequent return_empty occurs from 
/// the avio context.

class queue_type
{
	public:
		/// Called by the comms reader to acquire an empty block
		/**
		The implementation is responsible for deciding when a new block should
		be created, whether a previously allocated one should be reused or if 
		a null object should be returned. 
		@return an allocated block or a null block if processing should stop
		*/
		virtual block_type_ptr acquire_empty( ) = 0;

		/// Called by the comms reader after it has filled a previously
		/// acquired empty block
		/**
		@param block A filled block sized to the number of bytes read
		*/
		virtual void return_full( block_type_ptr block ) = 0;

		/// Called by the avio context to acquire a block of data
		/**
		@return a previously returned full block or null to indicate eof
		*/
		virtual block_type_ptr acquire_full( ) = 0;

		/// Called by the avio context to return a block for reuse
		/**
		@param block A block that is no longer required
		*/
		virtual void return_empty( block_type_ptr block ) = 0;
};

/// The realtime implementation of the queue.
///
/// The upper limit on the number of blocks allocated, the block size and a
/// timeout relating to how long the the avio context will wait for a full 
/// block are passed in the constructor.
///
/// acquire_empty will return a null block when the number of allocated blocks
/// exceeds the upper limit.
///
/// Should the comms reader determine that an eof has occurred, it will return
/// a 0 sized full block and terminate reading.

class rt_queue : public queue_type
{
	public:
		// Read blocks are stored in an lru which is keyed by the absolute byte offset
		typedef cl::lru< boost::int64_t, block_type_ptr > lru_block_type;

		/// Constructor takes the upper size of the queue, size of each block
		/// and the timeout
		/**
		@param max_blocks The maximum number of empty blocks allocated
		@param block_size The number of bytes allocated in each block
		@param wait The maximum amount of time avio will wait for a full block
		*/
		rt_queue( size_t max_blocks = 4096, size_t block_size = 32768, size_t wait = 250 )
		: max_blocks_( max_blocks )
		, block_size_( block_size )
		, wait_( 1000 )
		, count_( 0 )
		, read_key_( 0 )
		, write_key_( 0 )
		, full_( max_blocks_ )
		{
		}

		/// Provides an empty block for the comms reader or a null block if the
		/// number of previously allocated blocks exceeds the specified size
		block_type_ptr acquire_empty( )
		{
			boost::recursive_mutex::scoped_lock lock( mutex_ );
			block_type_ptr result;
			if ( empty_.size( ) )
			{
				result = empty_.front( );
				empty_.pop_front( );
			}
			else if ( count_ < int( max_blocks_ ) )
			{
				result = block_type_ptr( new block_type( ) );
				result->resize( block_size_ );
				count_ ++;
			}
			if ( result ) result->resize( block_size_ );
			return result;
		}

		/// Returns a full block to the queue - a size of 0 indicates eof
		void return_full( block_type_ptr block )
		{
			full_.append( write_key_, block );
			write_key_ += block->size( );
		}

		/// Acquires a full block from the queue - a null block inidictaes a timeout
		/// and a block with size 0 indicates eof
		block_type_ptr acquire_full( )
		{
			block_type_ptr result = full_.find( read_key_, boost::posix_time::milliseconds( wait_ ) );
			read_key_ += result ? result->size( ) : 0;
			return result;
		}

		/// Returns a block for reuse
		void return_empty( block_type_ptr block )
		{
			boost::recursive_mutex::scoped_lock lock( mutex_ );
			empty_.push_back( block );
		}

	private:
		boost::recursive_mutex mutex_;
		size_t max_blocks_;
		size_t block_size_;
		size_t wait_;
		int count_;
		boost::int64_t read_key_;
		boost::int64_t write_key_;
		std::deque< block_type_ptr > empty_;
		lru_block_type full_;
};

/// The comms input reader.

class in
{
	public:
		in( const std::string url, queue_type &queue )
		: url_( url )
		, queue_( queue )
		, context_( 0 )
		{
		}

		~in( )
		{
			close( );
		}

		// Open the url
		bool open( int flags )
		{
			return context_ == 0 && ml::io::open_file( &context_, url_.c_str( ), flags ) == 0;
		}

		// Read a block
		int job( )
		{
			int result = 0;
			if ( context_ )
			{
				block_type_ptr block = queue_.acquire_empty( );
				if ( block )
				{
					result = avio_read( context_, &( ( *block )[ 0 ] ), block->size( ) );
					block->resize( result > 0 ? result : 0 );
					queue_.return_full( block );
				}
			}
			return result;
		}

		// Close the url
		void close( )
		{
			ml::io::close_file( context_ );
			context_ = 0;
		}

	private:
		const std::string url_;
		queue_type &queue_;
		AVIOContext *context_;
};

// 
class thread
{
	public:
		thread( in &reader )
		: mutex_( )
		, reader_( reader )
		, running_( false )
		{
		}

		~thread( )
		{
			stop( );
		}

		// Start the thread
		bool start( )
		{
			if ( !running( ) )
			{
				set_running( true );
				thread_ = boost::thread( boost::bind( &thread::run, this ) );
			}
			return running( );
		}

		// Stop the thread
		void stop( )
		{
			set_running( false );
			if ( thread_.joinable( ) )
				thread_.join( );
		}

		// Used to determine if the thread is still running
		bool running( )
		{
			boost::recursive_mutex::scoped_lock lock( mutex_ );
			return running_;
		}

	protected:

		// Change the running state
		void set_running( bool state )
		{
			boost::recursive_mutex::scoped_lock lock( mutex_ );
			running_ = state;
		}

		// Keep reading until told otherwise or a failure occurs
		void run( )
		{
			while( running( ) )
				if ( !reader_.job( ) ) break;
			reader_.close( );
		}

	private:
		boost::recursive_mutex mutex_;
		boost::thread thread_;
		in &reader_;
		bool running_;
};

// ring_type structure used by the avio wrapper
typedef struct ring_type
{
	ring_type( const char *url )
	: queue_( )
	, reader_( url, queue_ )
	, thread_( reader_ )
	, offset_( 0 )
	{
	}

	int read( unsigned char *buf, int size )
	{
		block_type_ptr block = current_ ? current_ : queue_.acquire_full( );
		int result = size;
		int offset = offset_;
		int left_in_block = block ? int( block->size( ) ) - offset : 0;

		if ( block && left_in_block <= size )
		{
			result = left_in_block;
			memcpy( buf, &( ( *block )[ offset ] ), result );
			current_ = block_type_ptr( );
			offset_ = 0;
		}
		else if ( block && block->size( ) )
		{
			memcpy( buf, &( ( *block )[ offset ] ), result );
			current_ = block;
			offset_ += result;
			block = block_type_ptr( );
		}
		else
		{
			result = 0;
		}

		if ( block )
			queue_.return_empty( block );

		return result;
	}

	rt_queue queue_;
	in reader_;
	thread thread_;
	block_type_ptr current_;
	int offset_;
	int flags_;
}
ring_type;

// Open the ring. The open accepts only read access and it will attempt to
// read the first block of data prior to starting the background thread. This
// is to ensure that a minimal amount of data is acquired prior to starting
// the thread, thus allowing a (relatively) short timeout on the the block
// read in the thread.
static int ring_open( void **context, const char *url, int flags )
{
	int result = AVERROR( EIO );

	av_strstart( url, "ring:", &url );

	*context = 0;

	if ( flags && AVIO_FLAG_READ )
	{
		ring_type *ring = new ring_type( url );

		if ( ring->reader_.open( flags ) )
		{
			result = ring->reader_.job( );
			if ( result > 0 && ring->thread_.start( ) )
			{
				result = 0;
				*context = ring;
			}
		}

		if ( result < 0 )
			delete ring;
	}

	return result;
}

// Read a block of data
static int ring_read( void *context, unsigned char *buf, int size )
{
	ring_type *ring = ( ring_type * )context;
	return ring ? ring->read( buf, size ) : -1;
}

// Close the ring
static int ring_close( void *context )
{
	ring_type *ring = ( ring_type * )context;
	delete ring;
	return 0;
}

void ML_PLUGIN_DECLSPEC register_protocol( void )
{
	ml::io::protocol_funcs ring_protocol =
	{
		ring_open,
		ring_close,
		ring_read,
		NULL, // Non-writable
		NULL  // Non-seekable
	};

	// Register the protocol
	ml::io::register_protocol_handler( "ring", ring_protocol );
}

} } } }
