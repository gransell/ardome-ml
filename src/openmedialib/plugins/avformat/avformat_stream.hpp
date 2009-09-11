#ifndef AVFORMAT_STREAM_H_
#define AVFORMAT_STREAM_H_

#include <openmedialib/ml/packet.hpp>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
}

namespace olib { namespace openmedialib { namespace ml {

extern std::map< CodecID, std::string > codec_name_lookup_;
extern std::map< std::string, CodecID > name_codec_lookup_;

class stream_avformat : public ml::stream_type
{
	public:
		/// Constructor for a video packet
		stream_avformat( CodecID codec, size_t length, int position, int key, int bitrate, const dimensions &size, const fraction &sar )
			: ml::stream_type( )
			, id_( ml::stream_video )
			, length_( length )
			, data_( length + 32 )
			, position_( position )
			, key_( key )
			, bitrate_( bitrate )
			, size_( size )
			, sar_( sar )
			, frequency_( 0 )
			, channels_( 0 )
			, samples_( 0 )
		{
			if ( codec_name_lookup_.find( codec ) != codec_name_lookup_.end( ) )
				codec_ = codec_name_lookup_[ codec ];
		}

		/// Constructor for a audio packet
		stream_avformat( CodecID codec, size_t length, int position, int key, int bitrate, int frequency, int channels, int samples )
			: ml::stream_type( )
			, id_( ml::stream_audio )
			, length_( length )
			, data_( length + 32 )
			, position_( position )
			, key_( key )
			, bitrate_( bitrate )
			, size_( 0, 0 )
			, sar_( 1, 1 )
			, frequency_( frequency )
			, channels_( channels )
			, samples_( samples )
		{
			if ( codec_name_lookup_.find( codec ) != codec_name_lookup_.end( ) )
				codec_ = codec_name_lookup_[ codec ];
		}

		/// Virtual destructor
		virtual ~stream_avformat( ) 
		{ }

		/// Return the properties associated to this frame.
		virtual olib::openpluginlib::pcos::property_container &properties( )
		{
			return properties_;
		}

		/// Indicates the container format of the input
		virtual const std::string &container( ) const
		{
			return container_;
		}

		/// Indicates the codec used to decode the stream
		virtual const std::string &codec( ) const
		{
			return codec_;
		}

		/// Returns the id of the stream (so that we can select a codec to decode it)
		virtual const enum ml::stream_id id( ) const
		{
			return id_;
		}

		/// Returns the length of the data in the stream
		virtual size_t length( )
		{
			return length_;
		}

		/// Returns a pointer to the first byte of the stream
		virtual boost::uint8_t *bytes( )
		{
			return &data_[ 16 - boost::int64_t( &data_[ 0 ] ) % 16 ];
		}

		/// Returns the position of the key frame associated to this packet
		virtual const int key( ) const
		{
			return key_;
		}

		/// Returns the position of this packet
		virtual const int position( ) const
		{
			return position_;
		}

		/// Returns the bitrate of the stream this packet belongs to
		virtual const int bitrate( ) const
		{
			return bitrate_;
		}

		/// Returns the dimensions of the image associated to this packet (0,0 if n/a)
		virtual const dimensions size( ) const 
		{
			return size_;
		}

		/// Returns the sar of the image associated to this packet (1,1 if n/a)
		virtual const fraction sar( ) const 
		{ 
			return sar_;
		}

		/// Returns the frequency associated to the audio in the packet (0 if n/a)
		virtual const int frequency( ) const 
		{ 
			return frequency_; 
		}

		/// Returns the channels associated to the audio in the packet (0 if n/a)
		virtual const int channels( ) const 
		{ 
			return channels_; 
		}

		/// Returns the samples associated to the audio in the packet (0 if n/a)
		virtual const int samples( ) const 
		{ 
			return samples_; 
		}

	private:
		enum ml::stream_id id_;
		std::string container_;
		std::string codec_;
		size_t length_;
		std::vector < boost::uint8_t > data_;
		olib::openpluginlib::pcos::property_container properties_;
		int position_;
		int key_;
		int bitrate_;
		dimensions size_;
		fraction sar_;
		int frequency_;
		int channels_;
		int samples_;
};

} } }

#endif

