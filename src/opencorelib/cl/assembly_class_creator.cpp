#include "precompiled_headers.hpp"
#include "./assembly_class_creator.hpp"

#include "assert_defines.hpp"
#include "assert.hpp"
#include "enforce_defines.hpp"
#include "enforce.hpp"

using namespace boost;

namespace olib
{
	namespace opencorelib
	{

        assembly_class_creator::assembly_class_creator() : m_instance_counter(0)
		{
		}
	
		void assembly_class_creator::register_creator(const t_string& id, plugin_creator pc)
		{
            boost::recursive_mutex::scoped_lock lck( m_mtx );
			creator_map::iterator fit = m_assembly_plugins.find(id);
            ARENFORCE( fit == m_assembly_plugins.end())(id).msg(_T("Id not unique. Can not register plugin."));
			m_assembly_plugins[id] = pc;
		}

		void assembly_class_creator::unregister_creator(const t_string& id)
		{
            boost::recursive_mutex::scoped_lock lck( m_mtx );
			creator_map::iterator fit = m_assembly_plugins.find(id);
			if(fit != m_assembly_plugins.end()) 
                        m_assembly_plugins.erase(fit);
		}

        object_ptr assembly_class_creator::create( const t_string& class_id )
        {
            boost::recursive_mutex::scoped_lock lck( m_mtx );
            creator_map::iterator fit = m_assembly_plugins.find(class_id);
            ARENFORCE_MSG( fit != m_assembly_plugins.end(), "Id %s unknown")(class_id);
            return fit->second();
        }

        assembly_class_factory_status_ptr assembly_class_creator::get_factory_status() const
        {
            assembly_class_factory_status_ptr 
                st( new assembly_class_factory_status(m_mtx, m_instance_counter));
            return st;
        }

        boost::int32_t assembly_class_creator::increment_instance_counter()
        {
             boost::recursive_mutex::scoped_lock lck( m_mtx );
             m_instance_counter += 1;
             ARENFORCE_MSG(m_instance_counter >= 0, "Logical error")(m_instance_counter);
             return m_instance_counter;
        }

        boost::int32_t assembly_class_creator::decrement_instance_counter()
        {
             boost::recursive_mutex::scoped_lock lck( m_mtx );
             m_instance_counter -= 1;
             ARENFORCE_MSG(m_instance_counter >= 0, "Logical error")(m_instance_counter);
             return m_instance_counter;
        }

	}
}

