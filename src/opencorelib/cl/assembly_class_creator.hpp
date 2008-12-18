#ifndef _CORE_PLUGIN_ASSEMBLY_CLASS_CREATOR_H_
#define _CORE_PLUGIN_ASSEMBLY_CLASS_CREATOR_H_

#include "./assembly_class_factory.hpp"

/** @file assembly_class_factory.h */

/// Ardome Media Framework
/** Framework used to handle media, edit timelines, filter and
    convert media, add graphic and finally look and listen
    to the result on a client machine or remote server. */
namespace olib
{
    /// Contains core classes and functions needed by the amf infrastructure.
	namespace opencorelib
	{

        /// Function signature used to by plug-in creators to create instances in an assembly.
		typedef object_ptr (* plugin_creator )();
        
        /// This class is used in plug-in assemblies to create instances of plug-in classes.
        /** Below is a typical example of the usage of the class.
            A singleton is created in the assembly (dll on windows, so on linux/mac).
            That singleton is used by one the and only function needed to be exported to 
            make an assembly a amf plug-in, get_assembly_class_factory, 
            and by all plug-in classes when inheriting from plugin_class_base_implementation.
            <pre>              
typedef Loki::SingletonHolder<  assembly_class_creator,
                                Loki::CreateStatic,
                                Loki::NoDestroy > the_assembly_class_creator;

extern "C"
{
    DLL_EXPORT assembly_class_factory& get_assembly_class_factory()
    {
        return the_assembly_class_creator::Instance();
    }
};

//// Here is an example class that uses the assembly_class_creator.
//// It automatically registers pre-main.
class test_plugin : public logtarget,
    public plugin_class_base_implementation< test_plugin, the_assembly_class_creator >
{
    ....
            </pre>
            @sa plugin_class_base_implementation
            @sa plugin_metadata
            @sa assembly_class_factory_status
            @author Mats Lindel&ouml;f*/
        class CORE_API assembly_class_creator : public assembly_class_factory
		{	
		public:

			assembly_class_creator();

            /// Register a new class that can be created by this factory
            /** This function is used by the plugin_class_base_implementation 
                during pre-main registration. It simplifies the creation 
                of plug-ins.
                @param id The id to match against when calling create.
                @param pc The function that will be called to create the plug-in instance.
                @throws base_exception if the id is already occupied. */
			void register_creator(const t_string& id, plugin_creator pc);

            /// Unregisters a previously registered creation function.
			void unregister_creator(const t_string& id);	

            /// Given a class id, create an object instance
            /** This function is used by functions such as create_plugin to
                create an instance of a class from a string identifier. 
                @param class_id The id of the class to create an instance of.
                @return An instance of the specified class.
                @throws base_exception If the class_id isn't registered with register_creator.*/
            virtual object_ptr create( const t_string& class_id );

            /// Get the status of this factory.
            /** The returned instance holds a mutex to prevent the
                status from changing while examining the result. For instance,
                if the current outstanding reference count is zero, this 
                prevents other threads from changing that, making it 
                safe to unload the plug-in.*/ 
            virtual assembly_class_factory_status_ptr get_factory_status() const;

            /// Should be called by factory functions when new instances are created.
            /** This function is thread-safe, an internal mutex is used to 
                make sure that the instance count is correct. The 
                plugin_class_base_implementation always calls this function 
                when a new instance of a class is created.*/
            boost::int32_t increment_instance_counter();

            /// Reduces the instance count when a plug-in class gets destroyed.
            /** Should be called in the destructor of the shared_ptr 
                thats holding the the instance. The plugin_class_base_implementation
                makes sure that new instances always are created with
                a deleter that calls this function. */
            boost::int32_t decrement_instance_counter();

        private:
            typedef std::map< t_string, plugin_creator > creator_map;
            creator_map m_assembly_plugins;
            boost::int32_t m_instance_counter;
            mutable boost::recursive_mutex m_mtx;
		};
				
	}
}

#endif // _CORE_PLUGIN_ASSEMBLY_CLASS_CREATOR_H_

