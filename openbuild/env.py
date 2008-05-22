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

		self.root = os.path.abspath( '.' )
		package_manager.walk( self )

		self[ 'build_prefix' ] = os.path.join( 'build', [ 'release', 'debug' ][ int( self[ 'debug' ] ) ] )
		self[ 'libdir' ] = [ 'lib', 'lib64' ][ utils.arch( ) == 'x86_64' ]
		self[ 'stage_prefix' ] = utils.install_target( self )
		self[ 'stage_include' ] = os.path.join( self[ 'stage_prefix' ], 'include' )
		self[ 'stage_libdir' ] = os.path.join( self[ 'stage_prefix' ], self[ 'libdir' ] )
		self[ 'stage_pkgconfig' ] = os.path.join( self[ 'stage_libdir' ], 'pkgconfig' )

		if self[ 'PLATFORM' ] == 'posix' and self[ 'debug' ] == '0':
			self.Append( CCFLAGS = [ '-Wall', '-O3', '-pipe' ] )
		elif self[ 'PLATFORM' ] == 'posix':
			self.Append( CCFLAGS = [ '-Wall', '-ggdb', '-O0' ] )
		elif self[ 'PLATFORM' ] == 'darwin' and self[ 'debug' ] == '0':
			self.Append( CCFLAGS = [ '-Wall', '-O3', '-pipe' ] )
		elif self[ 'PLATFORM' ] == 'darwin':
			self.Append( CCFLAGS = [ '-Wall', '-gdwarf-2', '-O0' ] )
		elif self[ 'PLATFORM' ] == 'win32' and self[ 'debug' ] == '0':
			self.Append( CCFLAGS = [ '/W3', '/O2', '/EHsc' ] )
		elif self[ 'PLATFORM' ] == 'win32':
			self.Append( CCFLAGS = [ '/W3', '/O0' ] )
		else:
			raise( 'Unknown platform: %s', self[ 'PLATFORM' ] )

		self.Alias( 'install', self[ 'stage_prefix' ] )

	def build( self, path, deps = [] ):
		"""Invokes a SConscript, cloning the environment and linking against any inter
		project dependencies specified."""
		local_env = self.Clone( )
		if self[ 'PLATFORM' ] != 'posix':
			local_env.Append( LIBS = deps )
		return local_env.SConscript( [ os.path.join( path, 'SConscript' ) ], exports=[ 'local_env' ] )

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
		for package in packages:
			try:
				temp.packages( *packages )
			except OSError, e:
				result = False
				print "Dependency check" + str( e )
		return result

