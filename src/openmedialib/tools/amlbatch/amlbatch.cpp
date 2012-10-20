
#ifdef NEEDS_SDL
#	include <SDL_keysym.h>
#endif

#include <iostream>
#include <fstream>
#include <sstream>
#include <deque>

#ifdef WIN32
#include <io.h>
#endif

#include <opencorelib/cl/core.hpp>
#include <opencorelib/cl/log_defines.hpp>
#include <opencorelib/cl/media_definitions.hpp>
#include <opencorelib/cl/media_time.hpp>
#include <openpluginlib/pl/openpluginlib.hpp>
#include <openimagelib/il/openimagelib_plugin.hpp>
#include <openmedialib/ml/openmedialib_plugin.hpp>
#include <openmedialib/ml/indexer.hpp>
#include <openmedialib/ml/audio_channel_extract.hpp>

#include <openpluginlib/pl/timer.hpp>
#include <boost/thread.hpp>

#include <opencorelib/cl/special_folders.hpp>

namespace ml = olib::openmedialib::ml;
namespace il = olib::openimagelib::il;
namespace pl = olib::openpluginlib;
namespace cl = olib::opencorelib;

#define NANO_SECS 1000000000
#define MICRO_SECS 1000000
#define MILLI_SECS 1000

void assign( pl::pcos::property_container &props, const std::string &name, const std::wstring &value )
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
		ARENFORCE_MSG( false, "Invalid property %1%" )( name );
	}
}

void assign( pl::pcos::property_container &props, const std::wstring &pair )
{
	size_t pos = pair.find( L"=" );
	std::string name = cl::str_util::to_string( pair.substr( 0, pos ) );
	std::wstring value = pair.substr( pos + 1 );

	if ( value[ 0 ] == L'"' && value[ value.size( ) - 1 ] == L'"' )
		value = value.substr( 1, value.size( ) - 2 );

	assign( props, name, value );
}

void handle_token( std::vector<ml::store_type_ptr> &result, pl::pcos::property_container &properties, const std::wstring &arg, ml::frame_type_ptr &frame )
{
	if ( arg.find( L"=" ) != std::wstring::npos )
	{
		assign( properties, arg );
	}
	else
	{
		ml::store_type_ptr st = ml::create_store( arg, frame );
		if( st )
		{
			result.push_back( st );
		}
		else
		{
			std::wcerr << L"Error: Could not find a plugin providing the store \"" << arg << L"\"" << std::endl;
			exit(-1);
		}
	}

	if ( !result.empty( ) )
		properties = result.back( )->properties( );
}
 
std::vector<ml::store_type_ptr> fetch_store( int &index, int argc, char **argv, ml::frame_type_ptr frame )
{
	std::vector< ml::store_type_ptr > result;
	pl::pcos::property_container properties;

	if ( index + 1 < argc && std::string( argv[ index ++ ] ) == "--" )
	{
		while( index < argc )
		{
			std::wstring arg = cl::str_util::to_wstring( argv[ index ++ ] );
			handle_token( result, properties, arg, frame );
		}
	}
	else if ( cl::str_util::env_var_exists( olib::t_string( _CT( "AML_STORE" ) ) ) )
	{
		olib::t_string store = cl::str_util::get_env_var( olib::t_string( _CT( "AML_STORE" ) ) );
		std::vector < olib::t_string > tokens;
		cl::str_util::split_string( store, olib::t_string( _CT( " " ) ), tokens, cl::quote::respect );
		for ( std::vector < olib::t_string >::iterator iter = tokens.begin( ); iter != tokens.end( ); iter ++ )
		{
			handle_token( result, properties, cl::str_util::to_wstring( *iter ), frame );
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
	for ( ml::exception_list::iterator i = list.begin( ); i != list.end( ); ++i )
		std::cerr << i->first->what( ) << " " << cl::str_util::to_string( i->second->get_uri( ) ) << std::endl;
}

void walk_and_assign( ml::input_type_ptr input, const std::string &name, int value )
{
	if ( input )
	{
		//Note: it is important that we assign to filters in upstream
		//and downwards order. The reason for this is that we might be
		//assigning the active property to a threader filter, which will
		//start it, making any modification to its connected graph
		//thread-unsafe.
		for ( size_t i = 0; i < input->slot_count( ); i ++ )
			walk_and_assign( input->fetch_slot( i ), name, value );

		pl::pcos::property prop = input->property( name.c_str( ) );
		if ( prop.valid( ) )
			prop = value;
	}
}

void walk_and_reconnect( ml::input_type_ptr input )
{
	if ( input )
	{
		for ( size_t i = 0; i < input->slot_count( ); i ++ )
		{
			walk_and_reconnect( input->fetch_slot( i ) );
			if ( input->get_uri( ) == L"fft_pitch" )
				input->connect( input->fetch_slot( i ), i );
		}
	}
}

void prepare_graph( ml::filter_type_ptr input, std::vector< ml::store_type_ptr > &store )
{
	bool defer = true;
	bool drop = true;

	for ( std::vector< ml::store_type_ptr >::iterator iter = store.begin( ); iter < store.end( ); ++iter )
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
	walk_and_reconnect( input );
	//Important: must assign active property last (see comment in walk_and_assign).
	if ( input->is_thread_safe( ) )
		walk_and_assign( input, "active", 1 );
}

void run( ml::input_type_ptr input )
{
	boost::system_time last_time = boost::get_system_time( );
	int frame_count = 0;
	double last_fps = 0.0;

	if ( input->is_thread_safe( ) )
		walk_and_assign( input, "active", 1 );

	input->sync( );
	int total_frames = input->get_frames( );

	for ( int i = 0; i < total_frames; i ++ )
	{
		boost::system_time curr_time = boost::get_system_time( );
		boost::posix_time::time_duration diff = curr_time - last_time;

		if ( i % 50 == 0 || i == total_frames - 1 )
		{
			input->sync( );
			total_frames = input->get_frames( );
		}

		// Check if more than one second has passed
		if( diff > boost::posix_time::seconds( 1 ) ) 
		{
			last_fps = frame_count / ( double( diff.total_milliseconds( ) ) / double( MILLI_SECS ) );
			frame_count = 0;
			last_time = curr_time;
		}

		std::cerr << i << "/" << total_frames << " fps: " << last_fps << "      \r";

		input->seek( i );
		ml::frame_type_ptr frame = input->fetch( );

		if ( !( frame && !frame->in_error( ) && ( frame->has_image( ) || frame->has_audio( ) || frame->get_stream( ) || frame->audio_block( ) ) ) )
			break;

		frame_count ++;
	}

	std::cerr << std::endl;
}

void play( ml::filter_type_ptr input, std::vector< ml::store_type_ptr > &store, bool interactive, int speed, bool stats, bool show_source_tc )
{
	bool error = false;
	std::vector< ml::store_type_ptr >::iterator iter;

	for ( iter = store.begin( ); !error && iter < store.end( ); ++iter )
		error = !( *iter )->init( );

	if ( error )
	{
		fprintf( stderr, "Store failed on init\n" );
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
	int sync_check = 0;
	bool syncing = true;
	int total_frames = 0;

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

		if ( ( syncing && (sync_check ++ % 50 == 0) ) || ( input->get_position( ) == total_frames - 1 ) )
		{
			input->sync( );
			int current_frames = input->get_frames( );
			//syncing = current_frames != total_frames;
			total_frames = current_frames;
		}

		if ( stats )
		{
			std::cerr << input->get_position( ) << "/" << total_frames <<  " fps: " << last_fps;

			if( show_source_tc && frame )
			{
				int fps_num, fps_den;
				frame->get_fps( fps_num, fps_den );
				cl::frame_rate::type fps_type = cl::frame_rate::get_type( cl::rational_time( fps_num, fps_den ) );
				if( fps_type != cl::frame_rate::undef )
				{
					std::cerr << " - ";
					pl::pcos::property source_timecode = frame->property( "source_timecode" );
					pl::pcos::property source_timecode_dropframe = frame->property( "source_timecode_dropframe" );
					if( source_timecode.valid() )
					{
						int tc_frames = source_timecode.value< int >();
						bool tc_dropframe = false;
						if( source_timecode_dropframe.valid() )
						{
							tc_dropframe = ( source_timecode_dropframe.value<int>() != 0 ? 1 : 0 );
						}

						T_CERR << cl::from_frame_number( fps_type, tc_frames ).to_time_code( fps_type, tc_dropframe, true ).to_string();
						T_CERR << _CT("(") << cl::frame_rate::to_string( fps_type ) << ( tc_dropframe ? _CT(":DF") : _CT(":NDF") ) << ")";
					}
				}
			}
			std::cerr << "                      \r";
		}

		frame_count += 1;

		if ( !frame || frame->get_position( ) != input->get_position( ) )
			frame = input->fetch( );

		if( !frame ) break;

		if ( frame->in_error( ) )
			report_frame_errors( frame );

		for ( iter = store.begin( ); !error && iter < store.end( ); ++iter )
		{
			error = !( *iter )->push( frame );
			pl::pcos::property keydown = ( *iter )->properties( ).get_property_with_string( "keydown" );
			if ( keydown.valid( ) )
			{
				int key = keydown.value< int >( );

				if ( key == SDLK_ESCAPE )
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
				else if ( key == 'g' )
				{
					int position = input->get_position( ) - 1000;
					input->seek( position < 0 ? 0 : position );
				}
				else if ( key == 'h' )
				{
					int position = input->get_position( ) + 1000;
					input->seek( position >= total_frames ? total_frames - 1 : position );
				}
				else if ( key == SDLK_UP )
				{
					int position = input->get_position( ) - 100;
					input->seek( position < 0 ? 0 : position );
				}
				else if ( key == SDLK_DOWN )
				{
					int position = input->get_position( ) + 100;
					input->seek( position >= total_frames ? total_frames - 1 : position );
				}
				else if ( key == SDLK_RIGHT )
				{
					if ( speed != 0 )
						speed = 0;
					else
						input->seek( 1, true );
				}
				else if ( key == SDLK_LEFT )
				{
					if ( speed != 0 )
						speed = 0;
					else if ( input->get_position( ) > 0 )
						input->seek( -1, true );
				}
				else if ( key == SDLK_END || key == 'e' )
				{
					input->seek( total_frames - 1 );
					input->sync( );
					total_frames = input->get_frames( );
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
			else if ( input->get_position( ) >= total_frames )
				input->seek( total_frames - 1 );
		}
		else
		{
			if ( !error )
				error = input->get_position( ) < 0 || input->get_position( ) >= total_frames;
		}
	}

	for ( iter = store.begin( ); iter < store.end( ); ++iter )
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
        
    #elif defined( __APPLE__ )
        olib::t_path plugins_path = cl::special_folder::get( cl::special_folder::plugins );
        pl::init( plugins_path.string( ) );
    #else
		setlocale(LC_CTYPE, "");
       	pl::init( );
    #endif

	pl::init_log( );
	//Allow us to get diagnostic messages if the aml_stack input fails to load, since
	//any log_level AML word on the command line has not taken effect yet at this point.
	pl::set_log_level( 4 );

	if ( argc > 1 )
	{
		bool interactive = false;
		ml::input_type_ptr input = ml::create_input( std::wstring( L"aml_stack:" ) );
		pl::pcos::property push = input->properties( ).get_property_with_string( "command" );
		pl::pcos::property execute = input->properties( ).get_property_with_string( "commands" );
		pl::pcos::property result = input->properties( ).get_property_with_string( "result" );

		//Set the log_level back to -1 (disabled), and let any log_level AML words control
		//the level from this point on.
		pl::set_log_level( -1 );

		bool should_seek = false;
		int seek_to = 0;
		bool stats = true;
		bool show_source_tc = false;

		int index = 1;

		std::list< std::wstring > tokens;

		while( index < argc )
		{
			std::wstring arg = cl::str_util::to_wstring( argv[ index ] );

			if ( arg == L"--seek" )
			{
				seek_to = atoi( argv[ ++ index ] );
				should_seek = true;
			}
			else if ( arg == L"--show-source-tc" )
			{
				show_source_tc = true;
			}
			else if ( arg == L"--no-stats" )
				stats = false;
			else if ( arg == L"--interactive" )
				interactive = true;
			else if ( arg == L"--" || arg == L"@@" ) {
				// Ensure "--" and not "@@"
				argv[ index ] = (char*)"--";
				break;
			} else
				tokens.push_back( arg );

			index ++;
		}

		// Execute the tokens
		execute = tokens;

		if ( result.value< std::wstring >( ) != L"OK" )
		{
			std::cerr << cl::str_util::to_string( result.value< std::wstring >( ) ) << std::endl;
			return 1;
		}

		// Dot the stack - by default this connects the input on the top of the stack
		// to the first slot of the stack input - if the input is <= 0 frames, then print
		// the inputs title (if it exists), otherwise just exit here (supports commands which
		// have no stack output [such as available] or are just a computation). Additionally
		// . itself can be overriden.
		push = std::wstring( L"." );

		if ( input->get_frames( ) <= 0 )
		{
			if ( input->fetch_slot( 0 ) )
				std::cerr << cl::str_util::to_string( input->fetch_slot( 0 )->get_uri( ) ) << std::endl;
			return 1;
		}

		// Allow us to just iterate through the graph if the last filter says so
		if ( input->fetch_slot( ) && input->fetch_slot( )->property( "@loop" ).valid( ) )
		{
			run( input );
			return 0;
		}

		// If a store is specified we use the input as is, otherwise we apply more filters
		if ( index == argc )
		{
			interactive = true;

			// Return the .'d item to the top of the stack for further manipuation
			push = std::wstring( L"recover" );

			// Apply default normalisation filters when no store is specified
			push = std::wstring( L"filter:resampler" );
			push = std::wstring( L"filter:visualise" );
			push = std::wstring( L"type=1" );
			push = std::wstring( L"colourspace=yuv420p" );
			push = std::wstring( L"width=640" );
			push = std::wstring( L"height=480" );
			push = std::wstring( L"filter:conform" );
			push = std::wstring( L"filter:deinterlace" );
			push = std::wstring( L"filter:threader" );
			push = std::wstring( L"queue=50" );

			// Dot the top of stack item again
			push = std::wstring( L"." );
		}

		// Check the frame count again just in case
		if ( input->get_frames( ) <= 0 )
			return 1;

		ml::filter_type_ptr pitch = ml::create_filter( L"pitch" );
		pitch->properties( ).get_property_with_string( "speed" ) = 1.0;
		pitch->connect( input );

		if ( should_seek )
		{
			pitch->sync( );
			if ( seek_to >= 0 )
				pitch->seek( seek_to );
			else
				pitch->seek( pitch->get_frames( ) + seek_to );
		}

		ml::frame_type_ptr frame = pitch->fetch( );
		if ( frame == 0 )
			return 2;

		if ( index == argc || strcmp( argv[ index + 1 ], "env:" ) )
		{
			std::vector< ml::store_type_ptr > store = fetch_store( index, argc, argv, frame );
			if ( store.empty( ) )
				return 3;

			play( pitch, store, interactive, should_seek ? 0 : 1, stats, show_source_tc );
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

	ml::uninit( );

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
