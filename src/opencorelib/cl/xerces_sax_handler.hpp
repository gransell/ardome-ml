#ifndef _CORE_XERCES_SAX_HANDLER_H_
#define _CORE_XERCES_SAX_HANDLER_H_

#include "./typedefs.hpp"
#include "./xml_sax_parser.hpp"

namespace olib
{
   namespace opencorelib
    {
        class xerces_sax_traverser;

        /// Interface implemented by classes that want to be part of a xml-parse session.
        /** sax parsing is based on registered callbacks. To enable 
            nested parsing, that is having one class that parses content inside
            another class' parse-session this abstraction is needed. If class A
            has xml-content inside its xml-document that can be parsed by class B, 
            class A must be able to create a class B parser, let it register its 
            call-backs and then fetch the parse result from the class B parser when the parsing is done.
            If this wasn't possible we would have to parse the xml-document twice, once
            to get the things class A could understand and once to get the things class
            B could understand. Then we would have to patch the results together, meaning 
            internal knowledge about what class A actually creates.
            @see xerces_sax_traverser
            @author Mats Lindel&ouml;f */
        class CORE_API xerces_sax_handler 
        {
        public:

            /// Ensure correct destruction of derived classes.
            virtual ~xerces_sax_handler();

            /// Called to let the handler register relevant handler functions.
            /** These are used as call-backs during the parse by the traverser.
                @param traverser The parser that will call back to this class when 
                            the registered patterns are recognized. 
                @param state_prefix All registration should take this into account, that
                            is add it before all registered xml-paths. */
            virtual void register_handlers( xerces_sax_traverser& traverser, 
                                            const xerces_string& state_prefix) = 0;

            /// Called by the traverser when the full traversion of the whole xml-document is done.
            virtual void parsing_done();

            /// Get the result after a parse session.
            virtual object_ptr get_result() const = 0;

            /// Reset the internal result object, create a new empty one.
            virtual void reset_result() = 0;
        };

    }
}


#endif // _CORE_XERCES_SAX_HANDLER_H_

