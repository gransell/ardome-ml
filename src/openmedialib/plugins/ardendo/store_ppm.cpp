#include "precompiled_headers.hpp"
#include "amf_filter_plugin.hpp"
#include "utility.hpp"
#include <iostream>
#include <boost/format.hpp>

namespace aml { namespace openmedialib {

class ML_PLUGIN_DECLSPEC store_ppm : public ml::store_type
{
	public:
		store_ppm( const pl::wstring &resource ) 
			: ml::store_type( )
			, resource_( resource )
			, file_( 0 )
			, pusher_( )
			, render_( )
		{
		}

		virtual ~store_ppm( )
		{ }

		bool init( )
		{
			pusher_ = ml::create_input( L"pusher:" );
			render_ = ml::create_filter( L"render" );

			if ( render_ )
				render_->connect( pusher_ );

			return true;
		}

		virtual bool push( ml::frame_type_ptr frame )
		{
			bool result = true;
			if ( frame && frame->get_image( ) )
			{
				std::string resource = pl::to_string( resource_ );

				if ( file_ == 0 )
				{
					if ( resource.find( "%" ) == std::string::npos )
					{
						file_ = fopen( resource.c_str( ), "wb" );
					}
					else
					{
						std::string name = ( boost::format( resource ) % frame->get_position( ) ).str( );
						file_ = fopen( name.c_str( ), "wb" );
					}
				}

				if ( render_ && frame->is_deferred( ) )
				{
					pusher_->push( frame );
					frame = render_->fetch( );
				}

				il::image_type_ptr img = frame->get_image( );
				if ( file_ != 0 )
				{
					img = il::convert( img, L"r8g8b8" );
					int p = img->pitch( );
					int w = img->width( );
					int h = img->height( );
					int l = img->linesize( );
					boost::uint8_t *ptr = img->data( );
					
					fprintf( file_, "P6\n%d %d\n255\n", w, h );
					while( h -- )
					{
						fwrite( ptr, l, 1, file_ );
						ptr += p;
					}

					fflush( file_ );

					if ( resource.find( "%" ) != std::string::npos )
					{
						fclose( file_ );
						file_ = 0;
					}
				}
				else
				{
					result = false;
				}
			}
			return result;
		}

		virtual ml::frame_type_ptr flush( )
		{ return ml::frame_type_ptr( ); }

		virtual void complete( )
		{ }

	protected:
		pl::wstring resource_;
		FILE *file_;
		ml::input_type_ptr pusher_;
		ml::input_type_ptr render_;
};

ml::store_type_ptr ML_PLUGIN_DECLSPEC create_store_ppm( const pl::wstring &resource )
{
	return ml::store_type_ptr( new store_ppm( resource ) );
}

} }