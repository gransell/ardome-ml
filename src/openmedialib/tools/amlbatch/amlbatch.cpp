
#ifdef NEEDS_SDL
#	include <SDL.h>
#endif

#include <iostream>
#include <fstream>
#include <deque>

#include <openpluginlib/pl/openpluginlib.hpp>
#include <openpluginlib/pl/utf8_utils.hpp>
#include <openimagelib/il/openimagelib_plugin.hpp>
#include <openmedialib/ml/openmedialib_plugin.hpp>
#include <openmedialib/ml/indexer.hpp>

#include <openpluginlib/pl/timer.hpp>
#include <boost/thread.hpp>

namespace ml = olib::openmedialib::ml;
namespace il = olib::openimagelib::il;
namespace pl = olib::openpluginlib;

#define NANO_SECS 1000000000
#define MICRO_SECS 1000000
#define MILLI_SECS 1000

void assign( pl::pcos::property_container &props, const std::string name, const pl::wstring value )
{
	pl::pcos::property property = props.get_property_with_string( name.c_str( ) );
	if ( property.valid( ) )
	{
		property.set_from_string( value );
		property.update( );
	}
	else if ( name[ 0 ] == '@' )
	{
		pl::pcos::property prop( pl::pcos::key::from_string( name.c_str( ) ) );
		props.append( prop = value );
	}
	else
	{
		throw( std::string( "Invalid property " ) + name );
	}
}

void assign( pl::pcos::property_container &props, const pl::wstring pair )
{
	size_t pos = pair.find( L"=" );
	std::string name = pl::to_string( pair.substr( 0, pos ) );
	pl::wstring value = pair.substr( pos + 1 );

	if ( value[ 0 ] == L'"' && value[ value.size( ) - 1 ] == L'"' )
		value = value.substr( 1, value.size( ) - 2 );

	assign( props, name, value );
}

std::vector<ml::store_type_ptr> fetch_store( int &index, int argc, char **argv, ml::frame_type_ptr frame )
{
	std::vector< ml::store_type_ptr > result;
	pl::pcos::property_container properties;

	if ( index + 1 < argc && std::string( argv[ index ++ ] ) == "--" )
	{
		while( index < argc )
		{
			pl::wstring arg = pl::to_wstring( argv[ index ++ ] );

			if ( arg.find( L"=" ) != pl::wstring::npos )
				assign( properties, arg );
			else
				result.push_back( ml::create_store( arg, frame ) );

			if ( result.size( ) )
				properties = result.back( )->properties( );
		}
	}
	else
	{
		result.push_back( ml::create_store( L"preview:", frame ) );
	}

	return result;
}

bool toggle_fullscreen( const std::vector< ml::store_type_ptr >& stores, bool is_full  )
{
    std::vector< ml::store_type_ptr >::const_iterator iter( stores.begin()), eit(stores.end());
    for( ; iter != eit ; ++iter )
    {
        pl::pcos::property fullscreen = ( *iter )->properties( ).get_property_with_string( "fullscreen" );
        if( !fullscreen.valid() )
        {
            pl::pcos::property video_store_prop =
                (*iter)->properties().get_property_with_string("video_store");

            if( video_store_prop.valid() )
            {
                ml::store_type_ptr video_store = video_store_prop.value< ml::store_type_ptr >( );
                fullscreen = video_store->properties().get_property_with_string( "fullscreen" );
            }
        }

        if( fullscreen.valid() ) 
        {
            fullscreen = is_full ? 0 : 1;
        }
    }
    return !is_full;
}

void report_frame_errors( ml::frame_type_ptr &frame )
{
	ml::exception_list list = frame->exceptions( );
	for ( ml::exception_list::iterator i = list.begin( ); i != list.end( ); i ++ )
		std::cerr << i->first->what( ) << " " << pl::to_string( i->second->get_uri( ) ) << std::endl;
}

void walk_and_assign( ml::input_type_ptr input, std::string name, int value )
{
	if ( input )
	{
		pl::pcos::property prop = input->property( name.c_str( ) );
		if ( prop.valid( ) )
			prop = value;
		for ( size_t i = 0; i < input->slot_count( ); i ++ )
			walk_and_assign( input->fetch_slot( i ), name, value );
	}
}

void prepare_graph( ml::filter_type_ptr input, std::vector< ml::store_type_ptr > &store )
{
	bool defer = true;
	bool drop = true;

	for ( std::vector< ml::store_type_ptr >::iterator iter = store.begin( ); iter < store.end( ); iter ++ )
	{
		if ( defer )
		{
			pl::pcos::property deferrable = ( *iter )->property( "deferrable" );
			defer = deferrable.valid( ) && deferrable.value< int >( );
		}
		if ( drop )
		{
			pl::pcos::property droppable = ( *iter )->property( "droppable" );
			drop = droppable.valid( ) && droppable.value< int >( );
		}
	}

	walk_and_assign( input, "deferred", defer ? 1 : 0 );
	walk_and_assign( input, "dropping", drop ? 1 : 0 );
}

void play( ml::filter_type_ptr input, std::vector< ml::store_type_ptr > &store, bool interactive, int speed, bool stats )
{
	bool error = false;
	std::vector< ml::store_type_ptr >::iterator iter;

	for ( iter = store.begin( ); !error && iter < store.end( ); iter ++ )
		error = !( *iter )->init( );

	if ( error )
	{
		fprintf( stderr, "Failed on init\n" );
		return;
	}

	prepare_graph( input, store );

	double pitch = 1.0;

	// Variables used to compute the frames per second value.
    boost::system_time last_time = boost::get_system_time();
	int frame_count(0);
	double last_fps(0.0f);
	int store_position = 0;
    bool fullscreen = false;

	ml::frame_type_ptr frame;

	while( !error )
	{

        boost::system_time curr_time = boost::get_system_time();
        boost::posix_time::time_duration diff = curr_time - last_time;
        if( diff > boost::posix_time::seconds(1) ) 
		{
			// More than one second has passed
			last_fps = frame_count / (double)( diff.total_milliseconds() / (double)(MILLI_SECS) );
			frame_count = 0;
			last_time = curr_time;
		}

		if ( stats )
			std::cerr << input->get_position( ) << "/" << input->get_frames( ) <<  " fps: " << last_fps << '\r';

		frame_count += 1;

		if ( !frame || frame->get_position( ) != input->get_position( ) )
			frame = input->fetch( );

		if( !frame ) break;

		if ( frame->in_error( ) )
			report_frame_errors( frame );

		for ( iter = store.begin( ); !error && iter < store.end( ); iter ++ )
		{
			error = !( *iter )->push( frame );
			pl::pcos::property keydown = ( *iter )->properties( ).get_property_with_string( "keydown" );
			if ( keydown.valid( ) )
			{
				int key = keydown.value< int >( );

				if ( key == 27 )
				{
					error = true;
				}
				else if ( key == 'j' || key == 'J' )
				{
					if ( speed != -1 )
						speed = -1;
					else if ( pitch < 5.0 )
						pitch += 0.1;
				}
				else if ( key == 'k' || key == 'K' )
				{
					speed = speed != 0 ? 0 : 1;
					pitch = 1.0;
				}
				else if ( key == 'l' || key == 'L' )
				{
					if ( speed != 1 )
						speed = 1;
					else if ( pitch < 5.0 )
						pitch += 0.1;
				}
				else if ( key == 's' )
				{
					store_position = frame->get_position( );
				}
                else if ( key == 'f' || key == 'F' )
                {
                    fullscreen = toggle_fullscreen( store, fullscreen );
                }
				else if ( key == 'a' )
				{
					input->seek( store_position );
				}
				else if ( key == '0' )
				{
					input->seek( 0 - speed );
				}
				else if ( key == 273 )
				{
					int position = input->get_position( ) - 100;
					input->seek( position < 0 ? 0 : position );
				}
				else if ( key == 274 )
				{
					int position = input->get_position( ) + 100;
					input->seek( position >= input->get_frames( ) ? input->get_frames( ) - 1 : position );
				}
				else if ( key == 275 )
				{
					if ( speed != 0 )
						speed = 0;
					else
						input->seek( 1, true );
				}
				else if ( key == 276 )
				{
					if ( speed != 0 )
						speed = 0;
					else if ( input->get_position( ) > 0 )
						input->seek( -1, true );
				}
				else if ( key == 279 )
				{
					input->seek( input->get_frames( ) - 1 );
				}

				if ( key != 0 )
				{
					pl::pcos::property p = input->properties( ).get_property_with_string( "speed" );
					if ( p.valid( ) )
						p = pitch;
				}

				keydown.set( 0 );
			}
		}

		input->seek( speed, true );

		if ( interactive )
		{
			if ( input->get_position( ) < 0 )
				input->seek( 0 );
			else if ( input->get_position( ) >= input->get_frames( ) )
				input->seek( input->get_frames( ) - 1 );
		}
		else
		{
			if ( !error )
				error = input->get_position( ) < 0 || input->get_position( ) >= input->get_frames( );
		}
	}

	for ( iter = store.begin( ); iter < store.end( ); iter ++ )
		( *iter )->complete( );

	walk_and_assign( input, "active", 0 );

	if ( stats )
		std::cerr << std::endl;
}

void env_report( int argc, char *argv[ ], ml::input_type_ptr input, ml::frame_type_ptr frame )
{
	std::cout << "AML_FRAME_RATE=" << frame->get_fps_num( ) << ":" << frame->get_fps_den( ) << std::endl;
	std::cout << "AML_FRAMES=" << input->get_frames( ) << std::endl;
	std::cout << "AML_DURATION=" << double( input->get_frames( ) ) * frame->get_fps_den( ) / frame->get_fps_num( ) << std::endl;
	std::cout << "AML_HAS_IMAGE=" << ( frame->get_image( ) != 0 ) << std::endl;
	std::cout << "AML_HAS_AUDIO=" << ( frame->get_audio( ) != 0 ) << std::endl;
	if ( frame->get_image( ) )
	{
		std::cout << "AML_IMAGE_SAR=" << frame->get_sar_num( ) << ":" << frame->get_sar_den( ) << std::endl;
		std::cout << "AML_IMAGE_SIZE=" << frame->get_image( )->width( ) << "x" << frame->get_image( )->height( ) << std::endl;
	}
}

bool uses_stdin( )
{
#ifndef WIN32
	return !isatty( fileno( stdin ) );
#else
	return !_isatty( fileno( stdin ) );
#endif
}

int main( int argc, char *argv[ ] )
{
    #ifdef WIN32
        HMODULE mod_hand = GetModuleHandle(L"amlbatch.exe");
        char mod_name [255];
        if( ::GetModuleFileNameA( mod_hand, mod_name, 255) == 0 )
        {
            std::cout << "Failed to get the path to amlbatch.exe\n";
            return -1;
        }

        std::string a_path(mod_name);
        std::string::size_type pos = a_path.find("amlbatch.exe");
        if( pos == std::string::npos ) 
        {
            std::cout << "Expecting the program that started to be called amlbatch.exe.\n";
            std::cout << "amlbatch.exe must be located in a directory which have a subdir called aml-plugins\n";
            return -1;
        }
        
        std::string a_path2 = a_path.substr(0, pos);
        a_path2 += "aml-plugins";

        try
        {
            if( ! pl::init( a_path2 ) )
            {
                std::cout << "aml-plugins location: " << a_path2 << std::endl; 
                std::cout << "Failed to initialize ardome-ml\n";
                return -1;
            }
        }
        catch ( std::exception& e)
        {
            std::cout << "pl::init failed\n";
            std::cout << e.what() << std::endl;    
        }
        catch( char* msg )
        {
            std::cout << "pl::init failed error:" << msg << std::endl; 
            return -1;
        }
        catch( ... )
        {
            std::cout << "pl::init failed unknown error" << std::endl; 
            return -1;
        }
        
    #else
		setlocale(LC_CTYPE, "");
       	pl::init( );
    #endif

	// Initialise the indexer
	ml::indexer_init( );

	if ( argc > 1 || uses_stdin( ) )
	{
		bool interactive = false;
		ml::input_type_ptr input = ml::create_input( pl::wstring( L"aml_stack:" ) );
		pl::pcos::property push = input->properties( ).get_property_with_string( "command" );
		pl::pcos::property execute = input->properties( ).get_property_with_string( "commands" );
		pl::pcos::property result = input->properties( ).get_property_with_string( "result" );
		int seek_to = -1;
		bool stats = true;

		int index = 1;

		pl::wstring_list tokens;

		if ( uses_stdin( ) )
			tokens.push_back( L"stdin:" );

		while( index < argc )
		{
			pl::wstring arg = pl::to_wstring( argv[ index ] );

			if ( arg == L"--seek" )
				seek_to = atoi( argv[ ++ index ] );
			else if ( arg == L"--no-stats" )
				stats = false;
			else if ( arg == L"--interactive" )
				interactive = true;
			else if ( arg == L"--" )
				break;
			else
				tokens.push_back( arg );

			index ++;
		}

		// Execute the tokens
		execute = tokens;

		if ( result.value< pl::wstring >( ) != L"OK" )
		{
			std::cerr << pl::to_string( result.value< pl::wstring >( ) ) << std::endl;
			return 1;
		}

		// Duplicate top of stack and dot it - if the duped input is <= 0 frames, then print
		// the inputs title (if it exists), otherwise just exit here (supports commands which
		// have no stack output [such as available] or are just a computation)
		push = pl::wstring( L"0" );
		push = pl::wstring( L"pick" );
		push = pl::wstring( L"." );

		if ( input->get_frames( ) <= 0 )
		{
			if ( input->fetch_slot( 0 ) )
				std::cerr << pl::to_string( input->fetch_slot( 0 )->get_uri( ) ) << std::endl;
			return 1;
		}

		if ( index == argc )
		{
			interactive = true;
			push = pl::wstring( L"filter:resampler" );
			push = pl::wstring( L"filter:visualise" );
			push = pl::wstring( L"colourspace=yuv420p" );
			push = pl::wstring( L"width=640" );
			push = pl::wstring( L"height=480" );
			push = pl::wstring( L"filter:conform" );
			push = pl::wstring( L"filter:deinterlace" );
			push = pl::wstring( L"filter:threader" );
			push = pl::wstring( L"queue=50" );
			push = pl::wstring( L"active=1" );
		}

		push = pl::wstring( L"." );

		if ( input->get_frames( ) <= 0 )
			return 1;

		ml::filter_type_ptr pitch = ml::create_filter( L"pitch" );
		pitch->properties( ).get_property_with_string( "speed" ) = 1.0;
		pitch->connect( input );

		if ( seek_to >= 0 )
			pitch->seek( seek_to );

		ml::frame_type_ptr frame = pitch->fetch( );
		if ( frame == 0 )
			return 2;

		if ( index == argc || strcmp( argv[ index + 1 ], "env:" ) )
		{
			std::vector< ml::store_type_ptr > store = fetch_store( index, argc, argv, frame );
			if ( store.size( ) == 0 )
				return 3;

			play( pitch, store, interactive, seek_to < 0 ? 1 : 0, stats );
		}
		else
		{
			env_report( argc, argv, pitch, frame );
		}
	}
	else
	{
		std::cerr << "Usage: amlbatch <graph> [ -- ( <store> )* ]" << std::endl;
	}

	return 0;
}

#ifdef WIN32
#include <tchar.h>
int _tmain(int argc, wchar_t* argv[])
{
    std::vector< char* > converted;
    for(int i = 0; i < argc ; ++i )
    {
        int arg_i_size = static_cast<int>(wcslen(argv[i]));
        char* buf = new char[arg_i_size+1];
        for( int j = 0; j < arg_i_size; ++j )
            buf[j] = (char)(argv[i][j]);

        buf[arg_i_size] = 0;

        converted.push_back(buf);
    }

    main( static_cast<int>(converted.size()), &converted[0] );

    for(int i = 0; i < argc ; ++i )
    {
        delete [] converted[i];
    }
}

#endif
