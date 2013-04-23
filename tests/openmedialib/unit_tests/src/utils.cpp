#include "utils.hpp"

using namespace olib::openmedialib::ml;

stream_mock::stream_mock( std::string codec, size_t length, boost::int64_t position, boost::int64_t key, int bitrate,
	int frequency, int channels, int samples, int sample_size )
	: stream_type( )
	, codec_( codec )
	, id_( stream_audio )
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
}

stream_mock::~stream_mock( ) 
{ }

void stream_mock::set_position( boost::int64_t position )
{
	position_ = position;
}

void stream_mock::set_key( boost::int64_t key )
{
	key_ = key;
}

void stream_mock::set_index( size_t index )
{
	index_ = index;
}

size_t stream_mock::index( )
{
	return index_;
}

olib::openpluginlib::pcos::property_container &stream_mock::properties( )
{
	return properties_;
}

const std::string &stream_mock::container( ) const
{
	return container_;
}

const std::string &stream_mock::codec( ) const
{
	return codec_;
}

const enum stream_id stream_mock::id( ) const
{
	return id_;
}

size_t stream_mock::length( )
{
	return length_;
}

boost::uint8_t *stream_mock::bytes( )
{
	return &data_[ 16 - boost::int64_t( &data_[ 0 ] ) % 16 ];
}

const boost::int64_t stream_mock::key( ) const
{
	return key_;
}

const boost::int64_t stream_mock::position( ) const
{
	return position_;
}

const int stream_mock::bitrate( ) const
{
	return bitrate_;
}

const int stream_mock::estimated_gop_size( ) const
{ 
	return estimated_gop_size_;
}

const dimensions stream_mock::size( ) const 
{
	return size_;
}

const fraction stream_mock::sar( ) const 
{ 
	return sar_;
}

const int stream_mock::frequency( ) const 
{ 
	return frequency_; 
}

const int stream_mock::channels( ) const 
{ 
	return channels_; 
}

const int stream_mock::samples( ) const 
{ 
	return samples_; 
}

const int stream_mock::sample_size( ) const
{
	return sample_size_;
}

const std::wstring stream_mock::pf( ) const 
{ 
	return pf_; 
}

olib::openimagelib::il::field_order_flags stream_mock::field_order( ) const
{
	return field_order_;
}



