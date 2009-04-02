#ifndef _CORE_PLUGIN_CENTRAL_PLUGIN_FACTORY_H_
#define _CORE_PLUGIN_CENTRAL_PLUGIN_FACTORY_H_

#include "./object.hpp"
#include "./plugin_loader.hpp"
#include "./enforce_defines.hpp"
#include "./str_util.hpp"

/** @file central_plugin_factory.h */

namespace olib
{
	namespace opencorelib
	{
        /// Singleton used to load plug-ins in amf
        /** Some examples might be useful:
        <pre>
//// The path to the folder where the plug-ins are located.
const olib::t_path amf_pp = _CT("/plugins");
//// The path to the schemas used by amf
//// We need the amf-plugins.xsd schema to validate plug-in xml documents.
olib::t_path schema_path = special_folder::get( special_folder::amf_resources, 
                                    _CT(""), _CT("") ) / _CT("schemas/amf-plugin.xsd");

std::vector<t_string> xml_docs;
xml_docs.push_back(_CT("my_test_plugin.xml"));
//// Use the_plugin_loader to load the information about the plugins.
the_plugin_loader::instance().load_plugin_metadata( amf_pp, schema_path, xml_docs );
        </pre>

        After loading the xml-descriptions, the_plugin_loader can be used, for 
        example by the create_plugin template functions, or like this:
        <pre>
        object_ptr a_plugin = the_plugin_loader::instance().create(id);
        </pre>
        @author Mats Lindel&ouml;f
        @sa create_plugin<T> */

        class CORE_API the_plugin_loader
        {
        public:
            static plugin_loader& instance();
        };

        /// Creates a plug-in and uses dynamic_pointer_cast to convert it to the desired type
        /** @param id The id of the class to create.
            @param loader The plugin_loader to use when calling create.
            @return The created instance cast to the desired type, given by the template parameter.
            @throw base_exception if the id can't be found. */
		template <class desired_interface>
        boost::shared_ptr<desired_interface> create_plugin( const t_string& id, 
                                                            const plugin_loader& loader )
		{
			object_ptr a_plugin = loader.create(id);
            ARENFORCE_MSG( a_plugin, "The given loader could not create the desired class")(id);
            boost::shared_ptr<desired_interface> res;
            ARENFORCE_MSG( res = boost::dynamic_pointer_cast<desired_interface>(a_plugin), 
                "The created class does not support the given interface.")
                            (id)(str_util::to_t_string(typeid(desired_interface).name()));
			return res;
		}

        /// Creates a plug-in and uses dynamic_pointer_cast to convert it to the desired type.
        /** Simply calls create_plugin<T> using the the_plugin_loader as the plugin_loader
            instance. 
            @param id The id of the class to create.
            @throw base_exception if the desired id can't be found. */
        template <class desired_interface>
        boost::shared_ptr<desired_interface> create_plugin( const t_string& id )
        {
            return create_plugin<desired_interface>( id, the_plugin_loader::instance() );
        }
	}
}

#endif // _CORE_PLUGIN_CENTRAL_PLUGIN_FACTORY_H_
