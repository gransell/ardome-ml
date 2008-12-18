#ifndef _CORE_PLUGIN_ASSEMBLY_CLASS_FACTORY_H_
#define _CORE_PLUGIN_ASSEMBLY_CLASS_FACTORY_H_

#include <boost/thread.hpp>
#include "./object.hpp"

namespace olib
{
	namespace opencorelib
	{
        /// Describes the current status of a plug-in, and holds an external mutex lock.
        /** An instance of this class holds the number of instances that 
            has been created by a certain assembly_class_factory. It also 
            holds a lock on the factory to prevent the state from changing
            while investigating it. This means that instances of this class should
            only live for a short while, otherwise dead-lock may occur. 
            @sa assembly_class_factory
            @author Mats Lindel&ouml;f */
        class assembly_class_factory_status
        {
        public:
            /// Create a new status object
            /** @param mtx The external mutex to lock.
                @param instance_count The current instance count of the factory. */
            assembly_class_factory_status( 
                boost::recursive_mutex& mtx, boost::int32_t instance_count )
                : m_instance_count(instance_count), m_lock(mtx) {}

            /// Get the current number of outstanding, non-destroyed instances of this factory.
            boost::int32_t get_instance_count() const { return m_instance_count; }

        private:
            boost::int32_t m_instance_count;
            boost::recursive_mutex::scoped_lock m_lock;
        };

        typedef boost::shared_ptr< assembly_class_factory_status > assembly_class_factory_status_ptr;  

		/// Interface for classes that create objects in satelite assemblies, or plug-ins.
        /** @sa assembly_class_factory_status
            @sa assembly_class_creator
            @author Mats Lindel&ouml:f */
		class assembly_class_factory 
		{
		public:
			virtual ~assembly_class_factory() {}

            /// Create an instace of the class with the given id
            /** @param class_id The id of the class of which to create an instance.
                @return An instance of the given class.
                @throw base_exception if the class_id can't be found. */
			virtual object_ptr create( const t_string& class_id ) = 0;

            /// Returns the current outstanding instance count. 
            /** The returned instance also holds a lock on the class factory to
                make sure that the status is not changed while investigating
                the status. Make sure to release the pointer since creation
                of new instances are hindered while you're holding it. */
            virtual assembly_class_factory_status_ptr get_factory_status() const = 0;
		};
	}
}

#endif // _CORE_PLUGIN_ASSEMBLY_CLASS_FACTORY_H_

