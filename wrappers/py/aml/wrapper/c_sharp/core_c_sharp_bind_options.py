import os
import owl.wrapper.c_sharp.c_sharp_bind_options


class core_c_sharp_bind_options( owl.wrapper.c_sharp.c_sharp_bind_options ):
	
	def __init__( self, owl_path, output_path, assemblies ):
		super(core_c_sharp_bind_options, self).__init__( owl_path, output_path, assemblies )	
		self.precompiled_header_info = ( True, 'precompiled_headers.hpp',  'precompiled_headers.cpp' )
		
		self._conversion_dict.update( {'olib::opencorelib::frames' : 'System::Int32',
					'olib::opencorelib::rational_time' : 'Olib::Opencorelib::RationalTime',
					'boost::rational<long>' : 'Olib::Opencorelib::RationalTime',
					'olib::opencorelib::point' : 'System::Windows::Point', } )
	
	def get_standard_header_includes(self, type_instance):
		ret = super(core_c_sharp_bind_options, self).get_standard_header_includes( type_instance )
		return ret;

	def get_standard_source_files( self ):
		ret = super(core_c_sharp_bind_options, self).get_standard_source_files()
		# ret.extend(['objective_c/src/CoreConverts.mm'])
		return ret
	
	def generate_precompiled_headers( self ) :
		precompiled_hpp_file = os.path.join( self.output_path, self.precompiled_header_info[1])
		precompiled_cpp_file = os.path.join( self.output_path, self.precompiled_header_info[2])
		hpp = open(precompiled_hpp_file, "w+")
		cpp = open(precompiled_cpp_file, "w+")
		
		to_include = [ 'opencorelib/cl/core.hpp', 
						'opencorelib/cl/machine_readable_errors.hpp',
						'opencorelib/cl/assert_defines.hpp',
						'opencorelib/cl/enforce_defines.hpp',
						'opencorelib/cl/loghandler.hpp',
						'opencorelib/cl/log_defines.hpp',
						'opencorelib/cl/logtarget.hpp',
						'opencorelib/cl/base_exception.hpp',
						'src/c_sharp/RationalTime.hpp', 
						'src/c_sharp/AMLConversionFunctions.hpp',
						'owl/SharedPtr.hpp',
						'owl/owl_wrap_manager.hpp',
						'owl/ref_wrapper.hpp',
						'owl/CommonDefines.hpp',
						]
						
		
		
						
		hpp_content = '#pragma once\n'
		for inc in to_include:
			hpp_content += '#include "' + inc + '"\n'
		
		hpp_content += '\n#define LOKI_CLASS_LEVEL_THREADING\n'
		hpp_content += '#include <loki/Singleton.h>\n\n'
		
		hpp.write( hpp_content )
		cpp.write('#include "' + self.precompiled_header_info[1] + '"\n');
	
		return True
		
	def get_precompiled_header_info( self ) :
		return self.precompiled_header_info
		
	def get_standard_source_includes(self, type_instance):
		ret =  super(core_c_sharp_bind_options, self).get_standard_source_includes( type_instance )
		ret.extend( [ self.precompiled_header_info[1] ] )
		return ret
		
	def get_wrapper_prologue(self):
		return "WRAPPER_PROLOGUE {"
	
	def get_wrapper_epilogue(self):
		return "} WRAPPER_EPILOGUE();"
	