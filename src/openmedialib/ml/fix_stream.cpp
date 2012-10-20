// ml - A media library representation.

// Copyright (C) 2011 Vizrt
// Released under the LGPL.

#include <openmedialib/ml/fix_stream.hpp>
#include <opencorelib/cl/special_folders.hpp>

namespace cl = olib::opencorelib;
namespace pl = olib::openpluginlib;
namespace il = olib::openimagelib::il;

namespace olib { namespace openmedialib { namespace ml {

namespace fixers
{
	// Stores a stream header
	typedef std::vector< char > header;

	// Ensures that the headers are destroyed automatically
	typedef boost::shared_ptr< header > header_ptr;

	// Provides stream copy and join functionality
	class fix_stream_type : public ml::stream_type
	{
		public:
			fix_stream_type( const header_ptr &header, const stream_type_ptr &stream )
				: ml::stream_type( )
				, id_( stream->id( ) )
				, container_( stream->container() )
				, codec_( stream->codec() )
				, length_( header->size( ) + stream->length( ) - 6 )
				, data_( length_ + 32 )
				, position_( stream->position( ) )
				, key_( stream->key( ) )
				, bitrate_( stream->bitrate( ) )
				, size_( stream->size( ) )
				, sar_( stream->sar( ) )
				, frequency_( stream->frequency( ) )
				, channels_( stream->channels( ) )
				, samples_( stream->samples( ) )
				, pf_( stream->pf( ) )
				, field_order_( stream->field_order( ) )
				, estimated_gop_size_( stream->estimated_gop_size( ) )
			{
				memcpy( bytes( ), &( *header )[ 0 ], header->size( ) );
				memcpy( bytes( ) + header->size( ), stream->bytes( ) + 6, stream->length( ) - 6 );
				std::auto_ptr< pl::pcos::property_container > clone( stream->properties( ).clone( ) );
				properties_ = *clone.get( );
			}

			/// Virtual destructor
			virtual ~fix_stream_type( ) { }

			/// Return the properties associated to this frame.
			virtual olib::openpluginlib::pcos::property_container &properties( ) { return properties_; }

			/// Indicates the container format of the input
			virtual const std::string &container( ) const { return container_; }

			/// Indicates the codec used to decode the stream
			virtual const std::string &codec( ) const { return codec_; }

			/// Returns the id of the stream (so that we can select a codec to decode it)
			virtual const enum ml::stream_id id( ) const { return id_; }

			/// Returns the length of the data in the stream
			virtual size_t length( ) { return length_; }

			/// Returns a pointer to the first byte of the stream
			virtual boost::uint8_t *bytes( ) { return &data_[ 16 - boost::int64_t( &data_[ 0 ] ) % 16 ]; }

			/// Returns the position of the key frame associated to this packet
			virtual const boost::int64_t key( ) const { return key_; }

			/// Returns the position of this packet
			virtual const boost::int64_t position( ) const { return position_; }

			/// Returns the bitrate of the stream this packet belongs to
			virtual const int bitrate( ) const { return bitrate_; }
		
			/// Gop size is currently unknown for avformat inputs
			virtual const int estimated_gop_size( ) const { return estimated_gop_size_; }

			/// Returns the dimensions of the image associated to this packet (0,0 if n/a)
			virtual const dimensions size( ) const { return size_; }

			/// Returns the sar of the image associated to this packet (1,1 if n/a)
			virtual const fraction sar( ) const { return sar_; }

			/// Returns the frequency associated to the audio in the packet (0 if n/a)
			virtual const int frequency( ) const { return frequency_; }

			/// Returns the channels associated to the audio in the packet (0 if n/a)
			virtual const int channels( ) const { return channels_; }

			/// Returns the samples associated to the audio in the packet (0 if n/a)
			virtual const int samples( ) const { return samples_; }

			/// Returns the picture format
			virtual const std::wstring pf( ) const { return pf_; }

			/// Returns the picture field order
			virtual olib::openimagelib::il::field_order_flags field_order( ) const { return field_order_; }
	
		private:
			enum ml::stream_id id_;
			std::string container_;
			std::string codec_;
			size_t length_;
			std::vector < boost::uint8_t > data_;
			olib::openpluginlib::pcos::property_container properties_;
			boost::int64_t position_;
			boost::int64_t key_;
			int bitrate_;
			dimensions size_;
			fraction sar_;
			int frequency_;
			int channels_;
			int samples_;
			std::wstring pf_;
			olib::openimagelib::il::field_order_flags field_order_;
			int estimated_gop_size_;
	};

	class avc
	{
		protected:
			// This is the order in which we will read the headers from the file
			typedef enum
			{
				AVCI50_720p_59_94,
				AVCI50_720p_50,
				AVCI50_1080i_29_97,
				AVCI50_1080p_29_97,
				AVCI50_1080i_25,
				AVCI50_1080p_25,
				AVCI100_720p_59_94,
				AVCI100_720p_50,
				AVCI100_1080i_29_97,
				AVCI100_1080p_29_97,
				AVCI100_1080i_25,
				AVCI100_1080p_25,
				AVCI100_1080p_24,
				AVCI50_1080p_24,
				AVCI100_720p_29_97,
				AVCI50_720p_29_97,
				AVCI100_720p_24,
				AVCI50_720p_24,
				AVCI100_720p_25,
				AVCI50_720p_25,
				UNKNOWN
			}
			type;

			// Defines the lookup table type
			typedef std::map< type, header_ptr > table_type;

			// Holds the size of each packet
			typedef std::map< type, size_t > size_type;

			// The table instance
			table_type table_;

			// The size look ups
			size_type size_;

			// Constructor loads the table
			avc( const olib::t_path &path )
			{
				std::ifstream stream;

				int id = 0;
				stream.open( path.string( ).c_str( ), std::ios::in | std::ios::binary );

				ARENFORCE_MSG( stream.good( ), "Unable to open %1%" )( path.string( ).c_str( ) );

				while( stream.good( ) )
				{
					header_ptr block( new header( 518 ) );
					stream.read( &( *block )[ 0 ], block->size( ) );
					if ( stream.good( ) )
						table_[ type( id ++ ) ] = block;
				}

				ARENFORCE_MSG( type( id ) == UNKNOWN, "Unable to parse %1% - should be %2% blocks of 518 bytes" )( path.string( ).c_str( ) )( UNKNOWN );

				size_[ AVCI50_720p_59_94   ] = 116736;
				size_[ AVCI50_720p_50      ] = 140800;
				size_[ AVCI50_1080i_29_97  ] = 232960;
				size_[ AVCI50_1080p_29_97  ] = 232960;
				size_[ AVCI50_1080i_25     ] = 281088;
				size_[ AVCI50_1080p_25     ] = 281088;
				size_[ AVCI100_720p_59_94  ] = 236544;
				size_[ AVCI100_720p_50     ] = 284672;
				size_[ AVCI100_1080i_29_97 ] = 472576;
				size_[ AVCI100_1080p_29_97 ] = 472576;
				size_[ AVCI100_1080i_25    ] = 568832;
				size_[ AVCI100_1080p_25    ] = 568832;
				size_[ AVCI100_1080p_24    ] = 472576;
				size_[ AVCI50_1080p_24     ] = 232960;
				size_[ AVCI100_720p_29_97  ] = 236544;
				size_[ AVCI50_720p_29_97   ] = 116736;
				size_[ AVCI100_720p_24     ] = 236544;
				size_[ AVCI50_720p_24      ] = 116736;
				size_[ AVCI100_720p_25     ] = 284672;
				size_[ AVCI50_720p_25      ] = 140800;
			}

		public:
			// Fixes the avci frames as required
			void fix( frame_type_ptr &frame, bool overwrite ) const
			{
				type id = UNKNOWN;
				stream_type_ptr stream = frame->get_stream( );
				int n, d;
				frame->get_fps( n, d );
				int w = frame->width( );
				int h = frame->height( );
				int b = stream->bitrate( );

				bool p = ( stream->field_order( ) == il::progressive );

				// TODO: Confirm that this is accurate enough
				bool is50 = b <= 75000000;

				if      ( is50  && w == 960  && h == 720  && n == 60000 && d == 1001 && p  ) id = AVCI50_720p_59_94;
				else if ( is50  && w == 960  && h == 720  && n == 50    && d == 1    && p  ) id = AVCI50_720p_50;
				else if ( is50  && w == 1440 && h == 1080 && n == 30000 && d == 1001 && !p ) id = AVCI50_1080i_29_97;
				else if ( is50  && w == 1440 && h == 1080 && n == 30000 && d == 1001 && p  ) id = AVCI50_1080p_29_97;
				else if ( is50  && w == 1440 && h == 1080 && n == 25    && d == 1    && !p ) id = AVCI50_1080i_25;
				else if ( is50  && w == 1440 && h == 1080 && n == 25    && d == 1    && p  ) id = AVCI50_1080p_25;
				else if ( is50  && w == 1440 && h == 1080 && n == 24000 && d == 1001 && p  ) id = AVCI50_1080p_24;
				else if ( is50  && w == 960  && h == 720  && n == 30000 && d == 1001 && p  ) id = AVCI50_720p_29_97;
				else if ( is50  && w == 960  && h == 720  && n == 24000 && d == 1001 && p  ) id = AVCI50_720p_24;
				else if ( is50  && w == 960  && h == 720  && n == 25    && d == 1    && p  ) id = AVCI50_720p_25;
				else if ( !is50 && w == 1280 && h == 720  && n == 60000 && d == 1001 && p  ) id = AVCI100_720p_59_94;
				else if ( !is50 && w == 1280 && h == 720  && n == 50    && d == 1    && p  ) id = AVCI100_720p_50;
				else if ( !is50 && w == 1920 && h == 1080 && n == 30000 && d == 1001 && !p ) id = AVCI100_1080i_29_97;
				else if ( !is50 && w == 1920 && h == 1080 && n == 30000 && d == 1001 && p  ) id = AVCI100_1080p_29_97;
				else if ( !is50 && w == 1920 && h == 1080 && n == 25    && d == 1    && !p ) id = AVCI100_1080i_25;
				else if ( !is50 && w == 1920 && h == 1080 && n == 25    && d == 1    && p  ) id = AVCI100_1080p_25;
				else if ( !is50 && w == 1920 && h == 1080 && n == 24000 && d == 1001 && p  ) id = AVCI100_1080p_24;
				else if ( !is50 && w == 1280 && h == 720  && n == 30000 && d == 1001 && p  ) id = AVCI100_720p_29_97;
				else if ( !is50 && w == 1280 && h == 720  && n == 24000 && d == 1001 && p  ) id = AVCI100_720p_24;
				else if ( !is50 && w == 1280 && h == 720  && n == 25    && d == 1    && p  ) id = AVCI100_720p_25;

				// Determine location in the table for this id
				table_type::const_iterator chunk = table_.find( id );
				size_type::const_iterator size = size_.find( id );

				ARENFORCE_MSG( chunk != table_.end( ), "Invalid AVCI packet %1%, %2%x%3% @ %4%:%5%, %6%" )( b )( w )( h )( n )( d )( p );

				// Modify the stream component if required
				if ( size->second - 512 == stream->length( ) )
					frame->set_stream( ml::stream_type_ptr( new fix_stream_type( chunk->second, stream ) ) );
				else if( overwrite && size->second == stream->length( ) )
					memcpy( frame->get_stream()->bytes( ), &( *chunk->second )[ 0 ], chunk->second->size( ) );

				ARENFORCE_MSG( size->second == frame->get_stream( )->length( ), "Invalid AVCI packet length %1% should be %2%" )( frame->get_stream( )->length( ) )( size->second );
			}

			// Singleton accessor
			static avc *get( )
			{
				boost::mutex::scoped_lock scoped_lock( mutex_ );
				if ( instance_ == 0 )
				{
					olib::t_path path = cl::special_folder::get( cl::special_folder::fix_stream ) / _CT( "sps_pps.avc" );
					instance_ = new avc( path );
					atexit( destroy );
				}
				return instance_;
			}

			// Invoked by atexit in the accessor above
			static void destroy( )
			{
				boost::mutex::scoped_lock scoped_lock( mutex_ );
				delete instance_;
				instance_ = 0;
			}

			static boost::mutex mutex_;
			static avc *instance_;
	};

	boost::mutex avc::mutex_;
	avc *avc::instance_ = 0;
}

// Accessor for the fix_stream functionality
ML_DECLSPEC void fix_stream( frame_type_ptr &frame, bool overwrite )
{
	if ( frame && frame->get_stream( ) )
	{
		stream_type_ptr stream = frame->get_stream( );

		if ( stream->codec( ) == "http://www.ardendo.com/apf/codec/avc/avc" )
			fixers::avc::get( )->fix( frame, overwrite );
	}
}

} } }

