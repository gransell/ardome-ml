import env
import os

def build( self, path, deps = {} ):
	"""Invokes a SConscript, cloning the environment and linking against any inter
	project dependencies specified."""
	result = { }
	for build_type in [ env.Environment.prep_release, env.Environment.prep_debug ]:
		local_env = self.Clone( )
		build_type( local_env )
		if self[ 'PLATFORM' ] != 'posix':
			local_env.Append( LIBS = deps[ build_type ] )
		result[ build_type ] = local_env.SConscript( [ os.path.join( path, 'SConscript' ) ], build_dir=os.path.join( local_env[ 'stage_prefix' ], path ), duplicate=0, exports=[ 'local_env' ] )
	return result

def shared_library( self, lib, sources, *kw ):
	"""Build the shared library"""
	if self[ 'PLATFORM' ] == 'darwin':
		self.Append( LINKFLAGS = [ '-Wl,-install_name', '-Wl,%s/lib%s.dylib' % ( self[ 'install_name' ], lib ) ] )
	return self.SharedLibrary( lib, sources, *kw )

def plugin( self, lib, sources, *kw ):
	"""Build the plugin"""
	return self.SharedLibrary( lib, sources, *kw )


