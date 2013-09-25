#ifndef AVFORMAT_STREAM_H_
#define AVFORMAT_STREAM_H_

#include <openmedialib/ml/stream.hpp>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
}

#include "utils.hpp"

namespace olib { namespace openmedialib { namespace ml {

class stream_avformat : public ml::stream_type
{
	public:
		/// Constructor for a video packet
		stream_avformat( AVCodecID codec, size_t length, boost::int64_t position, boost::int64_t key, int bitrate,
			const dimensions &size, const fraction &sar, const std::wstring& pf,
			olib::openimagelib::il::field_order_flags field_order, int estimated_gop_size )
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
			, sample_size_( 0 )
			, pf_( pf )
			, field_order_( field_order )
			, estimated_gop_size_( estimated_gop_size )
			, index_( 0 )
		{
			codec_ = avformat_codec_id_to_apf_codec( codec );
		}

		// Constructor with known codec name
		stream_avformat( std::string codec, size_t length, boost::int64_t position, boost::int64_t key, int bitrate,
			const dimensions &size, const fraction &sar, const std::wstring& pf,
			olib::openimagelib::il::field_order_flags field_order, int estimated_gop_size )
			: ml::stream_type( )
			, id_( ml::stream_video )
			, codec_( codec )
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
			, sample_size_( 0 )
			, pf_( pf )
			, field_order_( field_order )
			, estimated_gop_size_( estimated_gop_size )
			, index_( 0 )
		{
		}

		/// Constructor for a audio packet
		stream_avformat( AVCodecID codec, size_t length, boost::int64_t position, boost::int64_t key, int bitrate,
			int frequency, int channels, int samples, int sample_size )
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
			, sample_size_( sample_size )
			, pf_( L"" )
			, field_order_( olib::openimagelib::il::top_field_first )
			, estimated_gop_size_( 0 )
			, index_( 0 )
		{
			codec_ = avformat_codec_id_to_apf_codec( codec );
		}

		/// Virtual destructor
		virtual ~stream_avformat( ) 
		{ }

		void set_position( boost::int64_t position )
		{
			position_ = position;
		}

		void set_key( boost::int64_t key )
		{
			key_ = key;
		}

		void set_index( size_t index )
		{
			index_ = index;
		}

		size_t index( )
		{
			return index_;
		}

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
		virtual const boost::int64_t key( ) const
		{
			return key_;
		}

		/// Returns the position of this packet
		virtual const boost::int64_t position( ) const
		{
			return position_;
		}

		/// Returns the bitrate of the stream this packet belongs to
		virtual const int bitrate( ) const
		{
			return bitrate_;
		}
		
		/// Gop size is currently unknown for avformat inputs
		virtual const int estimated_gop_size( ) const
		{ 
			return estimated_gop_size_;
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

		/// Returns the sample size of the audio in the packet (0 if n/a)
		virtual const int sample_size( ) const
		{
			return sample_size_;
		}
	
		virtual const std::wstring pf( ) const 
		{ 
			return pf_; 
		}

		virtual olib::openimagelib::il::field_order_flags field_order( ) const
		{
			return field_order_;
		}

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
		int sample_size_;
		std::wstring pf_;
		olib::openimagelib::il::field_order_flags field_order_;
		int estimated_gop_size_;
		size_t index_;
};

} } }

#endif

