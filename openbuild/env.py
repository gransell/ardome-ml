import os
from SCons.Script.SConscript import SConsEnvironment as BaseEnvironment

if os.name == 'posix':
	import pkgconfig as package_manager
else:
	import winconfig as package_manager

class Environment( BaseEnvironment ):

	packages = package_manager.packages
	package_cflags = package_manager.package_cflags
	package_libs = package_manager.package_libs

	"""Base environment for Ardendo AML/AMF and related builds."""

	def __init__( self, *kw ):
		"""Constructor"""
		BaseEnvironment.__init__( self, *kw )
		self.root = os.path.abspath( '.' )
		self.package_list = package_manager.walk( self )

