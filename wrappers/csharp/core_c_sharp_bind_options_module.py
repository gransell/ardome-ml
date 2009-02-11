import os
import owl.wrapper.c_sharp.c_sharp_bind_options

conversion_dict = {'olib::opencorelib::frames' : 'System::Int32',
					'olib::opencorelib::rational_time' : 'Olib::Opencorelib::RationalTime',
					'boost::rational<long>' : 'Olib::Opencorelib::RationalTime',
					'olib::opencorelib::point' : 'System::Windows::Point' }

class core_c_sharp_bind_options( owl.wrapper.c_sharp.c_sharp_bind_options ):
	
	def __init__( self, owl_path, output_path, assemblies ):
		super(core_c_sharp_bind_options, self).__init__( owl_path, output_path, assemblies )	
		self.precompiled_header_info = ( True, 'precompiled_headers.hpp',  'precompiled_headers.cpp' )
	
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
						'owl/wrapper/c_sharp/cppsrc/CommonDefines.hpp',
						'owl/wrapper/c_sharp/cppsrc/SharedPtr.hpp',
						'owl/wrapper/c_sharp/cppsrc/wrap_manager.hpp',
						'owl/wrapper/c_sharp/cppsrc/ref_wrapper.hpp']
						
		hpp_content = '#pragma once\n'
		for inc in to_include:
			hpp_content += '#include "' + inc + '"\n'
		
		hpp.write( hpp_content )
		cpp.write('#include "' + self.precompiled_header_info[1] + '"\n');
	
		return True
		
	def get_precompiled_header_info( self ) :
		return self.precompiled_header_info
		
	def get_conversion_dictionary( self ):
		ret = super(core_c_sharp_bind_options, self).get_conversion_dictionary( )
		for k in conversion_dict.keys() :
			ret[k] = conversion_dict[k]
		return ret
		
	def get_standard_source_includes(self, type_instance):
		ret =  super(core_c_sharp_bind_options, self).get_standard_source_includes( type_instance )
		ret.extend( [ self.precompiled_header_info[1] ] )
		return ret
		
	def get_wrapper_prologue(self):
		return "WRAPPER_PROLOGUE {"
	
	def get_wrapper_epilogue(self):
		return "} WRAPPER_EPILOGUE();"
	