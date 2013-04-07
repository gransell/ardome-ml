#include "precompiled_headers.hpp"
#include "./plugin_loader.hpp"

#include "./assert_defines.hpp"
#include "./assert.hpp"
#include "./enforce_defines.hpp"
#include "./enforce.hpp"
#include "boost/bind.hpp"
#include "./base_exception.hpp"
#include "./log_defines.hpp"
#include "./logger.hpp"
#include "./loghandler.hpp"
#include "./machine_readable_errors.hpp"

#include "./str_util.hpp"
#include "./plugin_metadata.hpp"

#ifndef OLIB_ON_WINDOWS
#include <dlfcn.h>
#endif

namespace fs = boost::filesystem;
using namespace olib::opencorelib::str_util;

namespace olib
{
   namespace opencorelib
    {
        assembly_reference::assembly_reference() 
            : m_handle(0), m_get_factory_function(0)
        {

        }

        assembly_reference::~assembly_reference()
        {
            // Only onload explicitly.
            //unload();
        }

        void assembly_reference::load( const olib::t_path& p )
        {
            m_path = p;

            #ifdef OLIB_ON_WINDOWS
            HMODULE mod;
            ARENFORCE_WIN( mod = ::LoadLibraryW( p.c_str() ))( p.native() );

            ARENFORCE_WIN(m_get_factory_function = (get_assembly_class_factory)
                ::GetProcAddress( mod, "get_assembly_class_factory"))(p.native());
            m_handle = (boost::int64_t)mod;
            
            #else // mac or unix

            // CY: It is essential that RTLD_GLOBAL is used here for dynamic_cast to be
            // functional. All g++ apps should also link with -Wl,-E.
            void* mod;
            ARENFORCE_MSG_ERR( mod = dlopen( to_string(m_path.string() ).c_str( ), RTLD_GLOBAL | RTLD_NOW ), 
                              "Failed to load assembly",
                              error::dlopen_failed())(p.string())(" Error: ")(dlerror());


            ARENFORCE_MSG_ERR( m_get_factory_function = 
                    (get_assembly_class_factory)dlsym( mod, "get_assembly_class_factory" ),
                            "Failed to find the get_assembly_class_factory function",
                              error::dlsym_failed())
                    (p.string())(" Error: ")(dlerror()).reason("olib::opencorelib::plugin_loader::dlsym_failed");

            m_handle = (boost::int64_t)mod;
            #endif
        }

        void assembly_reference::unload()
        {
            if(m_handle == 0) return;
            #ifdef OLIB_ON_WINDOWS
                ARENFORCE_WIN( ::FreeLibrary( (HMODULE)(m_handle) ) == TRUE )
                                                    (m_path.native());
            #else // mac or unix
                ARENFORCE( dlclose( (void*)(m_handle) ) == 0 )
                        (m_path.string())(std::string( dlerror( ) ) );
            #endif
            m_handle = 0; 
        }

        assembly_class_factory& assembly_reference::get_factory()
        {
            ARENFORCE_MSG(m_get_factory_function, "Assembly not loaded");
            return m_get_factory_function();
        }

        void plugin_loader::load_and_add_metadata(  const t_string& xml_file_name,
                                                    const olib::t_path& plugin_schema_location)
        {
            try
            {            
                olib::t_path full_path = olib::t_path( m_base_path ) / olib::t_path( xml_file_name );
                ARLOG_DEBUG2( _CT("Loading plugin metdata from \"%1%\"."))(full_path.native());
                ARENFORCE_MSG_ERR(  fs::exists(full_path), _CT("The plugin file can not be found. Path=%s"), 
                                    olib::error::file_not_found())(full_path.native());
                    
                schema_map schemas;
                schemas[_CT("http://www.ardendo.com/amf/core/")] = plugin_schema_location;
                plugin_metadata_ptr md;
                
                ARENFORCE( md = plugin_metadata_from_xml( full_path, schemas ) )
                    .reason(_CT("olib::opencorelib::plugin_loader::metadate_load_failed"));
                
                #ifdef _DEBUG
                    olib::t_path assembly_path = m_base_path / md->get_assembly_debug_name();
                #else // release mode
                    olib::t_path assembly_path = m_base_path / md->get_assembly_release_name();
                #endif

                class_description_vector::const_iterator it(md->get_class_descriptions().begin()),
                                                        eit(md->get_class_descriptions().end());

                for( ; it != eit; ++it )
                    m_class_to_assembly[ (*it)->get_id() ] = assembly_path;
            }
            catch (const base_exception& ex )
            {
                #ifdef _DEBUG
                    base_exception::show(ex);
                #endif
                // Just swallow, this will have been logged already.
                // Continue to look for valid files.
            }
        }


        void plugin_loader::load_plugin_metadata(const olib::t_path& location,
                                                 const olib::t_path& plugin_schema_location,
                                                 const std::vector< t_string >& metadata_file_names )
        {
            boost::recursive_mutex::scoped_lock lck(m_mtx);
            m_base_path = location;

            std::for_each( metadata_file_names.begin(), metadata_file_names.end(),
                boost::bind( &plugin_loader::load_and_add_metadata, 
                                this, _1, boost::ref(plugin_schema_location) ) );
        }

        void plugin_loader::unload_all_unused_assemblies()
        {
            boost::recursive_mutex::scoped_lock lck(m_mtx);
            assembly_map::iterator it( m_assemblies.begin()),
                                    eit( m_assemblies.end());
            for(; it != eit; )
            {
                if( it->second )
                {
                    assembly_class_factory& f = it->second->get_factory();
                    assembly_class_factory_status_ptr s = f.get_factory_status();
                    if( s && s->get_instance_count() == 0)
                    {
                        it->second->unload();
                        m_assemblies.erase(it);
                        it = m_assemblies.begin();
                    }
                    else
                    {
                        it++;
                    }
                }
                else
                {
                    it++;
                }

            }
        }

        t_string plugin_loader::all_registered_classes() const
        {
            class_to_assembly_map::const_iterator fit(m_class_to_assembly.begin()),
                                                    eit(m_class_to_assembly.end());

            t_stringstream ss;
            for( size_t i(0); fit != eit; ++fit, ++i ) 
            {
                ss << fit->first;
                if( i + 1 < m_class_to_assembly.size() ) ss << _CT(", ");
            }

            return ss.str();
        }

        object_ptr plugin_loader::create( const t_string& id ) const
        {
            ARLOG_DEBUG("Trying to load plugin with id = %s")(id);
            boost::recursive_mutex::scoped_lock lck(m_mtx);
            class_to_assembly_map::const_iterator fit = m_class_to_assembly.find(id);
            ARENFORCE_MSG(fit != m_class_to_assembly.end(), "No such class is registered")
                (id.c_str())( all_registered_classes() )
                .reason(_CT("olib::opencorelib::plugin_loader::no_such_class_registered"));

            assembly_reference_ptr aref;
            assembly_map::const_iterator ait=  m_assemblies.find( fit->second );
            if( ait == m_assemblies.end() )
            {
                aref = assembly_reference_ptr( new assembly_reference() );
                aref->load( fit->second );
                m_assemblies[ fit->second ] = aref;
            }
            else
            {
                aref = ait->second;
            }

            return aref->get_factory().create(id);
        }
    }
}
