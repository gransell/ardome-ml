#include "precompiled_headers.hpp"
#include "function_job.hpp"

namespace olib
{
   namespace opencorelib
    {
        function_job::function_job( function_job::job_function_type to_do )
            : m_to_do(to_do)
        {
        }

        function_job::function_job( boost::int32_t prio, function_job::job_function_type to_do )
            : base_job(prio), m_to_do(to_do)
        {
        }

        int function_job::do_work()
        {
            m_to_do(boost::dynamic_pointer_cast<function_job>(shared_from_this()));
            return 0;
        }
    }
}
