# (C) Ardendo SE 2008
#
# Released under the LGPL

import os
import utils
import glob
import SCons.Node.FS
import SCons

class PkgConfig:
	def walk( self, env ):
		"""Walk the bcomp directory to pick out all the .pc files"""
		flags = { }
		paths = { }
		full_paths = { }
		dirs = [ env.root + '/bcomp' ]
		while len( dirs ):
			dir = dirs[ 0 ]
			dirs.remove( dir )
			for r, d, files in os.walk( dir ):
				for dir in d:
					full = os.path.join( r, dir )
					if os.path.islink( full ) and not os.readlink( full ) == '.':
						dirs += [ full ]
				for f in files:
					if f.endswith( '.pc' ) and r.endswith( '/lib/pkgconfig' ) or r.endswith( '/lib64/pkgconfig' ):
						pkg = f.replace( ".pc", "" )
						prefix = r.rsplit( '/', 2 )[ 0 ]
						if prefix.endswith( '/debug' ): pkg = 'debug_' + pkg
						if prefix.endswith( '/release' ): pkg = 'release_' + pkg
						if pkg in flags:
							raise Exception, 'Found two different pkgconfig files for package %s:\n%s\nand:\n%s' % \
								( pkg, os.path.join( flags[ pkg ], 'lib/pkgconfig', f ), os.path.join( r, f ) )
						flags[ pkg ] = prefix
						paths[ prefix ] = 1
						full_paths[ r ] = 1

		full = ''
		for path in full_paths:
			full += path + ':'

		if env[ 'PLATFORM' ] == 'posix':
			if utils.arch( ) == 'x86_64':
				full += env.root + '/pkgconfig/ubuntu64'
			else:
				full += env.root + '/pkgconfig/ubuntu32'
		elif env[ 'PLATFORM' ] == 'darwin':
			full += '/usr/lib/pkgconfig:' + env.root + '/pkgconfig/osx'

		full += ':' + os.path.join( env.root, __file__.rsplit( '/', 1 )[ 0 ], 'pkgconfig' )

		if 'PKG_CONFIG_PATH' not in os.environ.keys( ) or os.environ[ 'PKG_CONFIG_PATH' ] == '':
			os.environ[ 'PKG_CONFIG_PATH' ] = full
		else:
			os.environ[ 'PKG_CONFIG_PATH' ] += ':' + full

		return flags

	def package_name( self, env, package ):
		if env.debug and 'debug_' + package in env.package_list.keys( ): return 'debug_' + package
		elif not env.debug and 'release_' + package in env.package_list.keys( ): return 'release_' + package
		return package

	def optional( self, env, *packages ):
		"""Extracts compile and link flags for the specified packages and adds to the 
		current environment along with a have_package variable to allow dependency checks."""
		result = {}
		for package in packages:
			try:
				env.ParseConfig( self.pkgconfig_cmd( env, package ) )
				result[ 'have_' + package ] = 1
			except Exception, e:
				result[ 'have_' + package ] = 0
		return result

	def packages( self, env, *packages ):
		"""Extracts compile and link flags for the specified packages and adds to the 
		current environment."""
		for package in packages:
			env.ParseConfig( self.pkgconfig_cmd( env, package ) )

	def pkgconfig_cmd( self, env, package, switches = '--cflags --libs' ):
		"""General purpose accessor for pkg-config - will override to use bcomp prefix 
		when necessary."""
		command = 'PKG_CONFIG_PATH="%s" PKG_CONFIG_TOP_BUILD_DIR="%s" pkg-config %s ' % ( os.getenv( 'PKG_CONFIG_PATH' ), SCons.Node.FS.default_fs.Dir('#').abspath, switches )
		name = self.package_name( env, package )
		if name in env.package_list.keys( ):
			command += '--define-variable=prefix=' + env.package_list[ name ] + ' '
			if env.debug:
				command += '--define-variable=debug=-d '
		command += package + ' 2> /dev/null'
		return command

	def package_install_list( self, env, key ):
		result = { }
		for package in env.package_list.keys( ):
			if env.debug and package.startswith( 'release_' ): continue
			if not env.debug and package.startswith( 'debug_' ): continue
			pkg = package.replace( 'debug_', '' ).replace( 'release_', '' )
			if env.package_list[ package ].startswith( os.path.join( env.root, 'bcomp' ) ):
				include = os.popen( self.pkgconfig_cmd( env, pkg, "--variable=" + key ) ).read( ).replace( '\n', '' )
				if include != '':
					for component in include.split( ' ' ):
						globbed_files = glob.glob( component )
						if len( globbed_files ) == 0:
							raise Exception, "Could not find the file:\n%s\nreferred to by rule \"%s\" in pkgconfig file for package \"%s\"" % ( component, key, package )

						for file in glob.glob( component ):
							name = ''
							if component.find( '*' ) != -1: name = file.rsplit( '/', 1 )[ -1 ]
							if name not in result.keys( ): result[ name ] = [ ]
							result[ name ] += [ file ]
		return result

