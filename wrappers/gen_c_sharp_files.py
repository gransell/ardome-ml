import os
import sys

# Make sure we can access owl, located in root/external/
sys.path.append( os.path.join( os.getcwd( ), 'external' ) )
sys.path.append( 'C:\\cygwin\\home\\mats\\bzr\\ardome-ml\\owl_additions\\external' ) 

from owl.reflection import header_file, header_files, assembly
import owl.wrapper.c_sharp
from csharp import core_c_sharp_bind_options

print 'Creating dependent assembly with built-in types'

script_location = os.path.dirname( os.path.abspath(__file__) )
project_root = os.path.normpath( os.path.join( script_location, '..' ) )
owl_output_dir = os.path.join( script_location, 'owl_generated_files', 'c_sharp', 'src' )

print ('project_root', project_root)

built_in_types_file = os.path.join( script_location , 'built_in_types.hpp' )
print built_in_types_file

dependent_headers = [ built_in_types_file ]
dependent_header_files = []
for dh in dependent_headers :
	dependent_header_files.append(header_file(str(dh)))


	
dependent_assemblies = [ assembly( 'dep_ass', header_files( dependent_header_files, [], [] ), [] ) ]	

cl_src_dir = os.path.join( project_root, 'src', 'opencorelib', 'cl' )

headers = [ header_file( os.path.join( project_root , 'wrappers', 'bind_typedefs.hpp' ) ),
			header_file( os.path.join( cl_src_dir, 'core_enums.hpp') ),
			header_file( os.path.join( cl_src_dir, 'basic_enums.hpp') ),
			header_file( os.path.join( cl_src_dir, 'minimal_string_defines.hpp') ),
			header_file( os.path.join( cl_src_dir, 'media_definitions.hpp') ),
			header_file( os.path.join( cl_src_dir, 'time_code.hpp') ),
			header_file( os.path.join( cl_src_dir, 'media_time.hpp') ),
			header_file( os.path.join( cl_src_dir, 'frames.hpp') ),
			header_file( os.path.join( cl_src_dir, 'span.hpp') ),
			header_file( os.path.join( cl_src_dir, 'point.hpp') ),
			header_file( os.path.join( cl_src_dir, 'assert.hpp') ),
			header_file( os.path.join( cl_src_dir, 'color.hpp') ),
			header_file( os.path.join( cl_src_dir, 'enforce.hpp') ),
			header_file( os.path.join( cl_src_dir, 'exception_context.hpp') ),
			header_file( os.path.join( cl_src_dir, 'invoker.hpp') ),
			header_file( os.path.join( cl_src_dir, 'ip_address.hpp') ),
			header_file( os.path.join( cl_src_dir, 'worker.hpp') ),
			header_file( os.path.join( cl_src_dir, 'jobbase.hpp') ),
			header_file( os.path.join( cl_src_dir, 'event_handler.hpp') ),
			header_file( os.path.join( cl_src_dir, 'base_exception.hpp') ),	
			header_file( os.path.join( cl_src_dir, 'logger.hpp') ) ,
			header_file( os.path.join( cl_src_dir, 'object.hpp') ) ,
			header_file( os.path.join( cl_src_dir, 'logtarget.hpp') ) ,
			header_file( os.path.join( cl_src_dir, 'point.hpp') ) ,
			header_file( os.path.join( cl_src_dir, 'rectangle.hpp') ),
			header_file( os.path.join( cl_src_dir, 'size.hpp') ),
			header_file( os.path.join( cl_src_dir, 'special_folders.hpp') ),
			header_file( os.path.join( cl_src_dir, 'uuid_16b.hpp') ) ,
			header_file( os.path.join( cl_src_dir, 'base_exception.hpp') ),
			header_file( os.path.join( cl_src_dir, 'template.hpp') ) ]

			
defines = [('CORE_API', ''), ]
defines.extend([('OLIB_USE_UTF16', '1'), ('TCHAR', 'wchar_t'), ])
	
files = header_files( headers, defines, [] )

core_assembly = assembly( 'core', files, dependent_assemblies, stop_on_exceptions = True )

assembly_xml_file = os.path.join( script_location, 'assembly.xml')
print ('Dumping Abstract Syntax Tree to: %s' % assembly_xml_file )
fd = open(  assembly_xml_file , "w" )
fd.write( core_assembly.xml_str() )


csharp_wrapper = owl.wrapper.c_sharp.c_sharp()
csharp_options = core_c_sharp_bind_options( os.path.join( project_root, 'external' ), owl_output_dir, [core_assembly] )

wrapper_files = csharp_wrapper.bind( core_assembly, csharp_options )

print 'Done generating files'


