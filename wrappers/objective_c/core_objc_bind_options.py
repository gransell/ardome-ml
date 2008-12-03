
import owl.wrapper.objc.objc_bind_options


conversion_dict = {'olib::opencorelib::frames' : 'NSUInteger',
 					'olib::opencorelib::rational_time' : 'OlibOpencorelibRationalTime',
					'boost::rational<long>' : 'OlibOpencorelibRationalTime',}

class core_objc_bind_options( owl.wrapper.objc.objc_bind_options ):
	"""docstring for core_objc_bind_options"""
	def __init__( self, owl_path, assemblies ):
		super(core_objc_bind_options, self).__init__( owl_path, assemblies )
	
	def get_standard_header_includes(self, type_instance):
		"""docstring for get_standard_header_includes"""
		return super(core_objc_bind_options, self).get_standard_header_includes( type_instance )
	
	def get_standard_objc_source_includes(self, type_instance):
		"""docstring for get_standard_objc_source_includes"""
		ret = super(core_objc_bind_options, self).get_standard_objc_source_includes()
		ret.extend(['"CoreConverts.h"'])
		return ret

	def get_standard_source_files( self ):
		"""docstring for get_standard_source_files"""
		ret = super(core_objc_bind_options, self).get_standard_source_files()
		ret.extend(['objective_c/src/CoreConverts.mm'])
		return ret
		
	def get_conversion_dictionary( self, type_instance ):
		"""docstring for get_conversion_dictionary"""
		ret = super(core_objc_bind_options, self).get_conversion_dictionary( type_instance )
		for k in conversion_dict.keys() :
			ret[k] = conversion_dict[k]
		return ret
	
	def get_converted_type_string( self, type_instance ):
		"""docstring for get_converted_type"""
		if not conversion_dict.has_key( type_instance.get_name() ) :
			return super(core_objc_bind_options, self).get_converted_type_string( type_instance )
		return conversion_dict[type_instance.get_name()]
		
	def get_standard_source_includes(self, type_instance):
		"""docstring for get_standard_source_includes"""
		ret =  super(core_objc_bind_options, self).get_standard_source_includes( type_instance )
		ret.extend( ['"opencorelib/cl/core.hpp"', 
					'"opencorelib/cl/machine_readable_errors.hpp"',
					'"opencorelib/cl/assert_defines.hpp"',
					'"opencorelib/cl/enforce_defines.hpp"',
					'"opencorelib/cl/loghandler.hpp"',
					'"opencorelib/cl/log_defines.hpp"',
					'"opencorelib/cl/logtarget.hpp"',
					'"opencorelib/cl/base_exception.hpp"',
					'"opencorelib/cl/special_folders.hpp"',
					'"opencorelib/cl/plugin_loader.hpp"',
					'"opencorelib/cl/central_plugin_factory.hpp"',] )
		return ret