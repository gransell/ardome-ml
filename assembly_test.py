import owl.wrapper.c_sharp
import owl.wrapper.objc
from owl.reflection import header_file, header_files, assembly
from owl.utility import dump_source

import os

dep_asses = []
dep_headers = [ os.path.join( '/Users/gransell/Documents/src/ardome-ml/owl_additions', 'wrappers', 'built_in_types.hpp' ) ]
dep_hfs = []
for dh in dep_headers :
	dep_hfs.append(header_file(str(dh)))
dep_asses.append(assembly( 'dep_ass', header_files( dep_hfs, [], [] ), [] ))

defines = [('CORE_API', ''), ]
defines.extend([('OLIB_USE_UTF8', '1'), ('TCHAR', 'char'), ])

headers = [ header_file( os.path.join( '/Users/gransell/Documents/src/ardome-ml/owl_additions', 'src', 'opencorelib', 'cl', 'typedefs.hpp') ),
			header_file( os.path.join( '/Users/gransell/Documents/src/ardome-ml/owl_additions', 'src', 'opencorelib', 'cl', 'minimal_string_defines.hpp') ),
			header_file( os.path.join( '/Users/gransell/Documents/src/ardome-ml/owl_additions', 'src', 'opencorelib', 'cl', 'media_definitions.hpp') ),
			header_file( os.path.join( '/Users/gransell/Documents/src/ardome-ml/owl_additions', 'src', 'opencorelib', 'cl', 'time_code.hpp') ),
			header_file( os.path.join( '/Users/gransell/Documents/src/ardome-ml/owl_additions', 'src', 'opencorelib', 'cl', 'media_time.hpp') ),
			header_file( os.path.join( '/Users/gransell/Documents/src/ardome-ml/owl_additions', 'src', 'opencorelib', 'cl', 'span.hpp') ),
			header_file( os.path.join( '/Users/gransell/Documents/src/ardome-ml/owl_additions', 'src', 'opencorelib', 'cl', 'frames.hpp') ),]

files = header_files( headers, defines, [] )

sample_assembly = assembly( 'core', files, dep_asses )

fd = open( 'assembly.xml', "w" )
fd.write( sample_assembly.xml_str() )

