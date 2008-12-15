#ifndef _CORE_XERCES_SAX_TRAVERSER_H_
#define _CORE_XERCES_SAX_TRAVERSER_H_

// Warning: This file should not be included if you're not using xerces
// xml-parser in you application/assembly. If you get strange
// errors from the compiler not being able to find the xerces include
// files, make sure that this file is NOT included at all. amf does
// not enforce inclusion of this file anywhere (should not at least).

#include "./typedefs.hpp"
#include "./core_enums.hpp"

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
#include <xercesc/util/BinInputStream.hpp>
#include <xercesc/framework/MemBufInputSource.hpp>

#include <boost/scoped_array.hpp>
#include <boost/date_time/posix_time/ptime.hpp>
#include <boost/any.hpp>

#ifdef OLIB_COMPILED_WITH_VISUAL_STUDIO
    #pragma warning( pop )
#endif

/** @file xerces_sax_traverser.h
    amf wrapper class around the xerces sax-parser. */

namespace olib
{
   namespace opencorelib
    {

		typedef std::basic_string<XMLCh> xerces_string;

		/// Convert a char array to xml string
		/** Uses str_util::to_wstring */
		xerces_string to_x_string(const char *source, unsigned int length);

		/// Convert a std::string to xml string
		/** Uses str_util::to_wstring */
		xerces_string to_x_string(const std::string &source);

		/// Convert a wchar_t array to xml string
		/** Mac and Linux implementation repacks from 32 bit to 16 bit */
		xerces_string to_x_string(const wchar_t *source, unsigned int length);

		/// Convert a std::wstring to xml string
		/** Mac and Linux implementation repacks from 32 bit to 16 bit */
		xerces_string to_x_string(const std::wstring &source);

		/// Convert a Xerces string to std::wstring
		std::wstring x_to_wstring(const XMLCh *source);

		/// Convert a Xerces string to olib::t_string
		t_string x_to_t_string(const XMLCh *source);

		/// Convert a Xerces string to std::string
		/** Uses str_util::to_string */ 
		std::string x_to_string(const XMLCh *source);

        /// An array of unsigned shorts, or XMLCh as it is called by xerces. 
        typedef boost::scoped_array<XMLCh> xerces_array;

        /// A SAXParser, wrapped by a shared_ptr.
        typedef boost::shared_ptr< XERCES_CPP_NAMESPACE::SAXParser > sax_parser_ptr;

        // Forward declaration.
        class xerces_sax_traverser;

        /// Class that wraps a std::istream and exposes it to a SAXParser.
        /** @author Mats Lindel&ouml;f */
        class CORE_API std_bin_input_stream : public XERCES_CPP_NAMESPACE::BinInputStream
        {
        public:

            /// Create a new instance, wrapping an existing std::istream.
            std_bin_input_stream( std::istream& is ) : m_is(is) 
            {
                m_is >> std::noskipws;
            }

            /// Which byte-pos are we on?
            /** Required to implement by BinInputStream. */
            virtual unsigned int curPos() const
            {
                return m_is.tellg();
            }

            /// Read a number of bytes from the stream
            /** Required to implement by BinInputStream. */
            virtual unsigned int readBytes( XMLByte* const toFill,
                                            const unsigned int maxToRead );
            
        private:
            std::istream& m_is;
        };

        /// Wraps a std_bin_input_stream.
        /** @author Mats Lindel&ouml;f */
        class CORE_API std_input_source : public XERCES_CPP_NAMESPACE::InputSource
        {
        public:
            /// Create a new std_input_source.
            /** @param is The stream that is wrapped. */
            std_input_source( std::istream& is ) : m_is(is) {}

            /// Creates a wrapped BinInputStream object.
            /** Required by InputSource */
            virtual XERCES_CPP_NAMESPACE::BinInputStream* makeStream() const 
            {
                return new std_bin_input_stream(m_is);
            }

        private:
            std::istream& m_is;
        };

        /// Describes where the sax parser is in relation to the current element
        namespace xml_location
        {
            enum type
            {
                element_start, /**< At the start of the element. Attributes can be present */
                element_content, /**< At the content of the element. Can have characters. */
                element_end /**< At the end of the element, no data available */
            };
        }

        /// Lightweight struct that holds the current state of the SAXParser.
        struct CORE_API traverse_state
        {
            /// Create a new state.
            /** @param al The current list of attributes, or null if not at a start-tag.
                @param ch The characters if pointing to the start of content between a  
                            start and end tag. Null if at start of end tag. 
                @param len The lenght of the ch array. 0 if ch is 0.
                @param st A pointer to the traverser itself. Always valid.
                @param loc The current location of the read-head. Always valid*/
            traverse_state( XERCES_CPP_NAMESPACE::AttributeList* al, 
                            const XMLCh* const ch,
                            const unsigned int len,
                            xerces_sax_traverser* st,
                            xml_location::type loc) 
                            :   m_attributes(al), 
                                m_chars(ch), 
                                m_length(len), 
                                m_traverser(st),
                                m_location(loc) {}

            /// The attribute list, can be 0 if not at an xml start tag with attributes.
            XERCES_CPP_NAMESPACE::AttributeList* m_attributes;

            /// Characters of content between a start and end tag.  \<tag\>content\</tag\>
            const XMLCh* const m_chars;

            /// The number of characters pointed to by m_chars. 0 is m_chars i 0.
            const unsigned int m_length;

            /// Pointer to the current traverser, should never be 0.
            xerces_sax_traverser*  m_traverser;

            xml_location::type m_location;
        };

        /// A callback function called when a state is matched by the parser.
        typedef boost::function< void ( const traverse_state& st ) > parse_handler;

        /// Converts a ansi-string to an encoded utf-16 string.
        CORE_API xerces_string L2( const char* str );

        /// Returns true if the first parameter ends with the second.
        /** Example: lhs="/apf/locations/location" rhs="location", returns true
                    since the first string ends with the second. 
            @param lhs The string to test against.
            @param rhs The string that the test string should end with.
            @return true if the first string ends with the second.*/
        CORE_API bool ends_with( const xerces_string& lhs, const xerces_string& rhs );

        /// Tests if two strings are equal.
        CORE_API bool equal( const xerces_string& lhs, const xerces_string& rhs );

        /// Parses a CCYY-MM-DDThh:mm:ss string.
        /** @param chars A pointer to a string with this format: CCYY-MM-DDThh:mm:ss
            @param length The number of characters that chars points to.
            @return A ptime representation of chars. */
        CORE_API boost::posix_time::ptime parse_ptime( const XMLCh* chars, unsigned int length );

		/// Parses a bool string. "true" or "1" returns true, otherwise false is returned.
		CORE_API bool parse_bool( const XMLCh* const chars );

		/// Parses a string value and a type-hint to a boost::any value.
		CORE_API boost::any string_to_boost_any( const XMLCh *const chars, type_hint::type th );

        /// Parses the attribute att_name and returns its value as a t_string.
        CORE_API t_string parse_attribute(  const char* att_name, 
                                            XERCES_CPP_NAMESPACE::AttributeList* att );
          

        /// Traverses an xml document and maintains a state string to denote the current location.
        /** It also holds a map of callbacks that are triggered
            when the state matches pre-registered keys. 
            @author Mats Lindel&ouml;f */
        class CORE_API xerces_sax_traverser 
                : public XERCES_CPP_NAMESPACE::HandlerBase
        {
        public:

            /// Create a new traverser. A SAXParser is needed for it to work.
            xerces_sax_traverser( const xerces_sax_handler_ptr& saxh );

            /// Loads and parses the given document.
            /** @param xml_doc_path The xml document to load.
                @param schemas A namespace->schema map used to validate the
                        parsed document.
                @return A deserialized object. */
            object_ptr load_and_parse(    const olib::t_path& xml_doc_path,
                                                const schema_map& schemas );

            /// Loads data from bin_data and parses it.
            /** @param bin_data An abstraction of a binary stream.
                @param schemas A namespace->schema map used to validate the parsed document. 
                @return A deserialized object. 
                @see std_bin_input_stream */
            object_ptr load_and_parse(  const XERCES_CPP_NAMESPACE::InputSource& bin_data ,
                                        const schema_map& schemas );

            /// Adds a callback function that will be called when the internal state matches s.
            /** The handler will be called both when the state begins to match and before
                it ends to match. For example, if the key is /some/path/to/ and the xml 
                document looks like this:
                <pre>
                &lt;some&gt;  
                    &lt;path&gt;
                        &lt;to&gt; &lt;-- First ph is called here. m_chars are null
                            jadajadajada &lt;-- ph is called here, attributes are null.
                        &lt;/to&gt; &lt;-- Finally ph is called here. attributes and m_chars are null.
                    &lt;/path&gt;
                &lt;/some&gt;
                </pre>
                @param s The key to match the parser's state against. When the match is 
                        true, the parse_hander is issued.
                @param ph The parse_hander to call when the state of the parser matches s. */
            void add_handler( const xerces_string& s, const parse_handler& ph );

            /// Call this function to start to stream all parsed xml to an internal stream.
            void start_create_inner_xml();

            /// Call this to stop stream all parsed xml to an internal stream.
            void stop_create_inner_xml();
            
            /// Call this function to get the internal xml stream.
            /** To make the stream be filled with data, call start_create_inner_xml 
                to start generate data and stop_create_inner_xml to finish it. */
            t_string get_inner_xml() const;

            /// Called when an error is detected.
            /** Converts the exception to a base_exception and throws that instead.*/
            virtual void error(const XERCES_CPP_NAMESPACE::SAXParseException& exc);    

            /// Called when a fatal error is detected.
            /** Converts the exception to a base_exception and throws that instead. */
            virtual void fatalError(const XERCES_CPP_NAMESPACE::SAXParseException& exc);

            /// Called by the SAXParser when a new element is entered.
            /** The traverser changes its state, and checks for any registered
                handlers to call. The traverse_state has a valid m_attributes
                pointer but m_chars and m_length are both 0. */
            virtual void startElement(const XMLCh* const name, XERCES_CPP_NAMESPACE::AttributeList&  attributes );

            /// Called by the SAXParser when data between start and end tag is entered.
            /** The traverser calls any registered handlers for the current state 
                The traverse_state has a invalid m_attributes pointer 
                but m_chars and m_length are both valid.*/
            virtual void characters(const XMLCh* const chars, const unsigned int length);

            /// An end element has been entered
            /** The traverser calls any registered handler to notifiy
            that the element is done. The traverse_state has an invalid m_attributes
            pointer and m_chars and m_length are both 0. */
            virtual void endElement(const XMLCh* const);

            /// Get the current state of the traverser
            /** Example /some/path/to/the/current/element. */
            const xerces_string& get_state() const { return m_state; } 

        private:

            object_ptr load_and_parse(  const XERCES_CPP_NAMESPACE::InputSource* bin_data ,
                                        const olib::t_path* xml_doc_path,
                                        const schema_map& schemas );

            void stream_xml_start_tag(  const XMLCh* const elem_name,
                                        XERCES_CPP_NAMESPACE::AttributeList* attributes );

            void stream_xml_content(    const XMLCh* const chars,
                                        const unsigned int length);

            void stream_xml_end_tag( const XMLCh* const chars);

            
            /// Map of functions to handle different states of the parser
            typedef std::map< xerces_string, parse_handler> parse_handler_map;

            /// Put you handler functions here.
            /** You should match your handler with a state string.
                If you have this xml: <apf><version></version>...</apf>
                and want to have a handler for the version tag, add a key
                to the map that looks like this "/apf/version".*/
            parse_handler_map m_handlers;

            /// The current state of the parser.
            xerces_string m_state;

            /** Set this to true if you want all seen data
                to be added to the m_xml_data member. To stop
                this process, the handler that set the flag to true,
                is called again with all its parameters set to 0.
                The use the m_xml_data and set the m_create_inner_xml to
                false. */
            bool m_crate_inner_xml;
            t_stringstream m_xml_data;
            xerces_sax_handler_ptr m_parser;
            
            sax_parser_ptr create_traverser( const schema_map& schemas );
            sax_parser_ptr m_traverser;
            
        };
    }
}

#endif // _CORE_XERCES_SAX_TRAVERSER_H_

