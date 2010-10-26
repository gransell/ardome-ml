#ifndef _CORE_MACHINE_READABLE_ERRORS_H_
#define _CORE_MACHINE_READABLE_ERRORS_H_

#include "./minimal_string_defines.hpp"

namespace olib 
{
    class CORE_API error 
    {
    public:
        static const TCHAR * null_pointer() { return _CT("olib.error.null_pointer"); }
        static const TCHAR * plugin_creation_failure() { return _CT("olib.error.plugin_creation_failure"); }
        static const TCHAR * xerces_not_initialized() { return _CT("olib.error.xerces_not_initialized"); }
        static const TCHAR * logical_error() { return _CT("olib.error.logical_error"); }
        static const TCHAR * dlopen_failed() { return _CT("olib.error.dlopen_failed"); }
        static const TCHAR * dlsym_failed() { return _CT("olib.error.dlsym_failed"); }
        static const TCHAR * filter_not_available() { return _CT("olib.error.filter_not_available"); }
        static const TCHAR * filter_not_defined() { return _CT("olib.error.filter_not_defined"); }
        static const TCHAR * fetch_from_placeholder_input() { return _CT("olib.error_fetch_from_placeholder_input"); }

        /// Issued when a transition can't be found in the_effect_handler.
        static const TCHAR * transition_not_defined() { return _CT("olib.error.transition_not_defined"); }
        /// Something is wrong in the settings of amf
        static const TCHAR * settings_error() { return _CT("olib.error.settings_error"); }
        /// The given transition is not correct (in one way or the other)
        static const TCHAR * invalid_transition() { return _CT("olib.error.invalid_transition"); } 

        /// A search for a suitable target track failed.
        static const TCHAR * target_track_not_found() { return _CT("olib.error.target_track_not_found"); }

        static const TCHAR * value_out_of_range() { return _CT("olib.error.value_out_of_range"); }
        static const TCHAR * parse_error() { return _CT("olib.error.parse_error"); }
        static const TCHAR * invalid_parameter_value() { return _CT("olib.error.invalid_parameter_value"); }
        static const TCHAR * file_not_found() { return _CT("olib.error.file_not_found"); }

        /// Issued by the xerces sax traverser when it find badly formatted xml
        static const TCHAR * invalid_xml() { return _CT("olib.error.invalid_xml"); }
    };
}

#endif //_CORE_MACHINE_READABLE_ERRORS_H_
