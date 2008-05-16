#ifndef PL_LOG_INC_
#define PL_LOG_INC_

#if _MSC_VER
#pragma warning ( push )
#	pragma warning ( disable:4251 )
#endif

#include <string>
#include <openpluginlib/pl/config.hpp>
#include <boost/format.hpp>

namespace olib { namespace openpluginlib {

#ifdef _MSC_VER
    #define PL_CURRENT_FUNC_NAME __FUNCDNAME__
#else 
    #define PL_CURRENT_FUNC_NAME __FUNCTION__
#endif

// Typedef for logger
typedef void ( *logger )( const char *, long, const char *, int, std::string );

// Set the log callback
OPENPLUGINLIB_DECLSPEC void set_log_callback( logger );

// Set the log level
OPENPLUGINLIB_DECLSPEC void set_log_level( int );

// Return the log level
OPENPLUGINLIB_DECLSPEC int log_level( );

// Log methods
OPENPLUGINLIB_DECLSPEC void log( const char *filename, long linenr, const char *funcname, int level, boost::format exp );
OPENPLUGINLIB_DECLSPEC void log( const char *filename, long linenr, const char *funcname, int level, std::string exp );

// Conditionally log a message
#define PL_LOG( level, msg ) \
	if ( level <= olib::openpluginlib::log_level( ) ) \
		olib::openpluginlib::log( __FILE__, __LINE__, PL_CURRENT_FUNC_NAME, level, msg )
	
// Use these to get named loglevel integers in PL_LOG.
// Make sure you have a namespace pl = olibs::openpluginlib for the following to work
// PL_LOG( pl::level::info,  "Some info message");
namespace level
{
	enum type
	{
		emergency = 0,  //!< emergency situation. 
		alert = 1,      //!< An extremely serious error has occurred
		critical = 2,   //!< A serious error has occured
		error = 3,      //!< An error has occurred
		warning = 4,    //!< warning, something bad could happen.
		notice = 5,     //!< The user should notice this
		info = 6,       //!< Information to the user
		debug1 = 7,     //!< Used for debugging purposes.
		debug2 = 8,     //!< Used for debugging purposes.
		debug3 = 9,     //!< Used for debugging purposes.
		debug4 = 10,    //!< Used for debugging purposes.
		debug5 = 11,    //!< Used for debugging purposes.
		debug6 = 12,    //!< Used for debugging purposes.
		debug7 = 13,    //!< Used for debugging purposes.
		debug8 = 14,    //!< Used for debugging purposes.
		debug9 = 15,    //!< Used for debugging purposes.
		unknown = 16    //!< unknown severity
	};
}
} }

#if _MSC_VER
#	pragma warning ( default:4251 )
#pragma warning ( pop )
#endif

#endif
