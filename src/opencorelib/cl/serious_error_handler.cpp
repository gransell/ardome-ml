#include "precompiled_headers.hpp"
#include "serious_error_handler.hpp"

#define LOKI_CLASS_LEVEL_THREADING
#include <loki/Singleton.h>

namespace olib
{
   namespace opencorelib
    {
        typedef Loki::SingletonHolder<	serious_error_handler > the_internal_serious_error_handler;

        serious_error_handler& the_serious_error_handler::instance()
        {
            return the_internal_serious_error_handler::Instance();
        }

        serious_error_handler::serious_error_handler()
        {
     
        }

        void serious_error_handler::set_dump_file_name( const t_string& dump_file_name)
        {
            m_dump_file_name = dump_file_name;
        }

        t_string serious_error_handler::get_dump_file_name() const
        {
            return m_dump_file_name;
        }

    }
}
