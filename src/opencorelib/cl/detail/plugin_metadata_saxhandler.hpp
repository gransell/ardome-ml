#ifndef _CORE_DETAIL_PLUGIN_METADATA_SAXHANDLER_H_
#define _CORE_DETAIL_PLUGIN_METADATA_SAXHANDLER_H_

#include "../typedefs.hpp"
#include "../xerces_sax_traverser.hpp"
#include "../xerces_sax_handler.hpp"

namespace olib
{
   namespace opencorelib
    {
        /// Parser for plugin_metadata creation.
        /** Uses the xerces validating sax-parser to parse
            xml-files with plugin_metadata. 
            @author Mats Lindel&ouml;f*/
        class plugin_metadata_sax_handler : public xerces_sax_handler
        {
        public:
      
            /// Create an empty instance of the handler.
            plugin_metadata_sax_handler();
            virtual ~plugin_metadata_sax_handler();

            /// Called to let the handler register relevant handler functions.
            /** These are used as call-backs during the parse by the traverser.
                @param traverser The parser that will call back to this class when 
                        the registered patterns are recognized. 
                @param state_prefix All registration should take this into account, that
                        is add it before all registered patterns */
            virtual void register_handlers( xerces_sax_traverser& traverser, 
                                            const xerces_string& state_prefix);
            
            /// Called when the parsing is done.
            /** Enables this class to do some clean-up. */
            virtual void parsing_done();

            /// Get the result from the parsing.
            virtual object_ptr get_result() const;

            /// Set the internal result pointer to null.
            virtual void reset_result();
           
        private:
            
            void setup_handlers();
            
            void handle_assembly_name(  const traverse_state& st );
            void handle_class(const traverse_state& st);

            plugin_metadata_ptr m_res;
        };
    }
}

#endif // _CORE_DETAIL_PLUGIN_METADATA_SAXHANDLER_H_
