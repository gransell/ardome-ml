#ifndef _CORE_FUNCTION_JOB_H_
#define _CORE_FUNCTION_JOB_H_

#include "jobbase.hpp"

namespace olib
{
   namespace opencorelib
    {
        /// Simple job implementation. 
        /** Use boost::bind to enable extremely simple job creation.
            The code below will create a job that will run the long_task
            function in its do_work implementation.
            <pre>	
            shared_ptr<job_base> bj( 
                    new function_job( bind( long_task, this, (int)i, 100 )));
            </pre>
            @author Mats Lindel&ouml;f*/
        class CORE_API function_job : public base_job
        {
        public:
            typedef boost::function<void ( boost::shared_ptr<base_job> )> job_function_type;
            /// Create a new job.
            /** Priority will be set to 5.
                @param to_do The job to run i do_work. */
            function_job(  job_function_type to_do );

            /// Create a new job with a given priority.
            /** @param prio The priority of the new job.
                @param to_do The function to run. */
            function_job( boost::int32_t prio, job_function_type to_do );

            /// Overrides job_base::do_work, just calls the bound function.
            virtual int do_work();
        private:
            function_job( const function_job& );
            function_job& operator=( const function_job& );

            job_function_type m_to_do;
        };
    }
}

#endif // _CORE_FUNCTION_JOB_H_
