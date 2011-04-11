#include "precompiled_headers.hpp"
#include "../str_util.hpp"
#include "./plugin_metadata_saxhandler.hpp"
#include "../plugin_metadata.hpp"

#include "../enforce_defines.hpp"
#include "../enforce.hpp"

#include "../assert_defines.hpp"
#include "../assert.hpp"

#include <boost/bind.hpp>

using namespace XERCES_CPP_NAMESPACE;
using namespace olib::opencorelib::str_util;

namespace olib
{
   namespace opencorelib
    {
        plugin_metadata_sax_handler::plugin_metadata_sax_handler()
            : m_res( new plugin_metadata())
        {
        }

        plugin_metadata_sax_handler::~plugin_metadata_sax_handler()
        {
        }

        void plugin_metadata_sax_handler::reset_result()
        {
            m_res = plugin_metadata_ptr( new plugin_metadata() );
        }

        void plugin_metadata_sax_handler::register_handlers( xerces_sax_traverser& traverser, 
                                                            const xerces_string& state_prefix)
        {
            #ifdef OLIB_ON_WINDOWS
            traverser.add_handler( 
                    state_prefix + L2("/amf_plugin_description/assembly_name/windows"),
                    boost::bind( &plugin_metadata_sax_handler::handle_assembly_name, this, _1) );

            #elif defined( OLIB_ON_MAC )
            traverser.add_handler( 
                state_prefix + L2("/amf_plugin_description/assembly_name/mac"),
                boost::bind( &plugin_metadata_sax_handler::handle_assembly_name, this, _1));

            #else
            traverser.add_handler( 
                state_prefix + L2("/amf_plugin_description/assembly_name/linux"),
                boost::bind( &plugin_metadata_sax_handler::handle_assembly_name, this, _1));

            #endif
            traverser.add_handler( 
                state_prefix + L2("/amf_plugin_description/classes/class"),
                boost::bind( &plugin_metadata_sax_handler::handle_class, this, _1));

            traverser.add_handler( 
                state_prefix + L2("/amf_plugin_description/classes/class/description"),
                boost::bind( &plugin_metadata_sax_handler::handle_class, this, _1));
        }

        void plugin_metadata_sax_handler::parsing_done()
        {
            // No internal parsers, nothing to do.
        }

        object_ptr plugin_metadata_sax_handler::get_result() const
        {
            return m_res;
        }

        void plugin_metadata_sax_handler::handle_assembly_name( const traverse_state& st )
        {
            if( st.m_location == xml_location::element_start && st.m_attributes )
            {
                const XMLCh* dname = st.m_attributes->getValue( L2("debug").c_str() );
                const XMLCh* rname = st.m_attributes->getValue( L2("release").c_str() );

                m_res->set_assembly_debug_name( xml::to_t_string(dname, XMLString::stringLen(dname) ) );
                m_res->set_assembly_release_name( xml::to_t_string(rname, XMLString::stringLen(dname) ));
            }
        }

        void plugin_metadata_sax_handler::handle_class(  const traverse_state& st )
        {
            if( st.m_attributes && ends_with(st.m_traverser->get_state() , L2("class")) )
            {
                plugin_class_description_ptr cd( new plugin_class_description() );
                const XMLCh* id = st.m_attributes->getValue( L2("id").c_str() );
                cd->set_id( xml::to_t_string(id, XMLString::stringLen(id) ) );
                m_res->get_class_descriptions().push_back( cd );
            }
            else if( st.m_chars && ends_with(st.m_traverser->get_state(), L2("class/description"))) 
            {
                ARENFORCE( m_res->get_class_descriptions().size() > 0 );
                m_res->get_class_descriptions().back()
                    ->set_description( xml::to_t_string(st.m_chars, st.m_length));
            }
        }
    }
}
