#ifndef _CORE_LOCAL_PLUGIN_FACTORY_H_
#define _CORE_LOCAL_PLUGIN_FACTORY_H_

#include <au_base/plugin_class.h>
#include <au_base/plugin.h>

namespace ardendo
{
	namespace common
	{
		
		class AUBASE_API CLocal_plugin_factory : public IPlugin_factory
		{	
		public:

			cLocal_plugin_factory();
			cLocal_plugin_factory( const t_string& name);

            typedef boost::function< boost::shared_ptr< CPlugin_class > () > Plugin_creator; 
			
            void register(const t_string& sid, Plugin_creator pc);
			void unregister(const t_string& sid);

			// IPlugin_factory functions.
            virtual bool can_unload_now() const;
            virtual std::vector< t_string > get_class_ids() const;
            virtual CClass_description get_class_description( const t_string& cid) const;
            virtual boost::shared_ptr<CPlugin_class> create_class_instance( const t_string& cid );
            virtual const t_string& name() const;

        private:
 
            typedef std::map< t_string, Plugin_creator > Id_creator_map;
            Id_creator_map m_Local_plugins;
            t_string m_Name;
		};
	}
}

#endif // _CORE_LOCAL_PLUGIN_FACTORY_H_

