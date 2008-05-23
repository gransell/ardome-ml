import os
import sys
from SCons.Script.SConscript import SConsEnvironment as BaseEnvironment
import SCons.Script
import utils

if os.name == 'posix':
	import pkgconfig as package_manager
else:
	import winconfig as package_manager

class Environment( BaseEnvironment ):

	packages = package_manager.packages
	package_cflags = package_manager.package_cflags
	package_libs = package_manager.package_libs

	"""Base environment for Ardendo AML/AMF and related builds."""

	def __init__( self, opts, *kw ):
		"""Constructor. The Options object is added to the environment, and all 
		common install and build related variables are defined as needed."""

		BaseEnvironment.__init__( self, *kw )

		opts.Update( self )
		opts.Save( opts.file, self )

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
			self.Append( CCFLAGS = [ '/W3', '/O0' ] )
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

	def build( self, path, deps = [] ):
		"""Invokes a SConscript, cloning the environment and linking against any inter
		project dependencies specified."""
		result = { }
		for build_type in [ Environment.prep_release, Environment.prep_debug ]:
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

	def check_dependencies( self, *packages ):
		"""Ensure that all the listed packages are found."""
		result = True
		temp = self.Clone( )
		temp.prep_release( )
		for package in packages:
			try:
				temp.packages( *packages )
			except OSError, e:
				result = False
				print "Dependency check" + str( e )
		return result

