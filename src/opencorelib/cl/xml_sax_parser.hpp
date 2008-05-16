#ifndef _CORE_XML_SAX_PARSER_H_
#define _CORE_XML_SAX_PARSER_H_

#include "./typedefs.hpp"

namespace olib
{
   namespace opencorelib
    {
        typedef std::basic_string<XMLCh> xerces_string;
        typedef boost::scoped_array<XMLCh> xerces_array;

        class parser_implementation;
        class CORE_API xml_sax_parser
        {
        public:
            xml_sax_parser(const schema_map& schemas );
            
            /// Actually returns a XERCES_CPP_NAMESPACE::SAXParser pointer
            /** Used internally in amf to parse xml. */
            void* get_parser();
            
        private:
            boost::shared_ptr<parser_implementation> m_impl;
        };
    }
}

#endif // _CORE_XML_SAX_PARSER_H_

