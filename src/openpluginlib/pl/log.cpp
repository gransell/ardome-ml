#include <iostream>
#include "log.hpp"
#include <opencorelib/cl/core.hpp>
#include <opencorelib/cl/logger.hpp>
#include <opencorelib/cl/loghandler.hpp>

using namespace std;
using namespace olib::openpluginlib;
namespace cl = olib::opencorelib;

namespace olib { namespace openpluginlib {

// Default logger
static void default_log( const char *filename, long linenr, const char *funcname, int level, std::string exp )
{
	std::cerr << filename << ":" << linenr << ":" << funcname << " " << exp << std::endl;
}

// Static variables to hold logging configuration
static int log_level_ = level::warning;
static logger callback_ = default_log;

// Set the log callback
void set_log_callback( logger callback )
{
	callback_ = callback;
}

// Set the log level
void set_log_level( int level )
{
	cl::the_log_handler::instance( ).set_global_log_level( static_cast< cl::log_level::severity >( level ) );
	log_level_ = level;
}

// Return the log level
int log_level( )
{
	return log_level_;
}

// Log methods
void log( const char *filename, long linenr, const char *funcname, int level, std::string exp )
{
	callback_( filename, linenr, funcname, level, exp );
}

void log( const char *filename, long linenr, const char *funcname, int level, boost::format exp )
{
	callback_( filename, linenr, funcname, level, exp.str( ) );
}

} }

