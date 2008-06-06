# (C) Ardendo SE 2008
#
# Released under the LGPL

import env
import os

def build( self, path, deps = [] ):
	"""Invokes a SConscript, cloning the environment and linking against any inter
	project dependencies specified."""
	result = { }
	for build_type in [ env.Environment.prep_release, env.Environment.prep_debug ]:
		local_env = self.Clone( )
		build_type( local_env )
		if self[ 'PLATFORM' ] != 'posix':
			for dep in deps:
				local_env.Append( LIBS = dep[ build_type ] )
		result[ build_type ] = local_env.SConscript( [ os.path.join( path, 'SConscript' ) ], build_dir=os.path.join( local_env[ 'stage_prefix' ], path ), duplicate=0, exports=[ 'local_env' ] )
	return result

def shared_library( self, lib, sources, headers=None, pre=None, nopre=None, **keywords ):
	"""Build a shared library ( dll or so )
		
		Keyword arguments:
		lib -- The name of the library to build
		sources -- A list of c++ source files
		headers -- A list of c++ header files, not needed for the build, but could be used 
					by certain build_managers that create IDE files.
		pre -- A tuple with two elements, the first is the name of the header file that is 
				included in all source-files using precompiled headers, the second is the
				name of the actual source file creating the precompiled header file itself.
		nopre -- If pre is set, this parameter contains a list of source files (that should
					be present in sources) that will not use precompiled headers. These files
					do not include the header file in pre[0].
		keywords -- All other parameters passed with keyword assignment.
	"""
		
	if self[ 'PLATFORM' ] == 'darwin':
		self.Append( LINKFLAGS = [ '-Wl,-install_name', '-Wl,%s/lib%s.dylib' % ( self[ 'install_name' ], lib ) ] )
	return self.SharedLibrary( lib, sources, **keywords )

def plugin( self, lib, sources, headers=None, pre=None, nopre=None, **keywords ):
	"""Build a plugin
	
		Keyword arguments:
		lib -- The name of the library to build
		sources -- A list of c++ source files
		headers -- A list of c++ header files, not needed for the build, but could be used 
					by certain build_managers that create IDE files.
		pre -- A tuple with two elements, the first is the name of the header file that is 
				included in all source-files using precompiled headers, the second is the
				name of the actual source file creating the precompiled header file itself.
		nopre -- If pre is set, this parameter contains a list of source files (that should
					be present in sources) that will not use precompiled headers. These files
					do not include the header file in pre[0].
		keywords -- All other parameters passed with keyword assignment."""

	return self.SharedLibrary( lib, sources, **keywords )


