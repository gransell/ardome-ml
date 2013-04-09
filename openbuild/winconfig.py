# (C) Ardendo SE 2008
#
# Released under the LGPL

import glob
import os
import re
import utils
import shutil

class PackageNotFoundException :
	def __init__( self, package_name ):
		self.package_name = package_name

class WinConfig :

	def __init__( self ):
		self.compiles = {}
		self.install_debug = {}
		self.install_release = {}

	def walk( self, env ):
		"""Walk the bcomp directory to pick out all the .wc files"""
		flags = { }
		shared = os.path.join( __file__.rsplit( '/', 1 )[ 0 ], 'pkgconfig' )

		self.find_wc_file( env, flags, os.path.join( 'pkgconfig', 'win32' ), None )

		#Only accept .wc files in winconfig directories when searching in bcomp/
		self.find_wc_file( env, flags, os.path.join( 'bcomp' ), 'winconfig' )

		return flags
	
	def find_wc_file( self, env, flags, dir_to_search, required_parent_dir ):
		for r, d, files in os.walk( os.path.join( env.root, dir_to_search ) ):
			if required_parent_dir is None or os.path.split(r)[-1] == required_parent_dir:
				for f in files:
					if f.endswith( '.wc' ):
						pkg = f.replace( '.wc', '' )
						prefix = r.rsplit( '\\', 2 )[ 0 ]
						type = prefix.split( os.sep )[-1]
						if type in [ 'debug', 'release'] : pkg += '_' + type
						if pkg in flags:
							raise Exception, 'Found two different winconfig files for package %s:\n%s\nand:\n%s' % \
								( pkg, os.path.join( flags[ pkg ][ 'file' ] ), os.path.join( r, f ) )
						flags[ pkg ] = { 'prefix': prefix, 'file': os.path.join( r, f ) }
		
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
				raise PackageNotFoundException('Unable to locate ' + package)
			
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
			
	def packages( self, env, *packages, **kwargs ):
		"""Add packages to the environment"""
		env.checked = []
		try:
			env.requires( *packages )
		except PackageNotFoundException, exc:
			if kwargs.get( 'optional', False ) == False:
				print "Exception when locating package:", exc.package_name
			raise exc
		
	def optional( self, env, *packages ):
		"""Extracts compile and link flags for the specified packages and adds to the 
		current environment along with a have_package variable to allow dependency checks."""
		result = {}
		for package in packages:
			try:
				self.packages( env, package, optional = True )
				result[ 'have_' + package ] = 1
			except PackageNotFoundException, e:
				result[ 'have_' + package ] = 0
		return result

