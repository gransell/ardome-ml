
// ml - A media library representation.

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

// simple example to exercise the load and store features of ml

#include <cstdio>
#include <cstdlib>
#include <string>
#include <iostream>

#ifdef HAVE_CONFIG_H
#include <openlibraries_global_config.hpp>
#endif

#include <openpluginlib/pl/openpluginlib.hpp>
#include <openpluginlib/pl/utf8_utils.hpp>

#include <openmedialib/ml/openmedialib_plugin.hpp>

namespace ml  = olib::openmedialib::ml;
namespace opl = olib::openpluginlib;

#ifdef WIN32
	const opl::string oml_plugin_path( "./plugins" );
	const opl::string oil_plugin_path( "./plugins" );
#else
	const opl::string oil_plugin_path( OPENIMAGELIB_PLUGINS );
	const opl::string oml_plugin_path( OPENMEDIALIB_PLUGINS );
#endif




class handler : public ml::store_keyboard_handler
{
	public:
		handler( ml::input_type_ptr input, ml::store_type_ptr store )
			: input_( input )
			, store_( store )
			, speed_( 1 )
			, done_( false )
		{
			ml::store_keyboard_feedback *feedback = dynamic_cast< ml::store_keyboard_feedback * >( store.get( ) );
			if ( feedback )
				feedback->register_keyboard_handler( this );
		}

		virtual ~handler( ) 
		{ 
		}

		void flush( int offset = 0 )
		{
			ml::frame_type_ptr frame = store_->flush( );
			if ( frame )
				input_->seek( int( frame->get_pts( ) * frame->fps( ) + offset + 0.5 ) );
		}

		virtual void keyboard_handler( unsigned char key )
		{ 
			
			//boost::mutex::scoped_lock scoped_lock( mutex_ );

			switch( key )
			{

				case 27:
					done_ = true;
					break;
		
				case ' ':
					speed_ = !speed_;
					flush( );
					break;
		
				case 'h':
					speed_ = 0;
					flush( -1 );
					break;
		
				case 'l':
					speed_ = 0;
					flush( 1 );
					break;
		
				case 'j':
					flush( );
					input_->seek( ( int )input_->get_frames( ) );
					break;
		
				case 'k':
					flush( );
					input_->seek( 0 );
					break;

				default:
					break;
			}
		}

		void run( )
		{
			int end=input_->get_frames( ) - 1;
			int pos=input_->get_position( );
			
			while( !done_ && pos < end )
			{
				pos = input_->get_position( );

				ml::frame_type_ptr frame = input_->fetch( );
				if ( !store_->push( frame ) )
					break; 

				input_->seek( speed_, true );
			}

			store_->complete( );
		}


	private:
		ml::input_type_ptr input_;
		ml::store_type_ptr store_;

		int speed_;
		bool done_;
};

int main( int argc, char* argv[ ] )
{
	// Load both oil and oml plugins since oml uses oil
	opl::init( "" );

	if ( argc > 2 )
	{
		ml::input_type_ptr input = ml::create_input( argv[ 1 ] );
		if ( input == 0 ) 
			return 1;

		ml::store_type_ptr store = ml::create_store( argv[ 2 ], input->fetch( ) );
		if ( store == 0 ) 
			return 2;

		if ( store->init( ) )
		{
			handler handler( input, store );
			handler.run( );
		}
	}
	else
	{
		std::cerr << "Usage: store input store" << std::endl;
		std::cerr << "   ie: file.avi file.mpg" << std::endl;
	}

	opl::uninit( );

	return 0;
}

