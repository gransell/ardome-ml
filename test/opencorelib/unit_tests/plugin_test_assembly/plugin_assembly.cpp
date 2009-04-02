#include "precompiled_headers.hpp"
#include <boost/filesystem/path.hpp>

#include "opencorelib/cl/logtarget.hpp"
#include "opencorelib/cl/assembly_class_creator.hpp"
#include "opencorelib/cl/plugin_class_base_implementation.hpp"
#include "opencorelib/cl/special_folders.hpp"
#include "opencorelib/cl/utilities.hpp"

using namespace olib;
using namespace olib::opencorelib;
namespace fs = boost::filesystem;

#include <loki/Singleton.h>

typedef Loki::SingletonHolder<	assembly_class_creator,
                                Loki::CreateStatic,
                                Loki::NoDestroy > the_assembly_class_creator;

extern "C"
{
    OLIB_DLL_EXPORT assembly_class_factory& get_assembly_class_factory()
    {
        return the_assembly_class_creator::Instance();
    }
};
 

class test_plugin : public logtarget,
    public plugin_class_base_implementation< test_plugin, the_assembly_class_creator >
{
public:
    test_plugin() {}
    static const t_string get_class_id() { return _CT("test_plugin"); }

    virtual void log( invoke_assert& a, const TCHAR* log_source)
    {

    }

    virtual void log( base_exception& e, const TCHAR* log_source) 
    {

    }

    virtual void log( logger& log_msg, const TCHAR* log_source)  
    {
        olib::t_path tmp_dir = special_folder::get( special_folder::temp) / _CT( "core_unittests" );
		
		utilities::make_sure_path_exists(tmp_dir);

        fs_t_ofstream ofs( tmp_dir / _CT( "plugin_log_test.log" ), std::ios_base::app );
        ofs << log_msg.get_context()->as_xml() << std::endl;
    }

private:

};

