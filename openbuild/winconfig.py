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
		self.install_debug = {}
		self.install_release = {}

	def walk( self, env ):
		"""Walk the bcomp directory to pick out all the .wc files"""
		flags = { }
		shared = os.path.join( __file__.rsplit( '/', 1 )[ 0 ], 'pkgconfig' )
		for repo in [ os.path.join( 'pkgconfig', 'win32' ), os.path.join( 'bcomp' ) ]  :
			for r, d, files in os.walk( os.path.join( env.root, repo ) ):
				for f in files:
					if f.endswith( '.wc' ):
						pkg = f.replace( '.wc', '' )
						prefix = r.rsplit( '\\', 2 )[ 0 ]
						type = prefix.split( os.sep )[-1]
						if type in [ 'debug', 'release'] : pkg += '_' + type
						flags[ pkg ] = { 'prefix': prefix, 'file': os.path.join( r, f ) }
		return flags
		
	def locate_package( self, env, package_name ) :
		# return prefix and file ...
		if env.debug and env.package_list.has_key( package_name + "_debug"):
			return env.package_list[ package_name + "_debug"]
		elif env.package_list.has_key( package_name + "_release") : 
			return env.package_list[ package_name + "_release"]
		elif env.package_list.has_key( package_name ) :
			return env.package_list[ package_name]
		else : return None
	
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
			
			pkg = self.locate_package(env, package)
			if pkg == None :
				raise Exception, 'Unable to locate %s' % package
			
			# This can be used in the wc-file code, its the path to the file itself.
			prefix = pkg[ 'prefix' ]	
			wcfile = pkg[ 'file' ]

			if wcfile not in self.compiles.keys( ):
				wcf = open(wcfile, "r")
				wcfdata = wcf.read().replace("\r\n", "\n")
				code = compile(wcfdata, wcfile, "exec")
				namespace = { }
				exec( code ) in namespace
				self.compiles[ wcfile ] = namespace

			# Retrieve the derived namespace for the package
			namespace = self.compiles[ wcfile ]

			# Handle the install method on first use
			if 'install' in namespace.keys( ):
				if env.debug and wcfile not in self.install_debug.keys( ):
					self.install_debug[ wcfile ] = True
					namespace[ 'install' ]( env, prefix )
				if not env.debug and wcfile not in self.install_release.keys( ):
					self.install_release[ wcfile ] = True
					namespace[ 'install' ]( env, prefix )

			# Call the flags method
			if 'flags' in namespace.keys( ):
				namespace[ 'flags' ]( env, prefix )
			
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

