#ifndef _CORE_PLUGIN_PLUGIN_CLASS_BASE_IMPLEMENTATION_H_
#define _CORE_PLUGIN_PLUGIN_CLASS_BASE_IMPLEMENTATION_H_

#include "./object.hpp"
#include "./initializer.hpp"

namespace olib
{
	namespace opencorelib
	{
		/// It is recommended that plug-in classes inherit from this class instead of plugin_class.
		/** Requires the inheritor to implement a static get_class_id function. 
            @param inheritor The class that inherits from this class.
            @param local_factory_type The type of the singleton that has an Instance function
                    and a type that supports the functions: 
                    <ul>
                    <li> register_creator
                    <li> increment_instance_counter
                    <li> decrement_instance_counter
                    </ul>
                   A concrete class that supports this is assembly_class_creator.
                   @see assembly_class_creator
            @author Mats Lindel&ouml;f*/
		template< class inheritor, class local_factory_type >
		class plugin_class_base_implementation : public object
		{
		public:

            /// The type of this class (when instanciated)
            typedef plugin_class_base_implementation<inheritor, local_factory_type > my_type;

            /// Make sure that pre-main functionality is working.
            /** By calling the foo function of m_init, the compiler 
                must instantiate the m_init member of the template.
                Doing this registers this class with the local_factory_type
                during the c-runtime startup. */
            plugin_class_base_implementation() { m_init.foo(); }
            
            /// Ensure correct destruction in derived classes.
            virtual ~plugin_class_base_implementation() {}

            /// Called during pre-main when the m_init member is instantiated.
            static int do_premain_register()
            {
                local_factory_type::Instance().register_creator( inheritor::get_class_id(), object_factory); 
                return 0; 
            }

            /// Currently empty, could unregister with the local_factory if needed.
            static int do_final_unregister()
            {
                // Not needed, will always be unloaded when the dynamic lib is unloaded
                // local_factory_type::Instance().unregister_creator( inheritor::get_class_id() );
                return 0;
            }

            /// Create a new instance of the derived class.
            /** Increases the outstanding object count of the local_factory_type. */
            static object_ptr object_factory()
            {
                local_factory_type::Instance().increment_instance_counter();
                return object_ptr(new inheritor(), &static_deleter );
            }

            /// Deletes an object created in the object_factory function.
            /** Decreases the outstanding object count of the local_factory_type */
            static void static_deleter( inheritor* i)
            {
                delete i;
                local_factory_type::Instance().decrement_instance_counter();
            }

        private:

            static const initializer< my_type > m_init; 
		};

	    /// This static template creates the static initializer object we're after.
        template < class inheritor, class local_factory_type >
        const initializer< plugin_class_base_implementation<inheritor, local_factory_type > > 
                plugin_class_base_implementation<inheritor, local_factory_type >::m_init 
                    = initializer< plugin_class_base_implementation<inheritor, local_factory_type > >();
	}
}

#endif // _CORE_PLUGIN_PLUGIN_CLASS_BASE_IMPLEMENTATION_H_

