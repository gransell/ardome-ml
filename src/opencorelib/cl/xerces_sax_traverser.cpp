#include "precompiled_headers.hpp"
#include "./xerces_sax_traverser.hpp"
#include "./xerces_sax_handler.hpp"

#include <numeric>

#include "./enforce_defines.hpp"
#include "./enforce.hpp"
#include "./assert_defines.hpp"
#include "./assert.hpp"
#include "./string_defines.hpp"
#include "./str_util.hpp"
#include "./string_conversions.hpp"
#include "./log_defines.hpp"
#include "./logger.hpp"
#include "./loghandler.hpp"
#include "./exception_context.hpp"
#include "./machine_readable_errors.hpp"

using namespace olib::opencorelib::str_util;
using namespace XERCES_CPP_NAMESPACE;

namespace olib
{
   namespace opencorelib
    {
        unsigned int std_bin_input_stream::readBytes(   XMLByte* const toFill,
                                                        const unsigned int maxToRead )
        { 
            m_is.read( (char*)toFill, maxToRead);
            return m_is.gcount();
        }

        xerces_string L2( const char* str )
        {
            ARENFORCE_MSG( str, "Null pointer not allowed.");
            xerces_array val( XMLString::transcode(str));
            return xerces_string( val.get());
        }

        bool ends_with( const xerces_string& lhs, const xerces_string& rhs )
        {
            return XMLString::endsWith(lhs.c_str(), rhs.c_str());
        }

        bool equal( const xerces_string& lhs, const xerces_string& rhs )
        {
            return to_t_string(lhs.c_str()) == to_t_string(rhs.c_str());
        }

        boost::posix_time::ptime parse_ptime( const XMLCh* chars, unsigned int length )
        {
            // CCYY-MM-DDThh-mm-ss
            using namespace boost::gregorian;
            using namespace boost::posix_time;

            t_smatch xmatch;
            t_regex re(_T("([[:digit:]]{4})-([[:digit:]]{2})-([[:digit:]]{2})") _T("T")
                _T("([[:digit:]]{2}):([[:digit:]]{2}):([[:digit:]]{2})"));
            t_string str = to_t_string( (boost::uint16_t*)chars, length);
            ARENFORCE_MSG( boost::regex_search( str, xmatch, re ), 
                "The passed string is not on the form: CCYY-MM-DDThh:mm:ss" )( to_string(str));

            try {
                date d( boost::lexical_cast<int>( xmatch[1].str() ),
                    boost::lexical_cast<int>( xmatch[2].str() ),
                    boost::lexical_cast<int>( xmatch[3].str() ));

                time_duration dur(
                    boost::lexical_cast<int>( xmatch[4].str() ),
                    boost::lexical_cast<int>( xmatch[5].str() ),
                    boost::lexical_cast<int>( xmatch[6].str() ));

                return ptime(d, dur);
            }
            catch( const boost::bad_lexical_cast& e ) {
                ARENFORCE_MSG( false, "Failed to convert date using lexical cast" )(e.what());
            }
            return ptime(date(), time_duration());
        }
		
		bool parse_bool( const XMLCh* const chars )
		{
			t_string str = to_t_string(chars, XMLString::stringLen(chars));
			boost::algorithm::to_lower(str);
			return str == _T("true") || str == _T("1");
		}

        t_string parse_attribute(  const char* att_name, 
                                   XERCES_CPP_NAMESPACE::AttributeList* att )
        {
            ARENFORCE_ERR(att, error::null_pointer());
            const XMLCh* att_val = att->getValue( L2(att_name).c_str() );
            return str_util::to_t_string( att_val, XMLString::stringLen(att_val));
        }

		boost::any string_to_boost_any( const XMLCh *const chars, type_hint::type th )
		{
			if( th  == type_hint::th_int )
			{
				int v = boost::lexical_cast<int>((to_t_string(chars, XMLString::stringLen(chars))));
				return boost::any(v);
			}
			else if( th == type_hint::th_float )
			{
				float v = boost::lexical_cast<float>((to_t_string(chars, XMLString::stringLen(chars))));
				return boost::any(v);
			}
			else if( th == type_hint::th_double )
			{
				double v = boost::lexical_cast<double>((to_t_string(chars, XMLString::stringLen(chars))));
				return boost::any(v);
			}
			else if( th == type_hint::th_bool )
			{
				bool v = parse_bool(chars);
				return boost::any(v);
			}
			else if( th == type_hint::th_string ||
				th == type_hint::th_any )
			{
				t_string v = to_t_string(chars);
				return boost::any(v);
			}
			else
			{
				ARLOG_ERR("Invalid type_hint: %i (%s)")(th)(to_t_string(chars, XMLString::stringLen(chars)));
				return boost::any();
			}
		}

        xerces_sax_traverser::xerces_sax_traverser(
                const xerces_sax_handler_ptr& saxh  ) 
                : m_crate_inner_xml(false), m_parser(saxh)
        {
            ARENFORCE(m_parser);
            m_parser->register_handlers( *this, L2(""));
        }

        void xerces_sax_traverser::add_handler( const xerces_string& s, const parse_handler& ph )
        {
            parse_handler_map::iterator it = m_handlers.find(s);
            ARENFORCE_MSG( it == m_handlers.end(), "key already in use" )(str_util::to_t_string(s.c_str()));
            m_handlers[s] = ph;
        }

        void xerces_sax_traverser::start_create_inner_xml()
        {
            m_xml_data.str(_T(""));
            m_crate_inner_xml = true;
        }

        void xerces_sax_traverser::stop_create_inner_xml()
        {
            m_crate_inner_xml = false;
        }

        t_string xerces_sax_traverser::get_inner_xml() const
        {
            return m_xml_data.str();
        }

        object_ptr xerces_sax_traverser::load_and_parse(const XERCES_CPP_NAMESPACE::InputSource& bin_data,
                                                        const schema_map& schemas )
        {
            return load_and_parse( &bin_data, 0, schemas );
        }

        object_ptr xerces_sax_traverser::load_and_parse(   
                            const olib::t_path& xml_doc_path,
                            const schema_map& schemas )
        {
            return load_and_parse( 0, &xml_doc_path, schemas );
        }

        object_ptr xerces_sax_traverser::load_and_parse(  const XERCES_CPP_NAMESPACE::InputSource* bin_data ,
                                                        const olib::t_path* xml_doc_path,
                                                        const schema_map& schemas )
        {
            m_traverser = create_traverser( schemas );
            m_traverser->setDocumentHandler(this);
            m_traverser->setErrorHandler(this);

            try
            {
                boost::uint16_t const * xml_doc_path_ptr(0);
                #if defined( OLIB_ON_MAC ) || defined( OLIB_ON_LINUX )
                std::vector<boost::uint16_t> packed;
                if( xml_doc_path) 
                {
                    packed = string_conversions::pack_wide_string( 
                                reinterpret_cast<const boost::uint32_t*>(
                                                    to_wstring( xml_doc_path->string() ).c_str()), 
                                                    xml_doc_path->string().size() );
                    xml_doc_path_ptr = &packed[0];
                }
                #else
                    if(xml_doc_path) xml_doc_path_ptr = xml_doc_path->string().c_str();
                #endif

                if(xml_doc_path_ptr) m_traverser->parse(xml_doc_path_ptr);
                else if(bin_data) m_traverser->parse( *bin_data );
                else ARASSERT_MSG(false, "One of xml_doc_path and bin_data should be non-null!");
                m_parser->parsing_done();
                return m_parser->get_result();
            }
            catch( SAXException& e)
            {
                ARLOG_ERR("Failed to parse. Message: %s")(to_t_string( e.getMessage()));
                return object_ptr();
            }
            catch( XMLException& e)
            {
                ARLOG_ERR("Failed to parse. Message: %s")(to_t_string( e.getMessage()));
                return object_ptr();
            }

            return object_ptr();
        }

        
        void xerces_sax_traverser::error(const SAXParseException& exc)
        {
            ARENFORCE_ERR(false, error::invalid_xml())( to_t_string(exc.getMessage()) )(exc.getLineNumber())(exc.getColumnNumber())
                (to_t_string(exc.getPublicId()))(to_t_string(exc.getSystemId()));
            throw exc;
        }

        void xerces_sax_traverser::fatalError(const SAXParseException& exc)
        {
            ARENFORCE_ERR(false, error::invalid_xml())( to_t_string(exc.getMessage()) )(exc.getLineNumber())(exc.getColumnNumber());
            throw exc;
        }

        void xerces_sax_traverser::startElement(const XMLCh* const name, AttributeList&  attributes )
        {
            xerces_string slash(&chForwardSlash, 1);
            int i = XMLString::indexOf(name, chColon);
            xerces_string name_wo_ns(name + i + 1);
            m_state += slash + name_wo_ns;

            if( m_crate_inner_xml )
            {
                stream_xml_start_tag(name, &attributes);
            }
            else
            {
                parse_handler_map::const_iterator fit = m_handlers.find( m_state );
                if(fit != m_handlers.end())
                {
                    traverse_state st( &attributes, 0, 0, this, xml_location::element_start );
                    fit->second(st);
                }
            }
        }

        void xerces_sax_traverser::characters(const XMLCh* const chars, const unsigned int length)
        {
            if(m_crate_inner_xml ) stream_xml_content(chars, length);
            else
            {
                parse_handler_map::const_iterator fit = m_handlers.find( m_state );
                if(fit != m_handlers.end())
                {
                    traverse_state st( 0, chars, length, this, xml_location::element_content );
                    fit->second(st);
                }
            } 
        }

        void xerces_sax_traverser::endElement(const XMLCh* const name)
        {
            if( m_crate_inner_xml )
            {
                parse_handler_map::const_iterator fit = m_handlers.find( m_state );
                if(fit != m_handlers.end())
                {
                    traverse_state st( 0, 0, 0, this, xml_location::element_end );
                    fit->second(st);
                }
                else
                {
                    stream_xml_end_tag(name);
                }
            }
            else
            {
                parse_handler_map::const_iterator fit = m_handlers.find( m_state );
                if(fit != m_handlers.end())
                {
                    traverse_state st( 0, 0, 0, this, xml_location::element_end );
                    fit->second(st);
                }
            }

            size_t n = m_state.find_last_of(chForwardSlash);
            m_state.erase(n);                     
        }

        
        void xerces_sax_traverser::stream_xml_start_tag( const XMLCh* const elem_name,
                                                            AttributeList* attributes )
        {
            m_xml_data << _T("<") <<  to_t_string(elem_name);
            boost::uint32_t att_len = attributes->getLength();
            for( boost::uint32_t i = 0; i < att_len ; ++i)
            {
                m_xml_data << _T(" ")
                           << to_t_string(attributes->getName(i)) << _T("=\"")
                           << to_t_string(attributes->getValue(i)) << _T("\"");
            }
            
            m_xml_data << _T(">");
        }

        void xerces_sax_traverser::stream_xml_content(   const XMLCh* const chars,
                                                            const unsigned int length)
        {
            m_xml_data << to_t_string(chars) ;
        }

        void xerces_sax_traverser::stream_xml_end_tag( const XMLCh* const elem_name )
        {
            m_xml_data << _T("</") << to_t_string(elem_name) << _T(">");
        }

        namespace
        {
            t_string create_schema_location( t_string& str, const schema_map::value_type& vt )
            {
                t_string path_to_schema( str_util::to_t_string(vt.second.string()));
                boost::algorithm::replace_all( path_to_schema, _T(" "), _T("%20"));
                if(!str.empty()) str += _T(" ");
                return str + vt.first + _T(" ") + path_to_schema;
            }
        }
       
        sax_parser_ptr xerces_sax_traverser::create_traverser( const schema_map& schemas )
        {
            using namespace opencorelib::string_conversions;
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

            if( schemas.empty()) return parser;

            t_string schema_locations =  std::accumulate( schemas.begin(), 
                                schemas.end(), t_string(_T("")), &create_schema_location );

            ARLOG_DEBUG4("Schema locations is %1%")(schema_locations);
            
            // Make sure that this is an utf 16 string with 2-byte chars
            utf16_string wide = str_util::to_wstring( schema_locations );
            #if defined( OLIB_ON_MAC ) || defined( OLIB_ON_LINUX )
            std::vector<boost::uint16_t> packed = 
                pack_wide_string( reinterpret_cast<const boost::uint32_t*>(wide.c_str()),
                                  schema_locations.size() );
            parser->setExternalSchemaLocation( &packed[0] );
            #else
            parser->setExternalSchemaLocation(wide.c_str());
            #endif

            return parser;
        }
    }
}
