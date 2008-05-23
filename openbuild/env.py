import os
import sys
from SCons.Script.SConscript import SConsEnvironment as BaseEnvironment
import SCons.Script
import utils

if os.name == 'posix':
	import pkgconfig as package_manager
else:
	import winconfig as package_manager

if 'vs=1' in sys.argv:
	import vsbuild as build_manager
else:
	import sysbuild as build_manager

class Environment( BaseEnvironment ):

	packages = package_manager.packages
	package_cflags = package_manager.package_cflags
	package_libs = package_manager.package_libs

	build = build_manager.build
	shared_library = build_manager.shared_library
	plugin = build_manager.plugin

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

