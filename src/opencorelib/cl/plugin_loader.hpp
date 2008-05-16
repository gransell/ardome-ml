#ifndef _CORE_PLUGIN_PLUGIN_LOADER_H_
#define _CORE_PLUGIN_PLUGIN_LOADER_H_

#include "./assembly_class_factory.hpp"
#include "./object.hpp"
#include <boost/filesystem/path.hpp>
#include <vector>

namespace olib
{
	namespace opencorelib
	{
        /// A C function that will be called to load a amf plugin. 
        typedef assembly_class_factory& (*get_assembly_class_factory)(void); 

        /// Holds a reference to a amf plug-in assembly.
        /** Holds a handle to a loaded assembly, and can 
            retrieve the assembly_class_factory of the plug-in
            to users of this class. 
            @author Mats Lindel&ouml;f */
        class CORE_API assembly_reference
        {
        public:
            /// Create an empty reference.
            assembly_reference();

            /// Will not unload, unload must be called explicitly.
            ~assembly_reference();

            /// Load an assembly at the given location.
            /** @param p The path to the assembly to Load
                @throws A base_exception if the load fails. */
            void load( const olib::t_path& p );

            /// Unload a previously loaded assembly.
            /** @throws A base_exception if the unload fails. */
            void unload();

            /// Has the assembly been loaded.
            bool is_loaded() const { return m_handle != 0; }

            /// Get the factory of the loaded assembly.
            /** @return The assembly_class_factory of the loaded plug-in.
                @throws A base_exception if the assembly hasn't been loaded. */
            assembly_class_factory& get_factory();
        private:
            boost::int64_t m_handle;
            olib::t_path m_path;
            get_assembly_class_factory m_get_factory_function;
        };

        typedef boost::shared_ptr< assembly_reference > assembly_reference_ptr;

        /// Loads a number of amf plug-ins.
        /** An instance of this class can load plug-in assemblies
            and create instances of classes provided by the loaded
            plug-ins. 
            @author Mats Lindel&ouml;f */
        class CORE_API plugin_loader : public boost::noncopyable
		{			
		public:
            /// Create an empty plugin_loader.
            plugin_loader() {}
			~plugin_loader(void){ }

			/// Load metadata describing the plugins. No assemblies are loaded yet.
            /** @param location The base location of the plug-in xml files to load.
                @param plugin_schema_location The location of the schema-file that describes
                    the syntax of the plug-in xml files.
                @param metadata_file_names A vector of xml filenames that 
                        describe the contents of assemblies in the same folder as the xml files. */
            void load_plugin_metadata(  const olib::t_path& location,
                                        const olib::t_path& plugin_schema_location,
                                        const std::vector< t_string >& metadata_file_names );

            /// Create an instance of a plug-in given an id.
            /** No plug-ins are actually loaded before a call to this
                function occurs. This enables amf to start with greater 
                speed. 
                @return An instance of the class with the given id.
                @throws A base_exception is the id wasn't registered. */
            object_ptr create( const t_string& id ) const; 

            /// Will query the loaded plug-ins if they have outstanding instance counts, and unload them if not.
            void unload_all_unused_assemblies();

        private:

            void load_and_add_metadata( const t_string& xml_file_name,
                                        const olib::t_path& plugin_schema_location);

            mutable boost::recursive_mutex m_mtx;
            olib::t_path m_base_path;
            t_string all_registered_classes() const;

            typedef std::map< olib::t_path, assembly_reference_ptr > assembly_map;
            mutable assembly_map m_assemblies;

            typedef std::map< t_string,  olib::t_path > class_to_assembly_map;
            class_to_assembly_map m_class_to_assembly;
		};
	}
}

#endif // _CORE_PLUGIN_PLUGIN_LOADER_H_

