#include "precompiled_headers.hpp"

#include <boost/test/test_tools.hpp>
#include <boost/filesystem/operations.hpp>

#include "opencorelib/cl/special_folders.hpp"
#include "opencorelib/cl/plugin_metadata.hpp"
#include "opencorelib/cl/plugin_loader.hpp"
#include "opencorelib/cl/central_plugin_factory.hpp"

#include "opencorelib/cl/logtarget.hpp"

using namespace olib;
using namespace olib::opencorelib;
namespace fs = boost::filesystem;

void test_plugin()
{
	olib::t_path schema_path = special_folder::get( special_folder::amf_resources ) / _CT( "schemas/amf-plugin.xsd" );
	olib::t_path xml_path = special_folder::get( special_folder::amf_resources ) / _CT("examples/amf_plugin_example.xml");
	
	schema_map schemas;
	schemas[_CT("http://www.ardendo.com/amf/core/")] = schema_path;

	plugin_metadata_ptr md = plugin_metadata_from_xml(xml_path, schemas);

	olib::t_path plugin_path = special_folder::get( special_folder::plugins); 
	plugin_loader loader;
	std::vector<t_string> plugin_names;
	plugin_names.push_back(_CT("plugin_test_assembly.xml"));
	loader.load_plugin_metadata( plugin_path, schema_path, plugin_names );

	{
		object_ptr cptr = create_plugin< object >( _CT("test_plugin"), loader );
		BOOST_REQUIRE_MESSAGE( cptr, "Could not create test_plugin.");
		logtarget_ptr lt = boost::dynamic_pointer_cast< logtarget >(cptr);
		BOOST_REQUIRE_MESSAGE( lt, "amf_standard_plugin should implement the logtarget interface");
		the_log_handler::instance().add_target(lt);

		ARLOG_WARN("Now the plugin should be able to log this... check file in tmp-dir!");

		the_log_handler::instance().remove_target(lt);
	}
}
