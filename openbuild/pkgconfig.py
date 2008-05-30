import os
import utils

class PkgConfig:
	def walk( self, env ):
		"""Walk the bcomp directory to pick out all the .pc files"""
		flags = { }
		paths = { }
		for r, d, files in os.walk( env.root + '/bcomp' ):
			for f in files:
				if f.endswith( '.pc' ):
					pkg = f.replace( ".pc", "" )
					prefix = r.rsplit( '/', 2 )[ 0 ]
					flags[ pkg ] = prefix
					paths[ prefix ] = 1

		full = ''
		for path in paths:
			full += path + '/lib/pkgconfig' + ':'

		if env[ 'PLATFORM' ] == 'posix':
			if utils.arch( ) == 'x86_64':
				full += env.root + '/pkgconfig/ubuntu64'
			else:
				full += env.root + '/pkgconfig/ubuntu32'
		elif env[ 'PLATFORM' ] == 'darwin':
			full += '/usr/lib/pkgconfig:' + env.root + '/pkgconfig/mac'

		full += ':' + os.path.join( __file__.rsplit( '/', 1 )[ 0 ], 'pkgconfig' )

		os.environ[ 'PKG_CONFIG_PATH' ] = full
		return flags

	def packages( self, env, *packages ):
		"""Extracts compile and link flags for the specified packages and adds to the 
		current environment."""
		for package in packages:
			env.ParseConfig( self.pkgconfig_cmd( env, package ) )

	def package_cflags( self, env, package ):
		"""Extracts compile flags for the specified packages."""
		return os.popen( self.pkgconfig_cmd( env, package, "--cflags" ) ).read( ).replace( '\n', '' )

	def package_libs( self, env, package ):
		"""Extracts lib flags for the specified packages."""
		return os.popen( self.pkgconfig_cmd( env, package, "--libs" ) ).read( ).replace( '\n', '' )

	def pkgconfig_cmd( self, env, package, switches = '--cflags --libs' ):
		"""General purpose accessor for pkg-config - will override to use bcomp prefix 
		when necessary."""
		command = 'pkg-config %s ' % switches
		if package in env.package_list.keys( ):
			command += '--define-variable=prefix=' + env.package_list[ package ] + ' '
			if env[ 'debug' ] == '1':
				command += '--define-variable=debug=-d '
		command += package
		return command


