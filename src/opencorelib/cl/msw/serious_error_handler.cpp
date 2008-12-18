#include "precompiled_headers.h"
#include <core/serious_error_handler.h>

// Microsoft Windows version
#ifdef __AUMSW__

#pragma warning (disable:4535)
#include <core/msw/debughlp.h>

namespace amf
{
    namespace core
    {
        class structured_exception : public std::exception
        {
        public:
            structured_exception(unsigned int cde) : m_se_code(cde) {}
            virtual const char* what( ) const throw( );
            unsigned int code() const;
            std::string code_asString() const;

        private:
            mutable boost::shared_array<char> m_Buffer;
            unsigned int m_se_code;
        };

        unsigned int structured_exception::code() const
        {
            return m_SECode;
        }

        const char* structured_exception::what( ) const throw( )
        {
            boost::format fmt("amf::useful::structured_exception::%s");
            fmt % code_asString();
            std::string str = fmt.str();
            m_Buffer = boost::shared_array<char>( new char[str.size()+1] );
            strncpy(m_Buffer.get(), str.c_str(), str.size()+1);
            return m_Buffer.get();
        }

        std::string structured_exception::code_asString() const
        {
            if( m_SECode == EXCEPTION_ACCESS_VIOLATION ) return "memory access violation";
            if( m_SECode == EXCEPTION_ILLEGAL_INSTRUCTION ) return "illegal instruction";
            if( m_SECode == EXCEPTION_PRIV_INSTRUCTION ) return "privileged instruction";
            if( m_SECode == EXCEPTION_IN_PAGE_ERROR ) return "memory page error";
            if( m_SECode == EXCEPTION_STACK_OVERFLOW ) return "stack overflow";
            if( m_SECode == EXCEPTION_DATATYPE_MISALIGNMENT ) return "data misalignment";
            if( m_SECode == EXCEPTION_INT_DIVIDE_BY_ZERO ) return "integer divide by zero";
            if( m_SECode == EXCEPTION_INT_OVERFLOW ) return "integer overflow";
            if( m_SECode == EXCEPTION_ARRAY_BOUNDS_EXCEEDED ) return "array bounds exceeded";
            if( m_SECode == EXCEPTION_FLT_DIVIDE_BY_ZERO ) return "floating point divide by zero";
            if( m_SECode == EXCEPTION_FLT_STACK_CHECK ) return "floating point stack check";
            if( m_SECode == EXCEPTION_INT_OVERFLOW ) return "integer overflow";
            if( m_SECode == EXCEPTION_FLT_DENORMAL_OPERAND ) return "floating point error, denormal operand";
            if( m_SECode == EXCEPTION_FLT_INEXACT_RESULT ) return "floating point error, inexact result";
            if( m_SECode == EXCEPTION_FLT_INVALID_OPERATION ) return "floating point error, invalid operation";
            if( m_SECode == EXCEPTION_FLT_OVERFLOW ) return "floating point error, overflow";
            if( m_SECode == EXCEPTION_FLT_UNDERFLOW ) return "floating point error, underflow";
            else return "unrecognized exception or signal";
        }

        namespace
        {
            boost::recursive_mutex g_Mtx;
            LPTOP_LEVEL_EXCEPTION_FILTER g_Old_filter = 0;

            long run_old_filter(_EXCEPTION_POINTERS* ep)
            {
                if(g_Old_filter) 
                {
                    std::cout << "Running old filter" << std::endl;
                    return g_old_filter(ep);
                }
                else return EXCEPTION_CONTINUE_SEARCH;
            }

            long WINAPI unhandled_exception_function( _EXCEPTION_POINTERS* ep )
            {
                boost::recursive_mutex::scoped_lock lck(g_Mtx);
                std::cout << "Unhandled_exception_function" << std::endl;
                // Dump the exception to file.
                if ( !wx_dbg_helpDLL::init() ) return EXCEPTION_CONTINUE_SEARCH;

                t_string file_name = the_serious_error_handler::instance().get_dump_file_name();

                HANDLE h_file = ::create_file(file_name.c_str(), 
                                            GENERIC_WRITE,
                                            0,    // no sharing
                                            NULL, // default security
                                            CREATE_ALWAYS,
                                            FILE_FLAG_WRITE_THROUGH, // no buffering
                                            NULL  // no template file 
                                            );
                
                if(!h_file) return run_old_filter(ep);
                
                MINIDUMP_EXCEPTION_INFORMATION minidump_exc_info;

                minidump_exc_info.thread_id = ::get_current_thread_id();
                minidump_exc_info.exception_pointers = ep;
                minidump_exc_info.client_pointers = FALSE; // in our own address space

               MINIDUMP_TYPE dump_flags = 
                   static_cast<MINIDUMP_TYPE>(      
                        Mini_dump_scan_memory | Mini_dump_with_indirectly_referenced_memory);

               wx_dbg_helpDLL::mini_dump_write_dump( ::get_current_process(),
                                                ::get_current_process_id(),
                                                h_file,     // file to write to
                                                dump_flags, // kind of dump to create
                                                &minidump_exc_info,
                                                NULL,      // no extra user-defined data
                                                NULL /* no callbacks */ );

                return run_old_filter(ep);    
            }

            void __cdecl structured_exception_translator( unsigned int code, 
                                                        EXCEPTION_POINTERS* ep)
            {
                aUUNUSED_ALWAYS((ep, code));
                throw; //cStructured_exception(code);
            }

            void __cdecl structured_exception_forward( unsigned int code, 
                                                     EXCEPTION_POINTERS* ep)
            {
                aUUNUSED_ALWAYS((ep, code));
                throw;
            }
        }

        void serious_error_handler::setup()
        {
            boost::recursive_mutex::scoped_lock lck(g_Mtx);

            if( !::is_debugger_present() )
                _set_se_translator( &Structured_exception_forward );
            else
                _set_se_translator( &Structured_exception_translator );
            
            if( g_Old_filter !=  &Unhandled_exception_function )
            {
                g_Old_filter = ::set_unhandled_exception_filter( &Unhandled_exception_function );
            }
        }
    }
}

#endif // __AUMSW__