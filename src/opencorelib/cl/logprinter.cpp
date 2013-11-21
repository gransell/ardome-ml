#include "precompiled_headers.hpp"

#include <iostream>
#include <locale>

#define LOKI_CLASS_LEVEL_THREADING
#include <loki/Singleton.h>

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/fstream.hpp>

#include "utilities.hpp"
#include "logprinter.hpp"
#include "str_util.hpp"
#include "enforce_defines.hpp"
#include "log_defines.hpp"
#include "logger.hpp"
#include "enforce.hpp"
#include "loghandler.hpp"
#include "special_folders.hpp"

namespace fs = boost::filesystem;
using namespace olib::opencorelib::str_util;

namespace olib
{
   namespace opencorelib
    {
        void log_utilities::get_formatted_stream( t_stringstream &ss, 
                                    const t_string& date_fmt,
                                    const t_string& time_fmt )
        {
            if( !date_fmt.empty() )
            {
                t_date_facet* date_output_facet = new t_date_facet();
                date_output_facet->format(date_fmt.c_str());
                ss.imbue( std::locale( ss.getloc(), date_output_facet) );
            }

            if( !time_fmt.empty() )
            {
                t_local_time_facet* time_output_facet = new t_local_time_facet();
                time_output_facet->format(time_fmt.c_str());
                ss.imbue( std::locale( ss.getloc(), time_output_facet) );
            }
        }

        t_string log_utilities::get_log_prefix_string( log_level::severity lvl, 
                                                       t_stringstream &ss,
                                                       logoutput::options log_options )
        {
            using namespace boost::date_time;
            using namespace boost::posix_time;

            //Clear out the stream before using it
            ss.str(_CT(""));
            ss.clear();

            ptime now = boost::posix_time::microsec_clock::local_time();
            std::locale loc = ss.getloc();
            if( std::has_facet<       t_date_facet >( loc ) ) ss << now.date();
            if( std::has_facet< t_local_time_facet >( loc ) ) ss << now.time_of_day();
            
            if( log_options & logoutput::current_thread_id  )
            {
                boost::uint64_t thread_id = utilities::get_current_thread_id();
                t_format thread_fmt(_CT(" (%08x)"));
                ss << (thread_fmt % thread_id).str();
            }

            if( log_options & logoutput::current_log_level )
            {
                ss << _CT(" [") << log_level::to_string(lvl) << _CT("]");
            }
            
            return ss.str();
        }

        void log_utilities::delete_old_log_files(const olib::t_path& full_path, const t_string& prefix, int older_than_days)
        {
            using namespace boost::posix_time;

            if ( older_than_days <= 0 )
                return;
            if( !fs::is_directory( full_path ) )
                return;

            // Match all files with the pattern:  
            // Prefix.YYYY-MM-DD-DAY.log
            olib::t_format wildcard_fmt(_CT("%s\\..*\\.log"));
            t_string escaped_prefix = olib::opencorelib::utilities::regex_escape(prefix);
            olib::t_regex wildcard( (wildcard_fmt % escaped_prefix ).str());

            ptime time_t_epoch(boost::gregorian::date(1970,1,1)); 
            ptime old_time_stamp = second_clock::universal_time() - boost::gregorian::days(older_than_days);
            time_duration time_since_epoch = old_time_stamp - time_t_epoch;

            olib::t_directory_iterator dir_itr( full_path ), end_iter;
            for ( ; dir_itr != end_iter; ++dir_itr )
            {
                try
                {
                    if ( !fs::is_directory( *dir_itr ) )
                    {
                        olib::t_smatch what; 
                        olib::t_string file_name = dir_itr->path().filename().native();
                        if( boost::regex_match(file_name, what , wildcard ) )
                        {
                            std::time_t last_write = fs::last_write_time(*dir_itr);
                            if( last_write < time_since_epoch.total_seconds() )
                            {
                                fs::remove(*dir_itr);
                            }
                        }
                    }
                }
                catch ( const std::exception & ex )
                {
                    ARLOG(ex.what());       
                }
            }
        }

        t_string log_utilities::get_suitable_logfile_name(const t_string& app_name )
        {
            using namespace boost::posix_time;
            ptime now = second_clock::local_time();

            t_stringstream ss;
            t_date_facet* output_facet = new t_date_facet();
            
            // %Y = year, four digits
            // %m = number of the month
            // %d = number of the day
            // %a = short name of the day
            output_facet->format(_CT("%Y-%m-%d-%a"));
            ss.imbue(std::locale(ss.getloc(), output_facet));
            ss << now.date();

            t_format file_name(_CT("%s.%s.log"));
            file_name % app_name % ss.str(); 
            return file_name.str();
        }

        void log_utilities::get_default_log_stream(const olib::t_path& log_file_path, std::ofstream &result)
        {
            utilities::make_sure_path_exists( log_file_path.parent_path() );
            
            result.open( log_file_path.c_str(), std::ios_base::app | std::ios_base::binary );
        }
    }
}
