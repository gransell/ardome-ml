
import owl.wrapper.objc.objc_bind_options


conversion_dict = {'olib::opencorelib::frames' : 'NSUInteger',
					'olib::opencorelib::rational_time' : 'OlibRationalTime',
					'boost::rational<long>' : 'OlibRationalTime',
					'olib::opencorelib::point' : 'NSPoint' }

conversion_method_dict = {'olib::opencorelib::frames' : 'ConvertFrames',
						  'boost::rational<int64_t>' : 'ConvertRationalTime',
						  'olib::opencorelib::point' : 'ConvertPoint',
						  'boost::filesystem::path' : 'ConvertPath',
						  'boost::posix_time::time_duration' : 'ConvertTimeDuration',}

non_objet_types = ['olib::opencorelib::point', 'olib::opencorelib::frames', ]

ns_abreviation_dict = { 'olib::opencorelib' : 'olib', }


class core_objc_bind_options( owl.wrapper.objc.objc_bind_options ):
	"""docstring for core_objc_bind_options"""
	def __init__( self, owl_path, output_path, assemblies ):
		super(core_objc_bind_options, self).__init__( owl_path, output_path, assemblies )
	
	def get_standard_header_includes(self, type_instance):
		"""docstring for get_standard_header_includes"""
		ret = super(core_objc_bind_options, self).get_standard_header_includes( type_instance )
		ret.append('"OlibError.h"')
		return ret
	
	def get_standard_objc_source_includes(self, type_instance):
		"""docstring for get_standard_objc_source_includes"""
		ret = super(core_objc_bind_options, self).get_standard_objc_source_includes()
		ret.extend(['"CoreConverts.h"', '"OlibErrorPrivate.h"', ])
		return ret

	def get_standard_source_files( self ):
		"""docstring for get_standard_source_files"""
		ret = super(core_objc_bind_options, self).get_standard_source_files()
		ret.extend(['objective_c/src/CoreConverts.mm', 
					'objective_c/src/OlibRationalTime.m',
					'objective_c/src/OlibError.mm', ])
		return ret
	
	def get_public_objc_headers( self ):
		"""
		Return a list of public headers that should be included in the resulting framework.
		"""
		ret = super(core_objc_bind_options, self).get_public_objc_headers()
		ret.extend(['objective_c/src/AmfInitializer.h', 
					'objective_c/src/AmfAdditions.h',
					'objective_c/src/OlibRationalTime.h',
					'objective_c/src/OlibError.h',])
		return ret
		
	def get_conversion_dictionary( self ):
		ret = super(core_objc_bind_options, self).get_conversion_dictionary( )
		for k in conversion_dict.keys() :
			ret[k] = conversion_dict[k]
		return ret
	
	def get_conversion_method_dictionary( self ):
		ret = super(core_objc_bind_options, self).get_conversion_method_dictionary( )
		for k in conversion_method_dict.keys() :
			ret[k] = conversion_method_dict[k]
		return ret
	
	def is_wrappable_type( self, type_instance ):
		# Ugly hack
		if type_instance.get_root_type_instance().get_name() == 'amf::the_initializer' :
			return False
		if type_instance.get_root_type_instance().get_name() == 'amf::initializer' :
			return False
		return super( core_objc_bind_options, self ).is_wrappable_type( type_instance )
	
	def is_object_type( self, type_instance ):
		if type_instance.get_root_type_instance().get_name() in non_objet_types : return False
		return super(core_objc_bind_options, self).is_object_type( type_instance )
	
	def get_name_abreviation_dict(self):
		return ns_abreviation_dict
	
	def to_wrapped_type_name( self, type_instance ) :
		"""
		Overload to suply a dictionary for abreviations
		"""
		return super( core_objc_bind_options, self).to_wrapped_type_name_abreviate( type_instance, self.get_name_abreviation_dict() )
		
	def generate_function_parameter_list( self, method ):
		ret = super( core_objc_bind_options, self ).generate_function_parameter_list( method )
		if ret != '' :
			ret += ', '
		ret += 'OlibError **outError '
		return ret
	
	def generate_forward_decls( self, method_list, exclude_types ):
		"""docstring for generate_forward_decls"""
		fwd_decls = super( core_objc_bind_options, self ).generate_forward_decls( method_list, exclude_types )
		
		for k in fwd_decls.keys() :
			if not self.is_object_type( fwd_decls[k] ) :
				del fwd_decls[k]
		
		return fwd_decls
	
	def get_native_type_includes(self, type_instance):
		ret = super(core_objc_bind_options, self).get_native_type_includes(type_instance)
		return [h for h in ret if not h.endswith('built_in_types.hpp')]
		
	def get_standard_source_includes(self, type_instance):
		"""docstring for get_standard_source_includes"""
		ret =  super(core_objc_bind_options, self).get_standard_source_includes( type_instance )
		ret.extend( ['"opencorelib/cl/core.hpp"', 
					'"opencorelib/cl/base_exception.hpp"',
					'"opencorelib/cl/enforce_defines.hpp"',
					'"opencorelib/cl/log_defines.hpp"', ] )
		return ret
		
	def get_wrapper_epilogue(self, method = None):
		if method is not None and method.get_declaring_type() is None :
			return """}
	catch( const olib::opencorelib::base_exception& be ) {
		if( outError ) *outError = [[[OlibError alloc] initWithBaseException:be] autorelease];
	}
	catch( const std::exception& e ) {
		if( outError ) *outError = [[[OlibError alloc] initWithStdException:e] autorelease];
	}
	catch( ... ) {
		if( outError ) *outError = [[[OlibError alloc] init] autorelease];
	}"""

		return """}
	catch( const olib::opencorelib::base_exception& be ) {
		NSDictionary *userInfo = [NSDictionary dictionaryWithObjectsAndKeys:ConvertString(be.what()),
			@"what", 
			ConvertString(be.context().message()),
			@"message",
			ConvertString(be.context().source()),
			@"source",
			ConvertString(be.context().filename()),
			@"filename",
			ConvertString(be.context().target_site()),
			@"function",
			ConvertString(be.context().expression()),
			@"expression",
			nil];
		[[NSException exceptionWithName:@"AmfException" 
								 reason:ConvertString(be.context().error_reason())
							   userInfo:userInfo] raise];
	}
	catch( const std::exception& e ) {
		[[NSException exceptionWithName:@"StdException" 
								 reason:ConvertString(e.what()) 
							   userInfo:nil] raise];
	}
	catch( const std::string& s ) {
		[[NSException exceptionWithName:@"StringException" 
								 reason:ConvertString(s.c_str()) 
							   userInfo:nil] raise];
	}
	catch( ... ) {
		[[NSException exceptionWithName:@"UnknownException" 
								 reason:@"Unknown" 
							   userInfo:nil] raise];
	}"""
	
