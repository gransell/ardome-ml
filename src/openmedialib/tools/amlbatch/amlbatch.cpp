
#ifdef NEEDS_SDL
#	include <SDL_keysym.h>
#endif

#include <iostream>
#include <fstream>
#include <sstream>
#include <deque>

#include <signal.h>

#ifndef WIN32
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <termios.h>
#else
#include <conio.h>
#include <io.h>
#endif

#include <opencorelib/cl/core.hpp>
#include <opencorelib/cl/log_defines.hpp>
#include <opencorelib/cl/media_definitions.hpp>
#include <opencorelib/cl/media_time.hpp>
#include <openpluginlib/pl/openpluginlib.hpp>
#include <openmedialib/ml/openmedialib_plugin.hpp>
#include <openmedialib/ml/indexer.hpp>
#include <openmedialib/ml/audio_channel_extract.hpp>

#include <openpluginlib/pl/timer.hpp>
#include <boost/thread.hpp>

#include <opencorelib/cl/special_folders.hpp>

namespace ml = olib::openmedialib::ml;
namespace pl = olib::openpluginlib;
namespace cl = olib::opencorelib;

#define NANO_SECS 1000000000
#define MICRO_SECS 1000000
#define MILLI_SECS 1000

#ifndef WIN32
#define clear_to_eol	char( 27 ) << "[K\r" 
#else
#define clear_to_eol	"                \r" 
#endif

#ifndef WIN32
static struct termios oldtty;
static int restore_tty = 0;
#endif

#ifdef WIN32
#define AMLBATCH_MAIN utf8_main
#else
#define AMLBATCH_MAIN main
#endif

//For Unicode support on Windows, wmain needs to be used (which takes
//UTF-16 command line args). We convert the command-line args to UTF-8,
//then call the Unix-style UTF-8 main.
#ifdef WIN32

int AMLBATCH_MAIN( int argc, const char *argv[ ] );

int wmain( int argc, const wchar_t *argv[ ] )
{
	std::vector< std::string > converted_args;
	converted_args.reserve( argc );
	std::vector< const char * > converted_args_raw;
	converted_args_raw.reserve( argc );
	for( int i = 0; i < argc; ++i )
	{
		converted_args.push_back( cl::str_util::to_string( argv[ i ] ) );
		converted_args_raw.push_back( converted_args[ i ].c_str() );
	}

	return AMLBATCH_MAIN( argc, &converted_args_raw[ 0 ] );
}

#endif

static void term_exit(void)
{
#ifndef WIN32
	if( restore_tty )
	{
		tcsetattr( 0, TCSANOW, &oldtty );
		restore_tty = 0;
	}
#endif
}

static volatile int sigterm_count = 0;

static void sigterm_handler( int sig )
{
	if ( sigterm_count ++ == 1 )
	{
		term_exit( );
		exit( 1 );
	}
}

static void term_init(void)
{
#ifndef WIN32
	struct termios tty;

	if ( tcgetattr( 0, &tty ) == 0) 
	{
		oldtty = tty;
		restore_tty = 1;
		atexit(term_exit);
		tty.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL|IXON);
		tty.c_oflag |= OPOST;
		tty.c_lflag &= ~(ECHO|ECHONL|ICANON|IEXTEN);
		tty.c_cflag &= ~(CSIZE|PARENB);
		tty.c_cflag |= CS8;
		tty.c_cc[VMIN] = 1;
		tty.c_cc[VTIME] = 0;

		tcsetattr (0, TCSANOW, &tty);
	}
	signal( SIGQUIT, sigterm_handler ); /* Quit (POSIX).  */
#endif
	signal( SIGINT, sigterm_handler ); /* Interrupt (ANSI).	*/
	signal( SIGTERM, sigterm_handler ); /* Termination (ANSI).  */
}

/* read a key without blocking */
static int read_key( )
{
	unsigned char ch = 0;

#ifndef WIN32

	if ( restore_tty )
	{
		fd_set rfds;
		FD_ZERO( &rfds );
		FD_SET( 0, &rfds );
		struct timeval tv;
		tv.tv_sec = 0;
		tv.tv_usec = 0;
		int n = select( 1, &rfds, NULL, NULL, &tv );
		if ( n > 0 ) 
		{
			n = read( 0, &ch, 1 );
			ch = n == 1 ? ch : 0;
		}
	}

#else

	if ( kbhit( ) )
		ch = getch( );

#endif
	return ch != 0 ? ch : -1;
}

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
 
std::vector<ml::store_type_ptr> fetch_store( int &index, int argc, const char *argv[], ml::frame_type_ptr frame )
{
	std::vector< ml::store_type_ptr > result;
	pl::pcos::property_container properties;

	if ( index < argc && std::string( argv[ index ] ) == "--" )
	{
		++index;
		while( index < argc )
		{
			std::wstring arg = cl::str_util::to_wstring( argv[ index ] );
			handle_token( result, properties, arg, frame );
			++index;
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

void run( ml::input_type_ptr input, bool interactive, bool stats )
{
	boost::system_time last_time = boost::get_system_time( );
	int frame_count = 0;
	double last_fps = 0.0;

	if ( input->is_thread_safe( ) )
		walk_and_assign( input, "active", 1 );

	input->sync( );

	int total_frames = input->get_frames( );
	bool running = true;
	bool trickplay = input->is_seekable( ) ? interactive || input->fetch_slot( )->property( "@loop" ).value< std::wstring >( ) == std::wstring( L"2" ) : false;
	int speed = 1;

	term_init( );

	for ( int i = 0; running && sigterm_count == 0 && i < total_frames; )
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

		if ( stats )
			std::cerr << i << "/" << total_frames << " fps: " << last_fps << clear_to_eol;

		input->seek( i );
		ml::frame_type_ptr frame = input->fetch( );

		if ( !( frame && !frame->in_error( ) && ( frame->has_image( ) || frame->has_audio( ) || frame->get_stream( ) || frame->audio_block( ) ) ) )
			break;

		frame_count ++;

		if ( trickplay )
		{
			int last_frame = i;

			switch( read_key( ) )
			{
				case 27:
				case 'q':
					running = false;
					break;

				case 'j':
					speed = speed >= 0 ? -1 : speed * 2;
					break;

				case 'k':
				case ' ':
					speed = speed == 0 ? 1 : 0;
					break;

				case 'l':
					speed = speed <= 0 ? 1 : speed * 2;
					break;

				case 'J':
					speed = -25;
					break;

				case 'L':
					speed = 25;
					break;

				case '0':
					i = 0 - speed;
					break;

				case 'e':
					i = total_frames - 1 - speed;
					break;
			}

			i = cl::utilities::clamp( i + speed, 0, total_frames - 1 );

			if ( i == last_frame )
				boost::this_thread::sleep( boost::posix_time::milliseconds( int( 1000 / frame->fps( ) ) ) );
		}
		else
		{
			i ++;
		}
	}

	walk_and_assign( input, "active", 0 );

	if ( stats )
		std::cerr << std::endl;

	term_exit( );
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

	term_init( );

	while( !error && sigterm_count == 0 )
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
			std::cerr << clear_to_eol;
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

	term_exit( );
}

void env_report( int argc, const char *argv[ ], ml::input_type_ptr input, ml::frame_type_ptr frame )
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

int real_main( int argc, const char *argv[ ] )
{
	pl::init_log( );

	//Allow us to get diagnostic messages if the aml_stack input fails to load, since
	//any log_level AML word on the command line has not taken effect yet at this point.
	pl::set_log_level( 4 );

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
		if ( input->fetch_slot( 0 ) ) {
			std::cerr << cl::str_util::to_string( input->fetch_slot( 0 )->get_uri( ) ) << std::endl;
		} else {
			std::cerr << "No input specified" << std::endl;
		}
		return 1;
	}

	// Allow us to just iterate through the graph if the last filter says so
	if ( input->fetch_slot( ) && input->fetch_slot( )->property( "@loop" ).valid( ) )
	{
		run( input, interactive, stats );
	}
	else
	{
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

		if ( index + 1 < argc && strcmp( argv[ index + 1 ], "env:" ) == 0 )
		{
			env_report( argc, argv, pitch, frame );
		}
		else
		{
			std::vector< ml::store_type_ptr > stores = fetch_store( index, argc, argv, frame );
			if ( stores.empty( ) )
			{
				std::cerr << "One or more stores must be specified after the \"--\" token" << std::endl;
				return 3;
			}
			play( pitch, stores, interactive, should_seek ? 0 : 1, stats, show_source_tc );
		}
	}
	return 0;
}

int AMLBATCH_MAIN( int argc, const char *argv[ ] )
{
	if ( argc <= 1 )
	{
		std::cerr << "Usage: amlbatch <graph> [ -- ( <store> )* ]" << std::endl;
		return 0;
	}

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
            std::cout << "amlbatch.exe must be located in a directory which has a subdir called aml-plugins\n";
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

	std::string *exception_msg = 0;
	int return_code = 0;
	try {
		return_code = real_main (argc, argv);
	}
	catch ( std::exception& e)
	{
		exception_msg = new std::string(e.what());
	}
	catch ( char* msg )
	{
		exception_msg = new std::string(msg);
	}
	catch ( ... )
	{
		exception_msg = new std::string("Unknown");
	}

	ml::uninit( );

	if ( exception_msg )
	{
		std::cerr.flush();
		std::cerr << "\nLast exception:\n";
		std::cerr << "--------------------------------------------------------------\n";
		std::cerr << *exception_msg << std::endl;
		std::cerr << "--------------------------------------------------------------\n";
		std::cerr << "Terminated because of exception\n";

		delete exception_msg;
		return_code = 4;
	}

	return return_code;
}
