#include "precompiled_headers.hpp"
#include "amf_filter_plugin.hpp"
#include "utility.hpp"

#include <map>

#include <boost/thread/recursive_mutex.hpp>
#include <boost/thread/condition.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/xtime.hpp>

#include <iostream>

#ifdef WIN32
#pragma warning(push)
#pragma warning(disable:4355)
#endif

namespace aml { namespace openmedialib {

class filter_pulldown : public ml::filter_type
{
	public:
		// Filter_type overloads
		explicit filter_pulldown( const pl::wstring & )
			: ml::filter_type( )
			, total_src_frames_( 0 )
			, total_frames_( 0 )
		{
		}

		virtual bool requires_image( ) const { return true; }

		virtual const pl::wstring get_uri( ) const { return L"pulldown"; }

		virtual int get_frames( ) const { return total_frames_; }

	protected:
		void do_fetch( ml::frame_type_ptr &result )
		{
			ml::input_type_ptr input = fetch_slot( 0 );
			int src_frame = get_position( ) * 4 / 5;
			src_frame -= src_frame % 4;

			if ( map_.find( src_frame ) == map_.end( ) )
			{
				ml::frame_type_ptr frame;
				int index = src_frame;
				map_.erase( map_.begin( ), map_.end( ) );
				while( map_.size( ) < 4 && index < total_src_frames_ )
				{
					input->seek( index );
					frame = input->fetch( );
					frame->get_image( );
					map_[ index ++ ] = ml::frame_type_ptr( new ml::frame_type( frame ) );
				}

				while( map_.size( ) < 4 )
					map_[ index ++ ] = frame;
			}

			switch( get_position( ) % 5 )
			{
				case 0: result = map_[ src_frame ]; break;
				case 1: result = map_[ src_frame + 1 ]; break;
				case 2: result = merge( map_[ src_frame + 1 ], map_[ src_frame + 2 ] ); break;
				case 3: result = merge( map_[ src_frame + 2 ], map_[ src_frame + 3 ] ); break;
				case 4: result = map_[ src_frame + 3 ]; break;
			}

			result = ml::frame_type_ptr( new ml::frame_type( result ) );
			result->set_position( get_position( ) );
		}

		virtual void sync_frames( )
		{
			ml::input_type_ptr input = fetch_slot( 0 );
			total_src_frames_ = input->get_frames( );
			total_frames_ = total_src_frames_ * 5 / 4;
		}

		ml::frame_type_ptr merge( ml::frame_type_ptr frame1, ml::frame_type_ptr frame2 )
		{
			il::image_type_ptr image1 = frame1->get_image( );
			il::image_type_ptr image2 = frame2->get_image( );

			if ( image1->field_order( ) == il::progressive || image2->field_order( ) == il::progressive )
			{
				image1->set_field_order( il::top_field_first );
				image2->set_field_order( il::top_field_first );
			}

			il::image_type_ptr field1 = il::field( image1, 1 );
			il::image_type_ptr field2 = il::field( image2, 0 );

			ml::frame_type_ptr result = ml::frame_type_ptr( new ml::frame_type( frame1 ) );
			result->set_image( merge_fields( field1, field2 ) );

			return result;
		}

		il::image_type_ptr merge_fields( il::image_type_ptr field0, il::image_type_ptr field1 )
		{
			il::image_type_ptr image;
			if ( field0 && field1 )
			{
				image = il::allocate( field0->pf( ), field0->width( ), field0->height( ) * 2 );

				for ( int p = 0; p < image->plane_count( ); p ++ )
				{
					boost::uint8_t *dest_ptr = image->data( p );
					boost::uint8_t *field0_ptr = field0->data( p );
					boost::uint8_t *field1_ptr = field1->data( p );
					int field0_pitch = field0->pitch( p );
					int field1_pitch = field1->pitch( p );
					int linesize = image->linesize( p );
					int pitch = image->pitch( p );
					int height = field0->height( p );
	
					while( height -- )
					{
						memcpy( dest_ptr, field0_ptr, linesize );
						dest_ptr += pitch;
						field0_ptr += field0_pitch;
						memcpy( dest_ptr, field1_ptr, linesize );
						dest_ptr += pitch;
						field1_ptr += field1_pitch;
					}
				}
			}

			return image;
		}

	private:
		std::map< int, ml::frame_type_ptr > map_;
		int total_src_frames_;
		int total_frames_;
};

ml::filter_type_ptr create_pulldown( const pl::wstring &spec )
{
	return ml::filter_type_ptr( new filter_pulldown( spec ) );
}

} }

