#ifndef _CORE_CORE_TYPEDEFS_H_
#define _CORE_CORE_TYPEDEFS_H_


/** @file core/typedefs.h
    Common typedefs used throughout the olib::core assembly.
    All forward declarations should preferrably reside
    here. */

#include <boost/rational.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/function.hpp>
#include <boost/cstdint.hpp>
#include <map>
#include "./minimal_string_defines.hpp"

namespace olib
{
   namespace opencorelib
    {
        class base_job;
        class function_job;
        class invoker;
        class logtarget;
        class configurable_logtarget;
        class plugin_metadata;
        class object;
        class plugin_class_description;
        class xerces_sax_handler;
        class serializer;
        class deserializer;
        class property_bag;
        class cache_utilizer;
        class cached_resource_status;
        class cache_description;
        class thread_sleeper;
        class library_info;

        typedef boost::shared_ptr< base_job > base_job_ptr;
        typedef boost::shared_ptr< const base_job > const_base_job_ptr;
        typedef boost::shared_ptr< function_job > function_job_ptr;
        typedef boost::shared_ptr< const function_job > const_function_job_ptr;
        typedef boost::shared_ptr< logtarget > logtarget_ptr;
        typedef boost::shared_ptr< configurable_logtarget > configurable_logtarget_ptr;
        typedef boost::shared_ptr< plugin_metadata > plugin_metadata_ptr;
        typedef boost::shared_ptr< object > object_ptr;
        typedef boost::shared_ptr< const object > const_object_ptr;
        typedef boost::shared_ptr< plugin_class_description > plugin_class_description_ptr;
        typedef boost::shared_ptr< xerces_sax_handler > xerces_sax_handler_ptr;
        typedef boost::shared_ptr< serializer > serializer_ptr;
        typedef boost::shared_ptr< deserializer > deserializer_ptr;
        typedef boost::shared_ptr< property_bag > property_bag_ptr;
        typedef boost::shared_ptr< cache_utilizer > cache_utilizer_ptr;
        typedef boost::shared_ptr< cached_resource_status > cache_resource_status_ptr;
        typedef boost::shared_ptr< cache_description > cache_description_ptr;
        typedef boost::shared_ptr< thread_sleeper > thread_sleeper_ptr;
        typedef boost::shared_ptr< library_info > library_info_ptr;

        typedef boost::weak_ptr< object > weak_object_ptr;
              
        typedef boost::shared_ptr<invoker> invoker_ptr;
        typedef boost::function< void () > invokable_function;

        /// Represents a media time in an exact way.
		typedef boost::rational< boost::int64_t > rational_time;

        typedef std::map< t_string, olib::t_path > schema_map;

    }
}

#endif // _CORE_CORE_TYPEDEFS_H_

