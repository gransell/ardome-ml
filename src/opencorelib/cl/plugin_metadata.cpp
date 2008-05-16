#include "precompiled_headers.hpp"
#include "./str_util.hpp"
#include "./plugin_metadata.hpp"
#include "./detail/plugin_metadata_saxhandler.hpp"
#include "./xml_sax_parser.hpp"

#include "./log_defines.hpp"
#include "./logger.hpp"
#include "./loghandler.hpp"
#include "./exception_context.hpp"

using namespace boost;

namespace olib
{
   namespace opencorelib
    {
        plugin_metadata::plugin_metadata()
        {
        }

        CORE_API 
        plugin_metadata_ptr 
            plugin_metadata_from_xml(   const olib::t_path& xml_path,
                                        const schema_map& schemas )
        {
            xerces_sax_handler_ptr a_sax_handler(new plugin_metadata_sax_handler() );
            xerces_sax_traverser traverser(a_sax_handler); 
            return dynamic_pointer_cast< plugin_metadata >
                            (traverser.load_and_parse( xml_path, schemas ));
        }
    }
}
