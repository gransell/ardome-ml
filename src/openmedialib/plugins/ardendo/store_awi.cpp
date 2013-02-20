// An AWI Store
//
// Copyright (C) 2012 Ardendo
// Released under the LGPL.
//
// #store:awi:
// 
// Generates an awi from a packet stream.

#include "precompiled_headers.hpp"
#include "amf_filter_plugin.hpp"
#include <openmedialib/ml/stream.hpp>
#include <openmedialib/ml/awi.hpp>
#include <openmedialib/ml/keys.hpp>
#include <openmedialib/ml/io.hpp>

namespace ml = olib::openmedialib::ml;
namespace io = ml::io;
namespace pl = olib::openpluginlib;

namespace aml { namespace openmedialib {

class ML_PLUGIN_DECLSPEC store_awi : public ml::store_type
{
	public:
		store_awi( const std::wstring &resource ) 
			: ml::store_type( )
			, resource_( resource )
			, context_( 0 )
			, index_( )
			, count_( 0 )
			, size_( 0 )
		{
		}

		virtual ~store_awi( )
		{ 
			if ( context_ )
			{
				index_.close( count_, size_ );
				write_index( );
				ml::io::close_file( context_ );
			}
		}

		bool init( )
		{
			return true;
		}

		virtual bool push( ml::frame_type_ptr frame )
		{
			bool result = frame && frame->get_stream( );

			if ( result && !context_ )
				result = ml::io::open_file( &context_, olib::opencorelib::str_util::to_string( resource_ ).c_str( ), AVIO_FLAG_WRITE | AVIO_FLAG_DIRECT ) >= 0;

			if ( result )
			{
				count_ ++;

				ml::stream_type_ptr stream = frame->get_stream( );

				pl::pcos::property coding_type = stream->properties( ).get_property_with_key( ml::keys::picture_coding_type );
				pl::pcos::property byte_offset = stream->properties( ).get_property_with_key( ml::keys::source_byte_offset );

				if ( !byte_offset.valid( ) )
					byte_offset = stream->properties( ).get_property_with_key( ml::keys::offset );

				result = coding_type.valid( ) && byte_offset.valid( );

				if ( result )
				{
					size_ = byte_offset.value< boost::int64_t >( );

					if ( coding_type.value< int >( ) == 1 )
					{
						index_.enroll( frame->get_position( ), size_ );
						write_index( );
					}

					size_ += stream->length( );
				}
			}

			return result;
		}

		void write_index( )
		{
			if ( context_ )
			{
				std::vector< boost::uint8_t > buffer;
				index_.flush( buffer );
				avio_write( context_, ( unsigned char * )( &buffer[ 0 ] ), buffer.size( ) );
				avio_flush( context_ );
			}
		}

		virtual ml::frame_type_ptr flush( )
		{ return ml::frame_type_ptr( ); }

		virtual void complete( )
		{ }

	protected:
		std::wstring resource_;
		AVIOContext *context_;
		ml::awi_generator_v3 index_;
		int count_;
		boost::int64_t size_;
};

ml::store_type_ptr ML_PLUGIN_DECLSPEC create_store_awi( const std::wstring &resource )
{
	return ml::store_type_ptr( new store_awi( resource ) );
}

} }
