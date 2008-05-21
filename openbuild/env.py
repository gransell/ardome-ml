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
		"""Constructor"""
		BaseEnvironment.__init__( self, *kw )

		opts.Update( self )
		opts.Save( opts.file, self )

		SCons.Script.Help( opts.GenerateHelpText( self ) )

		self.root = os.path.abspath( '.' )
		self.package_list = package_manager.walk( self )

		self[ 'build_prefix' ] = os.path.join( 'build', [ 'release', 'debug' ][ int( self[ 'debug' ] ) ] )
		self[ 'libdir' ] = [ 'lib', 'lib64' ][ utils.arch( ) == 'x86_64' ]
		self[ 'stage_prefix' ] = utils.install_target( self )
		self[ 'stage_include' ] = os.path.join( self[ 'stage_prefix' ], 'include' )
		self[ 'stage_libdir' ] = os.path.join( self[ 'stage_prefix' ], self[ 'libdir' ] )
		self[ 'stage_pkgconfig' ] = os.path.join( self[ 'stage_libdir' ], 'pkgconfig' )

		self.Alias( "install", self[ 'stage_prefix' ] )

	def build( self, path, deps = [] ):
		"""Invokes a SConscript, cloning the environment and linking against any inter
		project dependencies specified."""
		local_env = self.Clone( )
		local_env.Append( LIBS = deps )
		return local_env.SConscript( [ os.path.join( path, 'SConscript' ) ], exports=[ 'local_env' ] )

	def check_dependencies( self, *packages ):
		"""Ensure that all the listed packages are found."""
		result = True
		try:
			self.Clone( ).packages( *packages )
		except OSError, e:
			result = False
			print "Dependency check" + str( e )
		return result

