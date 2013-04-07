#include "precompiled_headers.hpp"

#include <numeric>

#ifdef OLIB_COMPILED_WITH_VISUAL_STUDIO
    #pragma warning( push )
    #pragma warning( disable : 4244 )
#endif

#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/parsers/AbstractDOMParser.hpp>
#include <xercesc/parsers/SAXParser.hpp>
#include <xercesc/sax/HandlerBase.hpp>
#include <xercesc/util/OutOfMemoryException.hpp>
#include <xercesc/util/TransService.hpp>
#include <xercesc/framework/MemBufInputSource.hpp>

#ifdef OLIB_COMPILED_WITH_VISUAL_STUDIO
    #pragma warning( pop )
#endif

#include "./string_defines.hpp"
#include "./xml_sax_parser.hpp"
#include "./str_util.hpp"
#include "./string_conversions.hpp"

using namespace XERCES_CPP_NAMESPACE;

 typedef boost::shared_ptr< SAXParser > sax_parser_ptr;

namespace olib
{
   namespace opencorelib
    {
        class parser_implementation
        {
        public:
            parser_implementation( const schema_map& schemas )
            {
                m_parser_instance = create_parser(schemas);
            }

            SAXParser* get_parser() { return m_parser_instance.get(); };

        private:

            static t_string create_schema_location( t_string& str, const schema_map::value_type& vt )
            {
                t_string path_to_schema( vt.second.native() );
                boost::algorithm::replace_all( path_to_schema, _CT(" "), _CT("%20"));
                if(!str.empty()) str += _CT(" ");
                return str + vt.first + _CT(" ") + path_to_schema;
            }

            sax_parser_ptr create_parser( const schema_map& schemas )
            {
                sax_parser_ptr parser(new SAXParser());

                // Only validate xml for which we have loaded a grammar.
                parser->setValidationScheme(SAXParser::Val_Auto);

                // Take name spaces into consideration.
                parser->setDoNamespaces(true);

                // Use an xml-schema, if added.
                parser->setDoSchema(true);

                // Use strict checking, might be slow ... can be altered later.
                parser->setValidationSchemaFullChecking(true);

                // Use cached grammars (we need to have a parser singleton 
                // to make this efficient.
                parser->cacheGrammarFromParse( true );

                t_string schema_locations = 
                    std::accumulate( schemas.begin(), 
                    schemas.end(), t_string(_CT("")), &parser_implementation::create_schema_location );

                // Make sure that this is an utf 16 string with 2-byte chars
                utf16_string wide = str_util::to_wstring( schema_locations );
                #if defined( OLIB_ON_MAC ) || defined( OLIB_ON_LINUX )
                    std::vector<boost::uint16_t> packed = 
                        opencorelib::string_conversions::pack_wide_string
                        ( reinterpret_cast<const boost::uint32_t*>(wide.c_str()), schema_locations.size() );
                    parser->setExternalSchemaLocation( &packed[0] );
                #else
                    parser->setExternalSchemaLocation(wide.c_str());
                #endif

                return parser;
            }
            
            sax_parser_ptr m_parser_instance;

        };

        xml_sax_parser::xml_sax_parser(const schema_map& schemas) 
            : m_impl( new parser_implementation( schemas) )
        {

        }

        void* xml_sax_parser::get_parser()
        {
            return (void*)m_impl->get_parser();
        }
    }
}
