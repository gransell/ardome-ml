#include "precompiled_headers.hpp"
#include "logger.hpp"
#include "loghandler.hpp"
#include "exception_context.hpp"
#include "str_util.hpp"

namespace olib
{
   namespace opencorelib
    {
        #ifdef OLIB_COMPILED_WITH_VISUAL_STUDIO
        #pragma warning(push)
        #pragma warning(disable:4355) //'this' : used in base member initializer list
        #endif

        #undef ARLOG_A
        #undef ARLOG_B

        logger::logger(void):
            ARLOG_A(*this), 
            ARLOG_B(*this),
			m_ignore_this_log(false),
			m_context( new exception_context()),
            mlog_level( log_level::info),
            m_log_hint( log_target::not_set )
        {
        }

        logger::logger( const logger& o )
            :	ARLOG_A(*this), 
                ARLOG_B(*this),
			m_ignore_this_log(o.m_ignore_this_log),
			m_context(o.m_context),
            mlog_level(o.mlog_level),
            m_log_hint( o.m_log_hint )
        {
            o.m_ignore_this_log = true;
        }

        // This constructor is only used by the trace_logger class.
        logger::logger( boost::shared_ptr<exception_context> ctx, log_level::severity lvl )
            :   ARLOG_A(*this), 
            ARLOG_B(*this )
        {
            m_ignore_this_log = true;
            m_context = ctx;
            mlog_level = lvl;
            m_log_hint = log_target::trace;
        }

        #ifdef OLIB_COMPILED_WITH_VISUAL_STUDIO
        #pragma warning(pop)
        #endif
            
        logger::~logger(void)
        {
            handle_log();
        }

        void logger::handle_log()
        {
            try
            {
                t_stringstream ss;
                ss << m_context->source() << _T("::") << m_context->target_site();;
                the_log_handler::instance().log(*this, ss.str().c_str());
            }
            catch (...) 
            {
                // swallow any exceptions that occur 
            }
        }
        
        logger& logger::operator=(const logger& o)
        {
            m_context = o.m_context;
            mlog_level = o.mlog_level;
            m_ignore_this_log = o.m_ignore_this_log;
            m_log_hint = o.m_log_hint;
            o.m_ignore_this_log = true;
            return *this;
        }

        logger logger::make_logger()
        {
            return logger();
        }

        logger& logger::add_context(	const char* filename, long linenr, 
            const char* funcdname, const char* exp )
        {
            m_context->add_context(filename, linenr, funcdname, exp );
            return *this;	
        }

        logger& logger::msg( const t_string& str_msg)
        {
            if(!m_context) return *this;
            m_context->message(str_msg);
            return *this;
        }

        logger& logger::msg( const wchar_t* str_msg)
        {
            if(!m_context) return *this;
            m_context->message(str_util::to_t_string(str_msg));
            return *this;
        }

        logger& logger::msg( const char* str_msg)
        {
            if(!m_context) return *this;
            m_context->message(str_util::to_t_string(str_msg));
            return *this;
        }
     
        log_level::severity logger::level() const
        {
            return mlog_level;
        }

        logger& logger::level( const log_level::severity lvl )
        {
            mlog_level = lvl;
            return *this;
        }

        boost::shared_ptr<exception_context> logger::context()
        {
            return m_context;
        }

        void logger::pretty_print( t_ostream& ostrm, print::option print_option )
        {
            if( !m_context ) return;
            m_context->pretty_print(ostrm, print_option);
        }

        void logger::pretty_print_one_line( t_ostream& ostrm, print::option print_option )
        {
            if( !m_context ) return;
            m_context->pretty_print_one_line(ostrm, print_option);
        }

      

        trace_logger::trace_logger(    const char* msg, log_level::severity level, 
            const char* filename, long linenr, 
            const char* funcdname, const char* funcname  ) 
            : m_funcname( str_util::to_t_string(funcname) ), m_msg( str_util::to_t_string(msg) ),
                m_level(level)
        {
            m_context = boost::shared_ptr<exception_context>( new exception_context() );
            m_context->add_context( filename, linenr, funcdname, "trace_logger");
            
            olib::t_stringstream ss_msg;
            ss_msg << _T(" --> ") << m_funcname << _T(" ") << m_msg;
            m_context->message( ss_msg.str() );
            
            olib::t_stringstream ss;
            ss << m_context->source() << _T("::") << m_context->target_site();
            logger to_log(m_context, m_level );
            olib::opencorelib::the_log_handler::instance().log( to_log , ss.str().c_str());
            m_timer.start();
        }

        trace_logger::~trace_logger()
        {
            m_timer.stop();
            
            olib::t_stringstream ss_msg;
            ss_msg << _T(" <-- (") << m_timer.elapsed() << _T(") ") << m_funcname << _T(" ") << m_msg;
            m_context->message( ss_msg.str() );

            olib::t_stringstream ss;
            ss << m_context->source() << _T("::") << m_context->target_site();

            logger to_log(m_context, m_level);
            olib::opencorelib::the_log_handler::instance().log( to_log, ss.str().c_str());
        }

    }
}
