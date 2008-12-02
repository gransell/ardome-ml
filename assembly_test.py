import owl.wrapper.c_sharp
import owl.wrapper.objc
from owl.reflection import header_file, header_files, assembly
from owl.utility import dump_source

import os

dep_asses = []
dep_headers = [ os.path.join( os.getcwd(), 'wrappers', 'built_in_types.hpp' ) ]
dep_hfs = []
for dh in dep_headers :
	dep_hfs.append(header_file(str(dh)))
dep_asses.append(assembly( 'dep_ass', header_files( dep_hfs, [], [] ), [] ))

defines = [('CORE_API', ''), ]
defines.extend([('OLIB_USE_UTF8', '1'), ('TCHAR', 'char'), ])

cl_src_dir =  os.path.join( os.getcwd(), 'src', 'opencorelib', 'cl' )

headers = [ header_file( os.path.join( os.getcwd(), 'wrappers', 'bind_typedefs.hpp' ) ),
			header_file( os.path.join( cl_src_dir, 'minimal_string_defines.hpp') ),
			header_file( os.path.join( cl_src_dir, 'media_definitions.hpp') ),
			header_file( os.path.join( cl_src_dir, 'time_code.hpp') ),]

files = header_files( headers, defines, [] )

sample_assembly = assembly( 'core', files, dep_asses )

fd = open( 'assembly.xml', "w" )
fd.write( sample_assembly.xml_str() )

