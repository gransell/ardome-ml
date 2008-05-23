import os
import sys
from SCons.Script.SConscript import SConsEnvironment as BaseEnvironment
import SCons.Script
import utils
import vsbuild

if os.name == 'posix':
	import pkgconfig as package_manager
else:
	import winconfig as package_manager

class Environment( BaseEnvironment ):

	packages = package_manager.packages
	package_cflags = package_manager.package_cflags
	package_libs = package_manager.package_libs

	"""Base environment for Ardendo AML/AMF and related builds."""

	def __init__( self, opts, bmgr = None , *kw ):
		"""	Constructor. The Options object is added to the environment, and all 
			common install and build related variables are defined as needed.
			
			Keyword arguments:
			opts -- 
			bmrg --  A possible build manager. The build manager can override functions
					in this class if required. The functions are build, shared_library 
					and plugin.
			kw -- The rest of the passed parameters to this constructor.
			"""

		BaseEnvironment.__init__( self, *kw )

		opts.Update( self )
		opts.Save( opts.file, self )
		
		if bmgr : self.build_manager = bmgr
		elif utils.vs() : self.build_manager = vsbuild.VsBuilder( utils.vs() )
		else: self.build_manager = None 
	
		SCons.Script.Help( opts.GenerateHelpText( self ) )

		self[ 'debug_prefix' ] = os.path.join( 'build', 'debug' )
		self[ 'release_prefix' ] = os.path.join( 'build', 'release' )
		self[ 'libdir' ] = [ 'lib', 'lib64' ][ utils.arch( ) == 'x86_64' ]
		self[ 'stage_include' ] = os.path.join( '$stage_prefix', 'include' )
		self[ 'stage_libdir' ] = os.path.join( '$stage_prefix', '$libdir' )
		self[ 'stage_pkgconfig' ] = os.path.join( '$stage_libdir', 'pkgconfig' )

		self.root = os.path.abspath( '.' )
		package_manager.walk( self )

		self.Alias( 'install', '$release_prefix' )
		self.Alias( 'debug-install', '$debug_prefix' )

	def prep_debug( self ):
		self[ 'debug' ] = '0'
		self[ 'build_prefix' ] = os.path.join( 'build', 'debug' )
		self[ 'stage_prefix' ] = utils.install_target( self )

		if self[ 'PLATFORM' ] == 'posix':
			self.Append( CCFLAGS = [ '-Wall', '-ggdb', '-O0' ] )
		elif self[ 'PLATFORM' ] == 'darwin':
			self.Append( CCFLAGS = [ '-Wall', '-gdwarf-2', '-O0' ] )
		elif self[ 'PLATFORM' ] == 'win32':
			self.Append( CCFLAGS = [ '/W3' ] )
		else:
			raise( 'Unknown platform: %s', self[ 'PLATFORM' ] )

	def prep_release( self ):
		self[ 'debug' ] = '1'
		self[ 'build_prefix' ] = os.path.join( 'build', 'release' )
		self[ 'libdir' ] = [ 'lib', 'lib64' ][ utils.arch( ) == 'x86_64' ]
		self[ 'stage_prefix' ] = utils.install_target( self )

		if self[ 'PLATFORM' ] == 'posix':
			self.Append( CCFLAGS = [ '-Wall', '-O3', '-pipe' ] )
		elif self[ 'PLATFORM' ] == 'darwin':
			self.Append( CCFLAGS = [ '-Wall', '-O3', '-pipe' ] )
		elif self[ 'PLATFORM' ] == 'win32':
			self.Append( CCFLAGS = [ '/W3', '/O2', '/EHsc' ] )
		else:
			raise( 'Unknown platform: %s', self[ 'PLATFORM' ] )

	def check_dependencies( self, *packages ):
		"""Ensure that all the listed packages are found."""
		result = True
		temp = self.Clone( )
		temp.prep_release( )
		for package in packages:
			try:
				temp.packages( package )
			except OSError, e:
				result = False
				print "Dependency check" + str( e )
		return result
		
	def build( self, path, deps = [] ):
		"""	Invokes a SConscript, cloning the environment and linking against any inter
			project dependencies specified."""
			
		if "build" in dir(self.build_manager) : 
			return self.build_manager.build( self, path, deps )
			
		result = { }
		for build_type in [ Environment.prep_release, Environment.prep_debug ]:
			local_env = self.Clone( )
			build_type( local_env )
			if self[ 'PLATFORM' ] != 'posix':
				for dep in deps:
					local_env.Append( LIBS = dep[ build_type ] )
			result[ build_type ] = local_env.SConscript( [ os.path.join( path, 'SConscript' ) ], 
															build_dir=os.path.join( local_env[ 'stage_prefix' ], path ), 
															duplicate=0, exports=[ 'local_env' ] )
		return result

	def shared_library( self, lib, sources, headers=None, pre=None, nopre=None, *keywords ):
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
	
		if "shared_library" in dir(self.build_manager) : 
			return self.build_manager.shared_library( self, lib, sources, headers, pre, nopre, keywords )
		
		if self[ 'PLATFORM' ] == 'darwin':
			self.Append( LINKFLAGS = [ '-Wl,-install_name', '-Wl,%s/lib%s.dylib' % ( self[ 'install_name' ], lib ) ] )
		return self.SharedLibrary( lib, sources, keywords )
		
	def plugin( self, lib, sources, headers=None, pre=None, nopre=None, *keywords ):
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
			
		if "plugin" in dir(self.build_manager) : 
			return self.build_manager.plugin( self, lib, sources, headers, pre, nopre, keywords )
		
		return self.SharedLibrary( lib, sources, keywords )
	


