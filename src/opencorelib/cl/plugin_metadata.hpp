#ifndef _CORE_PLUGIN_PLUGIN_METADATA_H_
#define _CORE_PLUGIN_PLUGIN_METADATA_H_

#include "./typedefs.hpp"
#include "./object.hpp"
#include "./minimal_string_defines.hpp"

namespace olib
{
   namespace opencorelib
    {
        /// Describes a plug-in class in an amf plug-in assembly
        /** The id of a plug-in class is probably given by a 
            static function in the plug-in class called get_class_id.
            @see plugin_class_base_implementation
            @author Mats Lindel&ouml;f */
        class CORE_API plugin_class_description
        {
        public:
            /// Create an empty description.
            plugin_class_description() {}

            /// Set some members of the description.
            /** @param id The id of this plug-in class.
                @param desc A description of what this plug-in is doing. */
            plugin_class_description( const t_string& id, const t_string& desc )
                : m_id(id), m_description(desc)
            {
            }

            /// Get the id of this class.
            const t_string& get_id() const { return m_id; }

            /// Set the id of this class.
            void set_id( const t_string& id ) { m_id = id; }

            /// Get the description of this class.
            const t_string& get_description() const { return m_description; }

            /// Set the description of this class.
            void set_description( const t_string& desc ) { m_description = desc; }

        private:
            t_string m_id, m_description;

        };

        typedef std::vector< plugin_class_description_ptr > class_description_vector;
        
        
        /// Contains a collection of plugin_class_descriptions and the assembly name of the file to load.
        class CORE_API plugin_metadata : public object
        {
        public:
            /// Create an empty metadata instance.
            plugin_metadata();

            /// Get the plugin_class_descriptions of the described plug-in.
            class_description_vector& get_class_descriptions() { return m_descriptions; }
            
            /// Get the release name of the assembly to load to get access to the described classes.
            const t_string& get_assembly_release_name() const { return m_release_name; }

            /// Set the release name of the assembly to load to get access to the described classes.
            void set_assembly_release_name( const t_string& rn) { m_release_name = rn; }
            
            /// Get the debug name of the assembly to load to get access to the described classes.
            const t_string& get_assembly_debug_name() const { return m_debug_name; }

            /// Set the debug name of the assembly to load to get access to the described classes.
            void set_assembly_debug_name( const t_string& dn) { m_debug_name = dn; }
            
            CORE_API friend plugin_metadata_ptr 
            plugin_metadata_from_xml(   const olib::t_path& xml_path,
                                        const schema_map& schemas );
        private:
            t_string m_release_name, m_debug_name;
            class_description_vector m_descriptions;
        };

        /// Load an xml-file containing info about plug-in metadata.
        /** Example usage: <pre>
schema_map schemas;
schemas[_T("http://www.ardendo.com/amf/core/")] = plugin_schema_location;
plugin_metadata_ptr md = plugin_metadata_from_xml( full_path, schemas ); </pre>

            @param xml_path Path to the actual xml file to load.
            @param schemas A map of xml namespace names, mapped to schema files 
                    that validate that namespace. 
            @return A plugin_metadata instance if successful.
            @throws A base_exception if validation or loading fails.*/
        CORE_API plugin_metadata_ptr 
        plugin_metadata_from_xml(   const olib::t_path& xml_path,
                                    const schema_map& schemas );
    }
}

#endif // _CORE_PLUGIN_PLUGIN_METADATA_H_

