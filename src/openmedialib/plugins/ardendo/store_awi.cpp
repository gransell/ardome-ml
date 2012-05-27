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
#include <openmedialib/ml/packet.hpp>
#include <openmedialib/ml/awi.hpp>

extern "C" {
#include <libavformat/url.h>
}

namespace ml = olib::openmedialib::ml;
namespace pl = olib::openpluginlib;

namespace aml { namespace openmedialib {

static const pl::pcos::key key_source_byte_offset_ = pcos::key::from_string( "source_byte_offset" );
static const pl::pcos::key key_picture_coding_type_ = pcos::key::from_string( "picture_coding_type" );

class ML_PLUGIN_DECLSPEC store_awi : public ml::store_type
{
	public:
		store_awi( const pl::wstring &resource ) 
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
				ffurl_close( context_ );
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
				result = ffurl_open( &context_, pl::to_string( resource_ ).c_str( ), AVIO_FLAG_WRITE, 0, 0 ) >= 0;

			if ( result )
			{
				count_ ++;

				ml::stream_type_ptr stream = frame->get_stream( );

				pl::pcos::property coding_type = stream->properties( ).get_property_with_key( key_picture_coding_type_ );
				pl::pcos::property byte_offset = stream->properties( ).get_property_with_key( key_source_byte_offset_ );

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
				ffurl_write( context_, ( unsigned char * )( &buffer[ 0 ] ), buffer.size( ) );
			}
		}

		virtual ml::frame_type_ptr flush( )
		{ return ml::frame_type_ptr( ); }

		virtual void complete( )
		{ }

	protected:
		pl::wstring resource_;
		URLContext *context_;
		ml::awi_generator_v3 index_;
		int count_;
		boost::int64_t size_;
};

ml::store_type_ptr ML_PLUGIN_DECLSPEC create_store_awi( const pl::wstring &resource )
{
	return ml::store_type_ptr( new store_awi( resource ) );
}

} }
