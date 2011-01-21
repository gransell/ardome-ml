// decode - A decode plugin to ml.
//
// Copyright (C) 2010 Vizrt
// Released under the LGPL.

#include <openmedialib/ml/openmedialib_plugin.hpp>
#include <openmedialib/ml/packet.hpp>
#include <openpluginlib/pl/pcos/observer.hpp>
#include <opencorelib/cl/thread_pool.hpp>
#include <opencorelib/cl/function_job.hpp>
#include <opencorelib/cl/profile.hpp>
#include <opencorelib/cl/enforce_defines.hpp>
#include <opencorelib/cl/log_defines.hpp>
#include <opencorelib/cl/uuid_16b.hpp>
#include <opencorelib/cl/str_util.hpp>
#include <openmedialib/ml/filter_encode.hpp>

#include <boost/algorithm/string.hpp>
#include <boost/enable_shared_from_this.hpp>

#include <iostream>
#include <cstdlib>
#include <vector>
#include <string>
#include <set>

#define LOKI_CLASS_LEVEL_THREADING
#include <loki/Singleton.h>

namespace pl = olib::openpluginlib;
namespace ml = olib::openmedialib::ml;
namespace il = olib::openimagelib::il;
namespace pcos = olib::openpluginlib::pcos;
namespace cl = olib::opencorelib;

namespace olib { namespace openmedialib { namespace ml { namespace decode {

static pl::pcos::key key_length_ = pl::pcos::key::from_string( "length" );
static pl::pcos::key key_force_ = pl::pcos::key::from_string( "force" );
static pl::pcos::key key_width_ = pl::pcos::key::from_string( "width" );
static pl::pcos::key key_height_ = pl::pcos::key::from_string( "height" );
static pl::pcos::key key_fps_num_ = pl::pcos::key::from_string( "fps_num" );
static pl::pcos::key key_fps_den_ = pl::pcos::key::from_string( "fps_den" );
static pl::pcos::key key_sar_num_ = pl::pcos::key::from_string( "sar_num" );
static pl::pcos::key key_sar_den_ = pl::pcos::key::from_string( "sar_den" );
static pl::pcos::key key_interlace_ = pl::pcos::key::from_string( "interlace" );
static pl::pcos::key key_bit_rate_ = pl::pcos::key::from_string( "bit_rate" );
static pl::pcos::key key_complete_ = pl::pcos::key::from_string( "complete" );
static pl::pcos::key key_instances_ = pl::pcos::key::from_string( "instances" );

class filter_pool
{
public:
	virtual ~filter_pool( ) { }
	
	friend class shared_filter_pool;
	friend class frame_lazy;
	
protected:
	virtual ml::filter_type_ptr filter_obtain( ) = 0;
	virtual void filter_release( ml::filter_type_ptr ) = 0;
	
};

class shared_filter_pool
{
public:
	ml::filter_type_ptr filter_obtain( filter_pool *pool_token )
	{
		boost::recursive_mutex::scoped_lock lck( mtx_ );
		std::set< filter_pool * >::iterator it = pools_.begin( );
		for( ; it != pools_.end( ); ++it )
		{
			if( *it == pool_token )
			{
				return (*it)->filter_obtain( );
			}
		}
		return ml::filter_type_ptr( );
	}

	void filter_release( ml::filter_type_ptr filter, filter_pool *pool_token )
	{
		ARENFORCE_MSG( filter, "Releasing null filter" );
		
		boost::recursive_mutex::scoped_lock lck( mtx_ );
		std::set< filter_pool * >::iterator it = pools_.begin( );
		for( ; it != pools_.end( ); ++it )
		{
			if( *it == pool_token )
			{
				(*it)->filter_release( filter );
			}
		}
	}
	
	void add_pool( filter_pool *pool )
	{
		boost::recursive_mutex::scoped_lock lck( mtx_ );
		pools_.insert( pool );
	}
	
	void remove_pool( filter_pool * pool )
	{
		boost::recursive_mutex::scoped_lock lck( mtx_ );
		pools_.erase( pool );
	}
		
private:
	std::set< filter_pool * > pools_;
	boost::recursive_mutex mtx_;
		   
};
	
typedef Loki::SingletonHolder< shared_filter_pool, Loki::CreateUsingNew, Loki::PhoenixSingleton > the_shared_filter_pool;

class ML_PLUGIN_DECLSPEC frame_lazy : public ml::frame_type 
{
	private:

		//Helper class that will make sure that the filter is returned to the
		//filter pool, once all users of it are gone.
		class filter_pool_holder
		{
		public:
			filter_pool_holder( const ml::filter_type_ptr &filter, filter_pool *pool )
			: filter_(filter)
			, pool_( pool )
			{ }

			virtual ~filter_pool_holder()
			{
				if ( filter_ )
					the_shared_filter_pool::Instance().filter_release( filter_, pool_ );
			}

			ml::filter_type_ptr filter()
			{
				filter_ = filter_ ? filter_ : the_shared_filter_pool::Instance().filter_obtain( pool_ );
				return filter_;
			}

		private:
			ml::filter_type_ptr filter_;
			filter_pool *pool_;
		};


	public:
		/// Constructor
		frame_lazy( const frame_type_ptr &other, int frames, filter_pool *pool_token, ml::filter_type_ptr filter, bool validated = true )
			: ml::frame_type( *other )
			, parent_( other )
			, frames_( frames )
			, pool_holder_( new filter_pool_holder( filter, pool_token ) )
			, validated_( validated )
			, evaluated_( false )
		{
		}

		/// Destructor
		virtual ~frame_lazy( )
		{
		}

		void evaluate( )
		{
			filter_type_ptr filter;
			evaluated_ = true;
			if ( pool_holder_ && ( filter = pool_holder_->filter( ) ) )
			{
				ml::frame_type_ptr other = parent_;

				{
					ml::input_type_ptr pusher = filter->fetch_slot( 0 );

					other->get_image( );

					if ( pusher == 0 )
					{
						pusher = ml::create_input( L"pusher:" );
						pusher->property_with_key( key_length_ ) = frames_;
						filter->connect( pusher );
						filter->sync( );
					}
					else if ( pusher->get_uri( ) == L"pusher:" && pusher->get_frames( ) != frames_ )
					{
						pusher->property_with_key( key_length_ ) = frames_;
						filter->sync( );
					}

					filter->fetch_slot( 0 )->push( other );
					filter->seek( other->get_position( ) );
					other = filter->fetch( );
				}

				image_ = other->get_image( );
				alpha_ = other->get_alpha( );
				audio_ = other->get_audio( );
				//properties_ = other->properties( );
				stream_ = other->get_stream( );
				pts_ = other->get_pts( );
				duration_ = other->get_duration( );
				//sar_num_ = other->get_sar_num( );
				//sar_den_ = other->get_sar_den( );
				fps_num_ = other->get_fps_num( );
				fps_den_ = other->get_fps_den( );
				exceptions_ = other->exceptions( );
				queue_ = other->queue( );

				//We will not have any use for the filter now
				pool_holder_.reset();
			}
		}

		/// Provide a shallow copy of the frame (and all attached frames)
		virtual frame_type_ptr shallow( )
		{
			ml::frame_type_ptr inner_copy;
			//If we have already been evaluated, we should no use the parent_ frame
			//anymore, since that will not necessarily have the decoded image set on it,
			//but we will have that on this frame already.
			if( evaluated_ )
				inner_copy = frame_type::shallow();
			else
				inner_copy = parent_->shallow();

			ml::frame_type_ptr clone( new frame_lazy( inner_copy, frames_, inner_copy, pool_holder_, validated_ ) );
			clone->set_position( get_position() );
			return clone;
		}

		/// Provide a deep copy of the frame (and all attached frames)
		virtual frame_type_ptr deep( )
		{
			frame_type_ptr result = parent_->deep( );
			result->set_image( get_image( ) );
			return result;
		}

		/// Indicates if the frame has an image
		virtual bool has_image( )
		{
			return !evaluated_ || image_;
		}

		/// Indicates if the frame has audio
		virtual bool has_audio( )
		{
			return ( pool_holder_ && parent_->has_audio( ) ) || audio_;
		}

		/// Set the image associated to the frame.
		virtual void set_image( olib::openimagelib::il::image_type_ptr image, bool decoded )
		{
			frame_type::set_image( image, decoded );
			evaluated_ = true;
			pool_holder_.reset();
			if( parent_ )
				parent_->set_image( image, decoded );
		}

		/// Get the image associated to the frame.
		virtual olib::openimagelib::il::image_type_ptr get_image( )
		{
			if( !evaluated_ ) evaluate( );
			return image_;
		}

		/// Set the audio associated to the frame.
		virtual void set_audio( audio_type_ptr audio, bool decoded )
		{
			frame_type::set_audio( audio );
			//We need to set the audio object on the inner frame as well to avoid
			//our own audio object being overwritten incorrectly on an evaulate() call
			if( parent_ )
				parent_->set_audio(audio);
		}

		/// Get the audio associated to the frame.
		virtual audio_type_ptr get_audio( )
		{
			return audio_;
		}

		virtual void set_stream( stream_type_ptr stream )
		{
			frame_type::set_stream( stream );
			if( parent_ )
				parent_->set_stream( stream );
		}

		virtual stream_type_ptr get_stream( )
		{
			if( !stream_ || !validated_ ) evaluate( );
			return stream_;
		}

		virtual void set_position( int position )
		{
			frame_type::set_position( position );
		}

	protected:
		
		frame_lazy( const frame_type_ptr &org, int frames, const frame_type_ptr &other, const boost::shared_ptr< filter_pool_holder > &pool_holder, bool validated )
			: ml::frame_type( *org )
			, parent_( other )
			, frames_( frames )
			, pool_holder_( pool_holder )
			, validated_( validated )
			, evaluated_( false )
		{
		}

	private:
		ml::frame_type_ptr parent_;
		int frames_;
		boost::shared_ptr< filter_pool_holder > pool_holder_;
		bool validated_;
		bool evaluated_;
};

namespace {
	
	static const pl::pcos::key key_temporal_offset_ = pl::pcos::key::from_string( "temporal_offset" );
	static const pl::pcos::key key_temporal_stream_ = pl::pcos::key::from_string( "temporal_stream" );

	/// Sequence header info
	static const pl::pcos::key key_frame_rate_code_ = pl::pcos::key::from_string( "frame_rate_code" );
	static const pl::pcos::key key_bit_rate_value_ = pl::pcos::key::from_string( "bit_rate_value" );
	static const pl::pcos::key key_vbv_buffer_size_ = pl::pcos::key::from_string( "vbv_buffer_size" );
	
	/// Sequence extension info
	static const pl::pcos::key key_chroma_format_ = pl::pcos::key::from_string( "chroma_format" );
	
	// Extended picture params
	static const pl::pcos::key key_temporal_reference_ = pl::pcos::key::from_string( "temporal_reference" );
	static const pl::pcos::key key_picture_coding_type_ = pl::pcos::key::from_string( "picture_coding_type" );
	static const pl::pcos::key key_vbv_delay_ = pl::pcos::key::from_string( "vbv_delay" );
	static const pl::pcos::key key_top_field_first_ = pl::pcos::key::from_string( "top_field_first" );
	static const pl::pcos::key key_frame_pred_frame_dct_ = pl::pcos::key::from_string( "frame_pred_frame_dct" );
	static const pl::pcos::key key_progressive_frame_ = pl::pcos::key::from_string( "progressive_frame" );
	
	// GOP header info
	static const pl::pcos::key key_closed_gop_ = pl::pcos::key::from_string( "closed_gop" );
	static const pl::pcos::key key_broken_link_ = pl::pcos::key::from_string( "broken_link" );
	
	namespace mpeg_start_code
	{
		enum type
		{
			undefined = -1,
			picture_start = 0x00,
			user_data_start = 0xb2,
			sequence_header = 0xb3,
			sequence_error = 0xb4,
			extension_start = 0xb5,
			sequence_end = 0xb7,
			group_start = 0xb8			
		};
	}
	
	namespace mpeg_extension_id {
		enum type
		{
			sequence_extension = 1,
			sequence_display_extension = 2,
			quant_matrix_extension = 3,
			copyright_extension = 4,
			sequence_scalable_extension = 5,
			// 6 is reserved
			picture_display_extension = 7,
			picture_coding_extension = 8,
			picture_spatial_scalable_extension = 9,
			picture_temporal_scalable_extension = 10		
		};
	}
}
    
class filter_analyse : public ml::filter_simple
{
public:
    // Filter_type overloads
    filter_analyse( )
    : ml::filter_simple( )
    {
		memset( &seq_hdr_, 0, sizeof( sequence_header ) );
		memset( &seq_ext_, 0, sizeof( sequence_extension ) );
		memset( &gop_hdr_, 0, sizeof( gop_header ) );
		memset( &pict_hdr_, 0, sizeof( picture_header ) );
		memset( &pict_ext_, 0, sizeof( picture_coding_extension ) );
	}
    
    virtual ~filter_analyse( )
    { }
    
    // Indicates if the input will enforce a packet decode
    virtual bool requires_image( ) const { return false; }
    
    // This provides the name of the plugin (used in serialisation)
    virtual const pl::wstring get_uri( ) const { return L"analyse"; }
    
protected:
    void do_fetch( ml::frame_type_ptr &result )
	{
		if ( gop_.find( get_position( ) ) == gop_.end( ) )
		{
			// Remove previously cached gop
			gop_.erase( gop_.begin( ), gop_.end( ) );

			// Fetch the current frame
			ml::frame_type_ptr temp;

			if ( inner_fetch( temp, get_position( ) ) && temp->get_stream( ) )
			{
				std::map< int, ml::stream_type_ptr > streams;
				int start = temp->get_stream( )->key( );
				int index = start;

				// Read the current gop
				while( index < get_frames( ) )
				{
					inner_fetch( temp, index ++ );
					if ( !temp || !temp->get_stream( ) || start != temp->get_stream( )->key( ) ) break;
					int sorted = start + temp->get_stream( )->properties( ).get_property_with_key( key_temporal_reference_ ).value< int >( );
					gop_[ temp->get_position( ) ] = temp;
					streams[ sorted ] = temp->get_stream( );
				}

				// Ensure that the recipient has access to the temporally sorted frames too
				for ( std::map< int, ml::frame_type_ptr >::iterator iter = gop_.begin( ); iter != gop_.end( ); iter ++ )
				{
					pl::pcos::property temporal_offset( key_temporal_offset_ );
					pl::pcos::property temporal_stream( key_temporal_stream_ );
					if ( streams.find( ( *iter ).first ) != streams.end( ) )
					{
						( *iter ).second->properties( ).append( temporal_offset = streams[ ( *iter ).first ]->position( ) );
						( *iter ).second->properties( ).append( temporal_stream = streams[ ( *iter ).first ] );
					}
				}

				result = gop_[ get_position( ) ];
			}
			else
			{
				result = temp;
			}
		}
		else
		{
			result = gop_[ get_position( ) ];
		}
	}
    
    // The main access point to the filter
    bool inner_fetch( ml::frame_type_ptr &result, int position )
    {
		fetch_slot( 0 )->seek( position );
		result = fetch_slot( 0 )->fetch( );
		if( !result ) return false;
		
		ml::stream_type_ptr stream = result->get_stream( );
		
		// If the frame has no stream then there is nothing to analyse.
		if( !stream ) return false;
		
		if( stream->codec( ) != "http://www.ardendo.com/apf/codec/mpeg/mpeg2" && stream->codec( ) != "http://www.ardendo.com/apf/codec/imx/imx" ) return false;
		        
        boost::uint8_t *data = stream->bytes( );
		size_t size = stream->length( );
		boost::uint8_t *end = data + size;
		
		bool done = false;
		mpeg_start_code::type sc = find_start( data, end );
		
		while( sc != mpeg_start_code::undefined && sc != mpeg_start_code::sequence_end && !done  )
		{
			switch ( sc )
			{
				case mpeg_start_code::picture_start:
					{
						parse_picture_header( data );
						
						pl::pcos::property temporal_reference_prop( key_temporal_reference_ );
						stream->properties( ).append( temporal_reference_prop = pict_hdr_.temporal_reference );
						
						pl::pcos::property picture_coding_type_prop( key_picture_coding_type_ );
						stream->properties( ).append( picture_coding_type_prop = pict_hdr_.picture_coding_type );

						pl::pcos::property vbv_delay_prop( key_vbv_delay_ );
						stream->properties( ).append( vbv_delay_prop = pict_hdr_.vbv_delay );					
					}					
					break;
					
				case mpeg_start_code::sequence_header:
					{
						parse_sequence_header( data );
						
						pl::pcos::property frame_rate_code_prop( key_frame_rate_code_ );
						stream->properties( ).append( frame_rate_code_prop = seq_hdr_.frame_rate_code );
						
						pl::pcos::property bit_rate_value_prop( key_bit_rate_value_ );
						stream->properties( ).append( bit_rate_value_prop = seq_hdr_.bit_rate_value );
						
						pl::pcos::property vbv_buffer_size_prop( key_vbv_buffer_size_ );
						stream->properties( ).append( vbv_buffer_size_prop = seq_hdr_.vbv_buffer_size_value );					
					}					
					break;
					
				case mpeg_start_code::extension_start:
					{
						mpeg_extension_id::type ext_id = (mpeg_extension_id::type)get( data, 0, 4 );
						if( ext_id == mpeg_extension_id::sequence_extension )
						{
							parse_sequence_extension( data );
							
							pl::pcos::property chroma_format_prop( key_chroma_format_ );
							stream->properties( ).append( chroma_format_prop = seq_ext_.chroma_format );
						}
						else if( ext_id == mpeg_extension_id::picture_coding_extension )
						{
							parse_picture_coding_extension( data );
							
							pl::pcos::property top_field_first_prop( key_top_field_first_ );
							stream->properties( ).append( top_field_first_prop = pict_ext_.top_field_first );
							
							pl::pcos::property frame_pred_frame_dct_prop( key_frame_pred_frame_dct_ );
							stream->properties( ).append( frame_pred_frame_dct_prop = pict_ext_.frame_pred_frame_dct );
							
							pl::pcos::property progressive_frame_prop( key_progressive_frame_ );
							stream->properties( ).append( progressive_frame_prop = pict_ext_.progressive_frame );
							
							// When we get here we should be done
							done = true;
						}
					}
					break;
					
				case mpeg_start_code::group_start:
					{
						parse_gop_header( data );
						
						pl::pcos::property broken_link_prop( key_broken_link_ );
						stream->properties( ).append( broken_link_prop = gop_hdr_.broken_link );
						
						pl::pcos::property closed_gop_prop( key_closed_gop_ );
						stream->properties( ).append( closed_gop_prop = gop_hdr_.closed_gop );					
					}
					break;
					
				case mpeg_start_code::sequence_end:
				case mpeg_start_code::user_data_start:
				case mpeg_start_code::sequence_error:
				default:
					break;
			}
			
			sc = find_start( data, end );
		}

		return true;
    }
    
private:

	int get( boost::uint8_t *p, int bit_offset, int bit_count )
	{
		int result = 0;
		p += bit_offset / 8;
		while( bit_count )
		{
			int bits = 8 - bit_offset % 8;
			int pattern = ( 1 << bits ) - 1;
			boost::uint8_t value = ( *p ++ & pattern );
			if ( bits > bit_count )
			{
				value = value >> ( bits - bit_count );
				bits = bit_count;
			}
			result = ( result << bits ) | value;
			bit_offset += bits;
			bit_count -= bits;
		}
		return result;
	}

	mpeg_start_code::type find_start( boost::uint8_t *&p, boost::uint8_t *end )
	{
		mpeg_start_code::type result = mpeg_start_code::undefined;

		while ( p + 4 < end && result == mpeg_start_code::undefined )
		{
			bool found = false;

			while( !found && p + 4 < end )
			{
				p = ( uint8_t * )memchr( p + 2, 1, end - p );
				if ( p )
				{
					found = *( p - 2 ) == 0 && *( p - 1 ) == 0;
					p ++;
				}
				else
				{
					p = end;
					return result;
				}
			}

			if ( found )
			{
				switch( *p ++ )
				{
					case 0x00:    result = mpeg_start_code::picture_start; break;
					case 0xb2:    result = mpeg_start_code::user_data_start; break;
					case 0xb3:    result = mpeg_start_code::sequence_header; break;
					case 0xb4:    result = mpeg_start_code::sequence_error; break;
					case 0xb5:    result = mpeg_start_code::extension_start; break;
					case 0xb7:    result = mpeg_start_code::sequence_end; break;
					case 0xb8:    result = mpeg_start_code::group_start; break;
				}
			}
		}

		if ( p + 4 > end )
		{
			p = end;
		}
		
		return result;
	}

	////////////////////////////////////////////
	// Sequence header structure	
	typedef struct sequence_header
	{
		int horizontal_size_value;			// 12 bits
		int vertical_size_value;			// 12 bits
		int aspect_ratio_information;		// 4 bits
		int frame_rate_code;				// 4 bits
		int bit_rate_value;					// 18 bits
		int marker_bit;					// 1 bits
		int vbv_buffer_size_value;			// 10 bits
		int constrained_parameters_flag;	// 1 bits
	} sequence_header;
	
	void parse_sequence_header( boost::uint8_t *&buf )
	{
		// Size of this header is at least 64 bits == 8 bytes
		int size = 8;
		
		int bit_offset = 0;
		seq_hdr_.horizontal_size_value = get( buf, bit_offset, 12 ); bit_offset += 12;
		seq_hdr_.vertical_size_value = get( buf, bit_offset, 12 ); bit_offset += 12;
		seq_hdr_.aspect_ratio_information = get( buf, bit_offset, 4 ); bit_offset += 4;
		seq_hdr_.frame_rate_code = get( buf, bit_offset, 4 ); bit_offset += 4;
		seq_hdr_.bit_rate_value = get( buf, bit_offset, 18 ); bit_offset += 18;
		seq_hdr_.marker_bit = get( buf, bit_offset, 1 ); bit_offset += 1;
		seq_hdr_.vbv_buffer_size_value = get( buf, bit_offset, 10 ); bit_offset += 10;
		seq_hdr_.constrained_parameters_flag = get( buf, bit_offset, 1 ); bit_offset += 1;
		
		bool load_intra_quantiser_matrix = get( buf, bit_offset, 1 ); bit_offset += 1;
		if( load_intra_quantiser_matrix )
		{
			bit_offset += 8*64;
			size += 64;
		}
		
		bool load_non_intra_quantiser_matrix = get( buf, bit_offset, 1 ); bit_offset += 1;
		if( load_non_intra_quantiser_matrix )
		{
			bit_offset += 8*64;
			size += 64;
		}
		
		// Update data pointer
		buf += size;
	}
	
	///////////////////////////////////////////////
	// Sequence extension structure
	typedef struct sequence_extension {
		int extension_start_code_identifier;	// 4 bits
		int profile_and_level_indication;		// 8 bits
		int progressive_sequence;				// 1 bit
		int chroma_format;						// 2 bits
		int horizontal_size_extension;			// 2 bits
		int vertical_size_extension;			// 2 bits
		int bit_rate_extension;					// 12 bits
		int marker_bit;						// 1 bits
		int vbv_buffer_size_extension;			// 8 bits
		int low_delay;							// 1 bits
		int frame_rate_extension_n;				// 2 bits
		int frame_rate_extension_d;				// 5 bits
	} sequence_extension;
	
	void parse_sequence_extension( boost::uint8_t *&buf )
	{
		// Size of sequence header extension is 48 bits == 6 bytes
		int size = 6;
		
		int bit_offset = 0;
		seq_ext_.extension_start_code_identifier = get( buf, bit_offset, 4 ); bit_offset += 4;
		seq_ext_.profile_and_level_indication = get( buf, bit_offset, 8 ); bit_offset += 8;
		seq_ext_.progressive_sequence = get( buf, bit_offset, 1 ); bit_offset += 1;
		seq_ext_.chroma_format = get( buf, bit_offset, 2 ); bit_offset += 2;
		seq_ext_.horizontal_size_extension = get( buf, bit_offset, 2 ); bit_offset += 2;
		seq_ext_.vertical_size_extension = get( buf, bit_offset, 2 ); bit_offset += 2;
		seq_ext_.bit_rate_extension = get( buf, bit_offset, 12 ); bit_offset += 12;
		seq_ext_.marker_bit = get( buf, bit_offset, 1 ); bit_offset += 1;
		seq_ext_.vbv_buffer_size_extension = get( buf, bit_offset, 8 ); bit_offset += 8;
		seq_ext_.low_delay = get( buf, bit_offset, 1 ); bit_offset += 1;
		seq_ext_.frame_rate_extension_n = get( buf, bit_offset, 2 ); bit_offset += 2;
		seq_ext_.frame_rate_extension_d = get( buf, bit_offset, 5 ); bit_offset += 5;
		
		buf += size;
	}
	
	///////////////////////////////////////////
	// GOP header structure
	typedef struct gop_header
	{
		int time_code;		// 25 bits
		int closed_gop;	// 1 bit
		int broken_link;	// 1 bit
	} gop_header;
	
	void parse_gop_header( boost::uint8_t *&buf )
	{
		// Size of sequence header extension is 26 bits so increase pointer with 3
		int size = 3;
		
		int bit_offset = 0;
		gop_hdr_.time_code = get( buf, bit_offset, 25 ); bit_offset += 25;
		gop_hdr_.closed_gop = get( buf, bit_offset, 1 ); bit_offset += 1;
		gop_hdr_.broken_link = get( buf, bit_offset, 1 ); bit_offset += 1;
		
		buf += size;
	}
	
	typedef struct picture_header
	{
		int temporal_reference;			// 10 bits
		int picture_coding_type;		// 3 bits
		int vbv_delay;					// 16 bits
		int full_pel_forward_vector;	// 1 bit
		int forward_f_code;				// 3 bits
		int full_pel_backward_vector;	// 1 bit
		int backward_f_code;			// 3 bits
	} picture_header;
	
	void parse_picture_header( boost::uint8_t *&buf )
	{
		int bit_offset = 0;
		pict_hdr_.temporal_reference = get( buf, bit_offset, 10 ); bit_offset += 10;
		pict_hdr_.picture_coding_type = get( buf, bit_offset, 3 ); bit_offset += 3;
		pict_hdr_.vbv_delay = get( buf, bit_offset, 16 ); bit_offset += 16;
		if ( pict_hdr_.picture_coding_type == 2 || pict_hdr_.picture_coding_type == 3) {
			pict_hdr_.full_pel_forward_vector = get( buf, bit_offset, 1 ); bit_offset += 1;
			pict_hdr_.forward_f_code = get( buf, bit_offset, 3 ); bit_offset += 3;
		}
		if ( pict_hdr_.picture_coding_type == 3 ) {
			pict_hdr_.full_pel_backward_vector = get( buf, bit_offset, 1 ); bit_offset += 1;
			pict_hdr_.backward_f_code = get( buf, bit_offset, 3 ); bit_offset += 3;
		}
		// Run through the rest of the header but we done care about the information
		while ( get( buf, bit_offset++, 1 ) == 1 ) {
//			extra_bit_picture
//			extra_information_picture
		}
	}
	
	typedef struct picture_coding_extension
	{
		int extension_start_code_identifier;	// 4 bits
		int f_code_0_0;							// 4 bits * forward horizontal *
		int f_code_0_1;							// 4 bits * forward vertical *
		int f_code_1_0;							// 4 bits * backward horizontal *
		int f_code_1_1;							// 4 bits * backward vertical *
		int intra_dc_precision;					// 2 bits
		int picture_structure;					// 2 bits
		int top_field_first;					// 1 bit
		int frame_pred_frame_dct;				// 1 bit
		int concealment_motion_vectors;			// 1 bit
		int q_scale_type;						// 1 bit
		int intra_vlc_format;					// 1 bit
		int alternate_scan;						// 1 bit
		int repeat_first_field;					// 1 bit
		int chroma_420_type;					// 1 bit
		int progressive_frame;					// 1 bit
		int composite_display_flag;				// 1 bit
		int v_axis;								// 1 bit
		int field_sequence;						// 3 bits
		int sub_carrier;						// 1 bit
		int burst_amplitude;					// 7 bits
		int sub_carrier_phase;					// 8 bits
	} picture_coding_extension;
	
	
	void parse_picture_coding_extension( boost::uint8_t *&buf )
	{
		int bit_offset = 0;
		pict_ext_.extension_start_code_identifier = get( buf, bit_offset, 4 ); bit_offset += 4;
		pict_ext_.f_code_0_0 = get( buf, bit_offset, 4 ); bit_offset += 4;
		pict_ext_.f_code_0_1 = get( buf, bit_offset, 4 ); bit_offset += 4;
		pict_ext_.f_code_1_0 = get( buf, bit_offset, 4 ); bit_offset += 4;
		pict_ext_.f_code_1_1 = get( buf, bit_offset, 4 ); bit_offset += 4;
		pict_ext_.intra_dc_precision = get( buf, bit_offset, 2 ); bit_offset += 2;
		pict_ext_.picture_structure = get( buf, bit_offset, 2 ); bit_offset += 2;
		pict_ext_.top_field_first = get( buf, bit_offset, 1 ); bit_offset += 1;
		pict_ext_.frame_pred_frame_dct = get( buf, bit_offset, 1 ); bit_offset += 1;
		pict_ext_.concealment_motion_vectors = get( buf, bit_offset, 1 ); bit_offset += 1;
		pict_ext_.q_scale_type = get( buf, bit_offset, 1 ); bit_offset += 1;
		pict_ext_.intra_vlc_format = get( buf, bit_offset, 1 ); bit_offset += 1;
		pict_ext_.alternate_scan = get( buf, bit_offset, 1 ); bit_offset += 1;
		pict_ext_.repeat_first_field = get( buf, bit_offset, 1 ); bit_offset += 1;
		pict_ext_.chroma_420_type = get( buf, bit_offset, 1 ); bit_offset += 1;
		pict_ext_.progressive_frame = get( buf, bit_offset, 1 ); bit_offset += 1;
		pict_ext_.composite_display_flag = get( buf, bit_offset, 1 ); bit_offset += 1;
		if ( pict_ext_.composite_display_flag ) {
			pict_ext_.v_axis = get( buf, bit_offset, 1 ); bit_offset += 1;
			pict_ext_.field_sequence = get( buf, bit_offset, 3 ); bit_offset += 3;
			pict_ext_.sub_carrier = get( buf, bit_offset, 1 ); bit_offset += 1;
			pict_ext_.burst_amplitude = get( buf, bit_offset, 7 ); bit_offset += 7;
			pict_ext_.sub_carrier_phase = get( buf, bit_offset, 8 ); bit_offset += 8;
		}
	}
    	
	/// Hold the last sequence header we read
	sequence_header seq_hdr_;
	/// Hold the last sequence extension information
	sequence_extension seq_ext_;
	/// Hold the last parsed gop header
	gop_header gop_hdr_;
	/// Hold the last picture header
	picture_header pict_hdr_;
	/// Hold the last extensions to picture header
	picture_coding_extension pict_ext_;

	std::map< int, ml::frame_type_ptr > gop_;
};

class ML_PLUGIN_DECLSPEC filter_decode : public filter_type, public filter_pool, public boost::enable_shared_from_this< filter_decode >
{
	private:
		boost::recursive_mutex mutex_;
		pl::pcos::property prop_inner_threads_;
		pl::pcos::property prop_filter_;
		pl::pcos::property prop_scope_;
		pl::pcos::property prop_source_uri_;
		std::deque< ml::filter_type_ptr > decoder_;
		ml::filter_type_ptr gop_decoder_;
		ml::frame_type_ptr last_frame_;
		cl::profile_ptr codec_to_decoder_;
		bool initialized_;
		std::string video_codec_;
		int total_frames_;
 
	public:
		filter_decode( )
			: filter_type( )
			, prop_inner_threads_( pl::pcos::key::from_string( "inner_threads" ) )
			, prop_filter_( pl::pcos::key::from_string( "filter" ) )
			, prop_scope_( pl::pcos::key::from_string( "scope" ) )
			, prop_source_uri_( pl::pcos::key::from_string( "source_uri" ) )
			, decoder_( )
			, codec_to_decoder_( )
			, initialized_( false )
			, total_frames_( 0 )
		{
			properties( ).append( prop_inner_threads_ = 1 );
			properties( ).append( prop_filter_ = pl::wstring( L"mcdecode" ) );
			properties( ).append( prop_scope_ = pl::wstring( cl::str_util::to_wstring( cl::uuid_16b().to_hex_string( ) ) ) );
			properties( ).append( prop_source_uri_ = pl::wstring( L"" ) );
			
			// Load the profile that contains the mappings between codec string and codec filter name
			codec_to_decoder_ = cl::profile_load( "codec_mappings" );
			
			the_shared_filter_pool::Instance( ).add_pool( this );
		}

		virtual ~filter_decode( )
		{
			the_shared_filter_pool::Instance( ).remove_pool( this );
		}

		// Indicates if the input will enforce a packet decode
		virtual bool requires_image( ) const { return false; }

		virtual const pl::wstring get_uri( ) const { return pl::wstring( L"decode" ); }

		virtual const size_t slot_count( ) const { return 1; }

		void sync( )
		{
			total_frames_ = 0;

			if ( gop_decoder_ )
			{
				gop_decoder_->sync( );
				total_frames_ = gop_decoder_->get_frames( );

				// Avoid smeared frames due to incomplete sources
				pl::pcos::property complete = fetch_slot( 0 )->properties( ).get_property_with_key( key_complete_ );
				if ( complete.valid( ) && complete.value< int >( ) == 0 )
					total_frames_ -= 2;
			}
			else if ( fetch_slot( 0 ) )
			{
				fetch_slot( 0 )->sync( );
				total_frames_ = fetch_slot( 0 )->get_frames( );
			}
		}

	protected:

		ml::filter_type_ptr filter_create( )
		{
			boost::recursive_mutex::scoped_lock lck( mutex_ );
			ml::input_type_ptr fg = ml::create_input( L"pusher:" );
			fg->property( "length" ) = get_frames( );
			
			ml::filter_type_ptr decode = ml::create_filter( prop_filter_.value< pl::wstring >( ) );
			if ( decode->property( "threads" ).valid( ) ) 
				decode->property( "threads" ) = prop_inner_threads_.value< int >( );
			if ( decode->property( "scope" ).valid( ) ) 
				decode->property( "scope" ) = prop_scope_.value< pl::wstring >( );
			if ( decode->property( "source_uri" ).valid( ) ) 
				decode->property( "source_uri" ) = prop_source_uri_.value< pl::wstring >( );
			
			decode->connect( fg );
			decode->sync( );
			
			return decode;
		}

 		ml::filter_type_ptr filter_obtain( )
		{
			boost::recursive_mutex::scoped_lock lck( mutex_ );
			ml::filter_type_ptr result;
			
			if ( decoder_.size( ) == 0 )
			{
				result = filter_create( );
				ARENFORCE_MSG( result, "Could not get a valid decoder filter" );
				decoder_.push_back( result );
                ARLOG_INFO( "Creating decoder. This = %1%, source_uri = %2%, scope = %3%" )( this )( prop_source_uri_.value< std::wstring >( ) )( prop_scope_.value< std::wstring >( ) );
			}
			
			result = decoder_.back( );
			decoder_.pop_back( );
			
			return result;
		}
	
		void filter_release( ml::filter_type_ptr filter )
		{
			boost::recursive_mutex::scoped_lock lck( mutex_ );
			decoder_.push_back( filter );
		}

		bool determine_decode_use( ml::frame_type_ptr &frame )
		{
			if( frame && frame->get_stream() && frame->get_stream( )->estimated_gop_size( ) != 1 )
			{
				ml::filter_type_ptr analyse = ml::filter_type_ptr( new filter_analyse( ) );
				analyse->connect( fetch_slot( 0 ) );
				gop_decoder_ = ml::create_filter( prop_filter_.value< pl::wstring >( ) );
				gop_decoder_->connect( analyse );
				gop_decoder_->sync( );
				return true;
			}
			return false;
		}

		void do_fetch( frame_type_ptr &frame )
		{
			int frameno = get_position( );
			
			if ( last_frame_ == 0 )
			{
				frame = fetch_from_slot( );
			
				// If there is nothing to decode just return frame
				if( !frame->get_stream( ) )
					return;
		
				if( !initialized_ )
				{
					initialize( frame );
					initialized_ = true;
				}
				
				determine_decode_use( frame );
			}

			if ( gop_decoder_ )
			{
				gop_decoder_->seek( frameno );
				frame = gop_decoder_->fetch( );
			}
			else
			{
				if ( !frame ) frame = fetch_from_slot( );
				ml::filter_type_ptr graph;
				frame = ml::frame_type_ptr( new frame_lazy( frame, get_frames( ), this, graph ) );
			}
		
			last_frame_ = frame;
		}
		
		void initialize( const ml::frame_type_ptr &first_frame )
		{
			ARENFORCE_MSG( codec_to_decoder_, "Need mappings from codec string to name of decoder" );
			ARENFORCE_MSG( first_frame && first_frame->get_stream( ), "No frame or no stream on frame" );
			
			cl::profile::list::const_iterator it = codec_to_decoder_->find( first_frame->get_stream( )->codec( ) );
			ARENFORCE_MSG( it != codec_to_decoder_->end( ), "Failed to find a apropriate codec" )( first_frame->get_stream( )->codec( ) );
			
			ARLOG_INFO( "Stream itentifier is %1%. Using decode filter %2%" )( first_frame->get_stream( )->codec( ) )( it->value );
			
			prop_filter_ = pl::wstring( cl::str_util::to_wstring( it->value ) );
		}
};

class ML_PLUGIN_DECLSPEC filter_encode : public filter_encode_type, public filter_pool
{
	private:
		boost::recursive_mutex mutex_;
		pl::pcos::property prop_filter_;
		pl::pcos::property prop_profile_;
		pl::pcos::property prop_instances_;
		std::deque< ml::filter_type_ptr > decoder_;
		ml::filter_type_ptr gop_encoder_;
		bool is_long_gop_;
		ml::frame_type_ptr last_frame_;
		cl::profile_ptr profile_to_encoder_mappings_;
		std::string video_codec_;
		bool stream_validation_;
 
	public:
		filter_encode( )
			: filter_encode_type( )
			, prop_filter_( pl::pcos::key::from_string( "filter" ) )
			, prop_profile_( pl::pcos::key::from_string( "profile" ) )
			, prop_instances_( pl::pcos::key::from_string( "instances" ) )
			, decoder_( )
			, gop_encoder_( )
			, is_long_gop_( false )
			, last_frame_( )
			, profile_to_encoder_mappings_( )
			, video_codec_( )
			, stream_validation_( false )
		{
			// Default to something. Will be overriden anyway as soon as we init
			properties( ).append( prop_filter_ = pl::wstring( L"mcencode" ) );
			// Default to something. Should be overriden anyway.
			properties( ).append( prop_profile_ = pl::wstring( L"vcodecs/avcintra100" ) );
			properties( ).append( prop_instances_ = 4 );
			
			the_shared_filter_pool::Instance( ).add_pool( this );
		}

		virtual ~filter_encode( )
		{
			the_shared_filter_pool::Instance( ).remove_pool( this );
		}

		// Indicates if the input will enforce a packet decode
		virtual bool requires_image( ) const { return false; }

		virtual const pl::wstring get_uri( ) const { return pl::wstring( L"encode" ); }

		virtual const size_t slot_count( ) const { return 1; }

	protected:

		ml::filter_type_ptr filter_create( )
		{
			boost::recursive_mutex::scoped_lock lck( mutex_ );
			// Create the encoder that we have mapped from our profile
			ml::filter_type_ptr encode = ml::create_filter( prop_filter_.value< pl::wstring >( ) );
			ARENFORCE_MSG( encode, "Failed to create encoder" )( prop_filter_.value< pl::wstring >( ) );

			// Set the profile property on the encoder
			ARENFORCE_MSG( encode->properties( ).get_property_with_string( "profile" ).valid( ),
				"Encode filter must have a profile property" );
			encode->properties( ).get_property_with_string( "profile" ) = prop_profile_.value< pl::wstring >( );

			if ( encode->properties( ).get_property_with_key( key_instances_ ).valid( ) )
				encode->properties( ).get_property_with_key( key_instances_ ) = prop_instances_.value< int >( );
			
			return encode;
		}

 		ml::filter_type_ptr filter_obtain( )
		{
			boost::recursive_mutex::scoped_lock lck( mutex_ );
			ml::filter_type_ptr result = gop_encoder_;
			if ( !result )
			{
				if ( decoder_.size( ) )
				{
					result = decoder_.back( );
					decoder_.pop_back( );
				}
				else
				{
					result = filter_create( );
				}

				if( result->properties( ).get_property_with_key( key_force_ ).valid( ) ) 
					result->properties( ).get_property_with_key( key_force_ ) = prop_force_.value< int >( );
				if( result->properties( ).get_property_with_key( key_width_ ).valid( ) ) 
					result->properties( ).get_property_with_key( key_width_ ) = prop_width_.value< int >( );
				if( result->properties( ).get_property_with_key( key_height_ ).valid( ) ) 
					result->properties( ).get_property_with_key( key_height_ ) = prop_height_.value< int >( );
				if( result->properties( ).get_property_with_key( key_fps_num_ ).valid( ) ) 
					result->properties( ).get_property_with_key( key_fps_num_ ) = prop_fps_num_.value< int >( );
				if( result->properties( ).get_property_with_key( key_fps_den_ ).valid( ) ) 
					result->properties( ).get_property_with_key( key_fps_den_ ) = prop_fps_den_.value< int >( );
				if( result->properties( ).get_property_with_key( key_sar_num_ ).valid( ) ) 
					result->properties( ).get_property_with_key( key_sar_num_ ) = prop_sar_num_.value< int >( );
				if( result->properties( ).get_property_with_key( key_sar_den_ ).valid( ) ) 
					result->properties( ).get_property_with_key( key_sar_den_ ) = prop_sar_den_.value< int >( );
				if( result->properties( ).get_property_with_key( key_interlace_ ).valid( ) ) 
					result->properties( ).get_property_with_key( key_interlace_ ) = prop_interlace_.value< int >( );
				if( result->properties( ).get_property_with_key( key_bit_rate_ ).valid( ) ) 
					result->properties( ).get_property_with_key( key_bit_rate_ ) = prop_bit_rate_.value< int >( );
			}
			return result;
		}

		void filter_release( ml::filter_type_ptr filter )
		{
			boost::recursive_mutex::scoped_lock lck( mutex_ );
			if ( filter != gop_encoder_ )
				decoder_.push_back( filter );
		}

		void push( ml::input_type_ptr input, ml::frame_type_ptr frame )
		{
			if ( input && frame )
			{
				if ( input->get_uri( ) == L"pusher:" )
					input->push( frame );
				for( size_t i = 0; i < input->slot_count( ); i ++ )
					push( input->fetch_slot( i ), frame );
			}
		}

		bool create_pushers( )
		{
			if( is_long_gop_ )
			{
				ARLOG_DEBUG3( "Creating gop_encoder in encode filter" );
				gop_encoder_ = filter_create( );
				gop_encoder_->connect( fetch_slot( 0 ) );
				
				return gop_encoder_.get( ) != 0;
			}
			return false;
		}

		void do_fetch( frame_type_ptr &frame )
		{
			int frameno = get_position( );
		
			if ( last_frame_ == 0 )
			{
				initialize_encoder_mapping( );
				create_pushers( );
			}

			if ( last_frame_ && last_frame_->get_position( ) == get_position( ) )
			{
				frame = last_frame_;
			}
			else if ( gop_encoder_ )
			{
				gop_encoder_->seek( frameno );
				frame = gop_encoder_->fetch( );
			}
			else
			{
				ml::filter_type_ptr graph;
				frame = fetch_from_slot( );
				frame->set_position( get_position( ) );

				// Check the first frame here so that any unspecified properties are picked up at this point
				if ( last_frame_ == 0 )
				{
					matching( frame );
					ARENFORCE_MSG( valid( frame ), "Invalid frame for encoder" );
				}
			   
				frame = ml::frame_type_ptr( new frame_lazy( frame, get_frames( ), this, graph, !stream_validation_ ) );
			}
		
			// Keep a reference to the last frame in case of a duplicated request
			last_frame_ = frame;
		}

		void initialize_encoder_mapping( )
		{
			// Load the profile that contains the mappings between codec string and codec filter name
			profile_to_encoder_mappings_ = cl::profile_load( "encoder_mappings" );
			
			// First figure out what encoder to use based on our profile
			ARENFORCE_MSG( profile_to_encoder_mappings_, "Need mappings from profile string to name of encoder" );
			
			std::string prof = cl::str_util::to_string( prop_profile_.value< pl::wstring >( ).c_str( ) );
			cl::profile::list::const_iterator it = profile_to_encoder_mappings_->find( prof );
			ARENFORCE_MSG( it != profile_to_encoder_mappings_->end( ), "Failed to find a apropriate encoder" )( prof );

			ARLOG_DEBUG3( "Will use encode filter \"%1%\" for profile \"%2%\"" )( it->value )( prop_profile_.value< pl::wstring >( ) );
			
			prop_filter_ = pl::wstring( cl::str_util::to_wstring( it->value ) );
			
			// Now load the actual profile to find out what video codec string we will produce
			cl::profile_ptr encoder_profile = cl::profile_load( prof );
			ARENFORCE_MSG( encoder_profile, "Failed to load encode profile" )( prof );
			
			cl::profile::list::const_iterator vc_it = encoder_profile->find( "stream_codec_id" );
			ARENFORCE_MSG( vc_it != encoder_profile->end( ), "Profile is missing a stream_codec_id" );

			video_codec_ = vc_it->value;

			vc_it = encoder_profile->find( "video_gop_size" );
			if( vc_it != encoder_profile->end() && vc_it->value != "1" )
			{
				ARLOG_DEBUG3( "Video codec profile %1% is long gop, since video_gop_size is %2%" )( prof )( vc_it->value );
				is_long_gop_ = true;
			}

			vc_it = encoder_profile->find( "stream_validation" );
			stream_validation_ = is_long_gop_ || ( vc_it != encoder_profile->end() && vc_it->value == "1" );
			if ( stream_validation_ )
			{
				ARLOG_DEBUG3( "Encoder plugin is responsible for validating the stream" );
			}
		}
};

class ML_PLUGIN_DECLSPEC filter_rescale : public filter_simple
{
	public:
		filter_rescale( )
			: filter_simple( )
			, prop_enable_( pl::pcos::key::from_string( "enable" ) )
			, prop_progressive_( pl::pcos::key::from_string( "progressive" ) )
			, prop_interp_( pl::pcos::key::from_string( "interp" ) )
			, prop_width_( pl::pcos::key::from_string( "width" ) )
			, prop_height_( pl::pcos::key::from_string( "height" ) )
			, prop_sar_num_( pl::pcos::key::from_string( "sar_num" ) )
			, prop_sar_den_( pl::pcos::key::from_string( "sar_den" ) )
		{
			properties( ).append( prop_enable_ = 1 );
			properties( ).append( prop_progressive_ = 1 );
			properties( ).append( prop_interp_ = 1 );
			properties( ).append( prop_width_ = 640 );
			properties( ).append( prop_height_ = 360 );
			properties( ).append( prop_sar_num_ = 1 );
			properties( ).append( prop_sar_den_ = 1 );
		}

		virtual ~filter_rescale( )
		{
		}

		// Indicates if the input will enforce a packet decode
		virtual bool requires_image( ) const { return false; }

		virtual const pl::wstring get_uri( ) const { return pl::wstring( L"rescale" ); }

		virtual const size_t slot_count( ) const { return 1; }

	protected:

		void do_fetch( frame_type_ptr &frame )
		{
			frame = fetch_from_slot( );
			if ( prop_enable_.value< int >( ) && frame && frame->has_image( ) )
			{
				il::image_type_ptr image = frame->get_image( );
				if ( prop_progressive_.value< int >( ) )
					image = il::deinterlace( image );
				image = il::rescale( image, prop_width_.value< int >( ), prop_height_.value< int >( ), il::rescale_filter( prop_interp_.value< int >( ) ) );
				frame->set_image( image );
				frame->set_sar( prop_sar_num_.value< int >( ), prop_sar_den_.value< int >( ) );
			}
		}

	private:
		pl::pcos::property prop_enable_;
		pl::pcos::property prop_progressive_;
		pl::pcos::property prop_interp_;
		pl::pcos::property prop_width_;
		pl::pcos::property prop_height_;
		pl::pcos::property prop_sar_num_;
		pl::pcos::property prop_sar_den_;
};

class ML_PLUGIN_DECLSPEC filter_field_order : public filter_simple
{
	public:
		filter_field_order( )
			: filter_simple( )
			, prop_order_( pl::pcos::key::from_string( "order" ) )
		{
			properties( ).append( prop_order_ = 2 );
		}

		virtual ~filter_field_order( )
		{
		}

		// Indicates if the input will enforce a packet decode
		virtual bool requires_image( ) const { return true; }

		virtual const pl::wstring get_uri( ) const { return pl::wstring( L"field_order" ); }

		virtual const size_t slot_count( ) const { return 1; }

	protected:

		void do_fetch( frame_type_ptr &frame )
		{
			frame = fetch_from_slot( );

			if ( prop_order_.value< int >( ) && frame && frame->has_image( ) )
			{
				// Fetch image
				il::image_type_ptr image = frame->get_image( );

				if ( image->field_order( ) != il::progressive && image->field_order( ) != il::field_order_flags( prop_order_.value< int >( ) ) )
				{
					// Do we have a previous frame and is it the one preceding this request?
					// If not, we'll just provide the current frame as the result
					if ( previous_ && previous_->position( ) == get_position( ) - 1 )
					{
						// Image is definitely in the wrong order - guessing here :-)
						il::image_type_ptr merged = merge( image, 0, previous_, 1 );
						merged->set_field_order( il::field_order_flags( prop_order_.value< int >( ) ) );
						merged->set_position( get_position( ) );
						frame->set_image( merged );
					}
					else
					{
						image->set_field_order( il::field_order_flags( prop_order_.value< int >( ) ) );
					}

					// We need to remember this frame for reuse on the next request
					previous_ = image;
					previous_->set_position( get_position( ) );
				}
			}
		}

		il::image_type_ptr merge( il::image_type_ptr image1, int scan1, il::image_type_ptr image2, int scan2 )
		{
			// Create a new image which has the same dimensions
			// NOTE: we assume that image dimensions are consistent (should we?)
			il::image_type_ptr result = il::allocate( image1 );

			for ( int i = 0; i < result->plane_count( ); i ++ )
			{
				boost::uint8_t *dst_row = result->data( i );
				boost::uint8_t *src1_row = image1->data( i ) + image1->pitch( i ) * scan1;
				boost::uint8_t *src2_row = image2->data( i ) + image2->pitch( i ) * scan2;
				int dst_linesize = result->linesize( i );
				int dst_stride = result->pitch( i );
				int src_stride = image1->pitch( i ) * 2;
				int height = result->height( i ) / 2;

				while( height -- )
				{
					memcpy( dst_row, src1_row, dst_linesize );
					dst_row += dst_stride;
					src1_row += src_stride;

					memcpy( dst_row, src2_row, dst_linesize );
					dst_row += dst_stride;
					src2_row += src_stride;
				}
			}

			return result;
		}

	private:
		pl::pcos::property prop_order_;
		il::image_type_ptr previous_;
};

class ML_PLUGIN_DECLSPEC filter_lazy : public filter_type, public filter_pool
{
	public:
		filter_lazy( const pl::wstring &spec )
			: filter_type( )
			, spec_( spec )
		{
			filter_ = ml::create_filter( spec_.substr( 5 ) );
			properties_ = filter_->properties( );
			
			the_shared_filter_pool::Instance( ).add_pool( this );
		}

		virtual ~filter_lazy( )
		{
			the_shared_filter_pool::Instance( ).remove_pool( this );
		}

		// Indicates if the input will enforce a packet decode
		virtual bool requires_image( ) const { return false; }

		virtual const pl::wstring get_uri( ) const { return spec_; }

		virtual const size_t slot_count( ) const { return filter_ ? filter_->slot_count( ) : 1; }

		virtual bool is_thread_safe( ) { return filter_ ? filter_->is_thread_safe( ) : false; }

		void on_slot_change( input_type_ptr input, int slot )
		{
			if ( filter_ )
			{
				filter_->connect( input, slot );
				filter_->sync( );
			}
		}

		int get_frames( ) const
		{
			return filter_ ? filter_->get_frames( ) : 0;
		}

	protected:

 		ml::filter_type_ptr filter_obtain( )
		{
			if ( filters_.size( ) == 0 )
				filters_.push_back( filter_create( ) );

			ml::filter_type_ptr result = filters_.back( );
			filters_.pop_back( );

			pcos::key_vector keys = properties_.get_keys( );
			for( pcos::key_vector::iterator it = keys.begin( ); it != keys.end( ); it ++ )
			{
				if ( result->property_with_key( *it ).valid( ) )
					result->property_with_key( *it ).set_from_property( properties_.get_property_with_key( *it ) );
				else
				{
					pl::pcos::property prop( *it );
					prop.set_from_property( properties_.get_property_with_key( *it ) );
					result->properties( ).append( prop );
				}
			}

			return result;
		}

		ml::filter_type_ptr filter_create( )
		{
			return ml::create_filter( spec_.substr( 5 ) );
		}

		void filter_release( ml::filter_type_ptr filter )
		{
			filters_.push_back( filter );
		}

		void do_fetch( frame_type_ptr &frame )
		{
			if ( last_frame_ && get_position( ) == last_frame_->get_position( ) )
			{
				frame = last_frame_;
			}
			else
			{
				frame = fetch_from_slot( 0 );
				if ( filter_ )
				{
					frame = ml::frame_type_ptr( new frame_lazy( frame, get_frames( ), this, ml::filter_type_ptr( ) ) );
				}
			}

			last_frame_ = frame;
		}

		void sync_frames( )
		{
			if ( filter_ )
				filter_->sync( );
		}

	private:
		boost::recursive_mutex mutex_;
		pl::wstring spec_;
		ml::filter_type_ptr filter_;
		std::deque< ml::filter_type_ptr > filters_;
		ml::frame_type_ptr last_frame_;
};

class ML_PLUGIN_DECLSPEC filter_map_reduce : public filter_simple
{
	private:
		pl::pcos::property prop_threads_;
		long int frameno_;
		int threads_;
		std::map< int, frame_type_ptr > frame_map;
		cl::thread_pool *pool_;
		boost::recursive_mutex mutex_;
		boost::condition_variable_any cond_;
		ml::frame_type_ptr last_frame_;
		int expected_;
		int incr_;
		bool thread_safe_;

	public:
		filter_map_reduce( )
			: filter_simple( )
			, prop_threads_( pl::pcos::key::from_string( "threads" ) )
			, frameno_( 0 )
			, threads_( 4 )
			, pool_( 0 )
			, mutex_( )
			, cond_( )
			, expected_( 0 )
			, incr_( 1 )
			, thread_safe_( false )
		{
			properties( ).append( prop_threads_ = 4 );
		}

		virtual ~filter_map_reduce( )
		{
			if ( pool_ )
				pool_->terminate_all_threads( boost::posix_time::seconds( 5 ) );
		}

		// Indicates if the input will enforce a packet decode
		virtual bool requires_image( ) const { return false; }

		virtual const pl::wstring get_uri( ) const { return pl::wstring( L"map_reduce" ); }

		virtual const size_t slot_count( ) const { return 1; }

	protected:

		void decode_job ( ml::frame_type_ptr frame )
		{
			frame->get_image( );
			make_available( frame );
		}
 
		bool check_available( const int frameno )
		{
			boost::recursive_mutex::scoped_lock lock( mutex_ );
			return frame_map.find( frameno ) != frame_map.end( );
		}

		ml::frame_type_ptr wait_for_available( const int frameno )
		{
			boost::recursive_mutex::scoped_lock lock( mutex_ );
			/// TODO: timeout and error return - assume that completion will happen for now...
			while( !check_available( frameno ) )
				cond_.wait( lock );
			ml::frame_type_ptr result = frame_map[ frameno ];
			make_unavailable( frameno );
			return result;
		}

		void make_unavailable( const int frameno )
		{
			boost::recursive_mutex::scoped_lock lock( mutex_ );
			if ( check_available( frameno ) )
				frame_map.erase( frame_map.find( frameno ) );
		}

		void make_available( ml::frame_type_ptr frame )
		{
			boost::recursive_mutex::scoped_lock lock( mutex_ );
			frame_map[ frame->get_position( ) ] = frame;
			cond_.notify_all( );
		}

		void add_job( int frameno )
		{
			make_unavailable( frameno );

			ml::input_type_ptr input = fetch_slot( 0 );
			input->seek( frameno );
			ml::frame_type_ptr frame = input->fetch( );

			opencorelib::function_job_ptr fjob( new opencorelib::function_job( boost::bind( &filter_map_reduce::decode_job, this, frame ) ) );
			pool_->add_job( fjob );
		}

		void clear( )
		{
			boost::recursive_mutex::scoped_lock lock( mutex_ );
			frame_map.erase( frame_map.begin( ), frame_map.end( ) );
			pool_->clear_jobs( );
		}

		void do_fetch( frame_type_ptr &frame )
		{
			threads_ = prop_threads_.value< int >( );

			if ( last_frame_ == 0 )
			{
				thread_safe_ = fetch_slot( 0 ) && is_thread_safe( );
				if ( thread_safe_ )
				{
					boost::posix_time::time_duration timeout = boost::posix_time::seconds( 5 );
					pool_ = new cl::thread_pool( threads_, timeout );
				}
			}

			if ( last_frame_ && get_position( ) == last_frame_->get_position( ) )
			{
				frame = last_frame_;
				return;
			}
			else if ( !thread_safe_ )
			{
				last_frame_ = frame = fetch_from_slot( );
				return;
			}

			if ( last_frame_ == 0 || get_position( ) != expected_ )
			{
				incr_ = get_position( ) >= expected_ ? 1 : -1;
				clear( );
				frameno_ = get_position( );
				for(int i = 0; i < threads_ * 2 && frameno_ < get_frames( ) && frameno_ >= 0; i ++)
				{
					add_job( frameno_ );
					frameno_ += incr_;
				}
			}
		
			// Wait for the requested frame to finish
			frame = wait_for_available( get_position( ) );

			// Add more jobs to the pool
			int fillable = pool_->get_number_of_idle_workers( );

			while ( frameno_ >= 0 && frameno_ < get_frames( ) && fillable -- > 0 && frameno_ <= get_position( ) + 2 * threads_ )
			{
				add_job( frameno_ );
				frameno_ += incr_;
			}

			// Keep a reference to the last frame in case of a duplicated request
			last_frame_ = frame;
			expected_ = get_position( ) + incr_;
		}
};

//
// Plugin object
//

class ML_PLUGIN_DECLSPEC plugin : public openmedialib_plugin
{
public:
	virtual input_type_ptr input( const pl::wstring & )
	{
		return input_type_ptr( );
	}

	virtual store_type_ptr store( const pl::wstring &, const frame_type_ptr & )
	{
		return store_type_ptr( );
	}

	virtual filter_type_ptr filter( const pl::wstring &spec )
	{
		if ( spec == L"analyse" )
			return filter_type_ptr( new filter_analyse( ) );
		if ( spec == L"decode" )
			return filter_type_ptr( new filter_decode( ) );
		if ( spec == L"encode" )
			return filter_type_ptr( new filter_encode( ) );
		if ( spec == L"field_order" )
			return filter_type_ptr( new filter_field_order( ) );
		if ( spec.find( L"lazy:" ) == 0 )
			return filter_type_ptr( new filter_lazy( spec ) );
		if ( spec == L"map_reduce" )
			return filter_type_ptr( new filter_map_reduce( ) );
		if ( spec == L"rescale" || spec == L"aml_rescale" )
			return filter_type_ptr( new filter_rescale( ) );
		return ml::filter_type_ptr( );
	}
};

} } } }

//
// Access methods for openpluginlib
//

extern "C"
{
	ML_PLUGIN_DECLSPEC bool openplugin_init( void )
	{
		return true;
	}

	ML_PLUGIN_DECLSPEC bool openplugin_uninit( void )
	{
		return true;
	}
	
	ML_PLUGIN_DECLSPEC bool openplugin_create_plugin( const char*, pl::openplugin** plug )
	{
		*plug = new ml::decode::plugin;
		return true;
	}
	
	ML_PLUGIN_DECLSPEC void openplugin_destroy_plugin( pl::openplugin* plug )
	{ 
		delete static_cast< ml::decode::plugin * >( plug ); 
	}
}
