# (C) Ardendo SE 2008
#
# Released under the LGPL

import glob
import os
import re
import utils
import shutil

class WinConfig :

	def __init__( self ):
		self.compiles = {}

	def walk( self, env ):
		"""Walk the bcomp directory to pick out all the .wc files"""
		# Start by copying all the .wc files found in winconfig to bcomp
		for r, d, f in os.walk( 'winconfig' ):
			for file in f:
				full = os.path.join( r, file )
				target = r.replace( 'winconfig' + os.sep, '' )
				if full.find( os.sep + '.' ) == -1:
					if not os.path.exists( target ):
						os.makedirs( target )
					shutil.copy( full, target )

		flags = { }
		shared = os.path.join( __file__.rsplit( '/', 1 )[ 0 ], 'pkgconfig' )
		for repo in [ os.path.join( 'pkgconfig', 'win32' ), os.path.join( 'bcomp', 'common' ), os.path.join( 'bcomp', env[ 'target' ] ) ]  :
			for r, d, files in os.walk( os.path.join( env.root, repo ) ):
				for f in files:
					if f.endswith( '.wc' ):
						pkg = f.replace( '.wc', '' )
						prefix = r.rsplit( '\\', 2 )[ 0 ]
						flags[ pkg ] = { 'prefix': prefix, 'file': os.path.join( r, f ) }
		return flags
	
	def requires( self, env, *packages ) :		
		""" Will load small snipptes of python-code stored in .wc files.
			These snippets assume there are a couple of variables available
			in the current scope: env, prefix . env is the env variable passed
			to this function, prefix is set before the code is executed. 
			
			The snippets can call env.requires( ... ) which will call this 
			function again recursively.
			
			To make sure that we don't call the same snippet twice, we
			keep a list of checked packages and just pass through the ones
			already called. """
			
		for package in packages :
			if package in env.checked : continue
			env.checked.append( package )
			if package not in env.package_list.keys( ) :
				raise Exception, 'Unable to locate %s' % package
			
			# This can be used in the wc-file code, its the path to the file itself.
			prefix = env.package_list[ package ][ 'prefix' ]	
			wcfile = env.package_list[ package ][ 'file' ]

			if wcfile not in self.compiles.keys( ):
				wcf = open(wcfile, "r")
				wcfdata = wcf.read().replace("\r\n", "\n")
				self.compiles[ wcfile ] = compile(wcfdata, wcfile, "exec")

			code = self.compiles[ wcfile ]
			exec( code )
			

	def packages( self, env, *packages ):
		"""Add packages to the environment"""
		env.checked = []
		env.requires( *packages )
		
	def optional( self, env, *packages ):
		"""Extracts compile and link flags for the specified packages and adds to the 
		current environment along with a have_package variable to allow dependency checks."""
		result = {}
		for package in packages:
			try:
				env.packages( package )
				result[ 'have_' + package ] = 1
			except Exception, e:
				result[ 'have_' + package ] = 0
		return result

	def package_cflags( self, env, *packages ):
		"""Obtains the CFlags in the .wc file"""
		flags = ''
		checked = []
		for package in packages:
			rules = self.obtain_rules( env, package, checked )
			if 'CFlags' in rules.keys( ):
				flags += rules[ 'CFlags' ] + ' '
		return flags

	def package_libs( self, env, *packages ):
		"""Obtains the Libs in the .wc file"""
		flags = ''
		checked = []
		for package in packages:
			rules = self.obtain_rules( env, package, checked )
			if 'CFlags' in rules.keys( ):
				flags += rules[ 'Libs' ] + ' '
		return flags

	def package_install_include( self, env ):
		result = []
		checked = []
		for package in env.package_list.keys( ):
			if env.package_list[ package ][ 'prefix' ].startswith( os.path.join( env.root, 'bcomp' ) ):
				rules = self.obtain_rules( env, package, checked )
				if 'install_include' in rules.keys( ):
					include = rules[ 'install_include' ]
					result += glob.glob( include )
		return result

	def package_install_libs( self, env ):
		result = []
		checked = []
		for package in env.package_list.keys( ):
			if env.package_list[ package ][ 'prefix' ].startswith( os.path.join( env.root, 'bcomp' ) ):
				rules = self.obtain_rules( env, package, checked )
				if 'install_libs' in rules.keys( ):
					libs = rules[ 'install_libs' ]
					result += glob.glob( libs )
		return result

