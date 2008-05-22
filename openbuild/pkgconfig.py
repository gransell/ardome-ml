import os
import utils

def walk( self ):
	"""Walk the bcomp directory to pick out all the .pc files"""
	flags = { }
	paths = { }
	for r, d, files in os.walk( self.root + '/bcomp' ):
		for f in files:
			if f.endswith( '.pc' ):
				pkg = f.replace( ".pc", "" )
				prefix = r.rsplit( '/', 2 )[ 0 ]
				flags[ pkg ] = prefix
				paths[ prefix ] = 1

	full = ''
	for path in paths:
		full += path + ':'

	if self[ 'PLATFORM' ] == 'posix':
		if utils.arch( ) == 'x86_64':
			full += self.root + '/pkgconfig/ubuntu64'
		else:
			full += self.root + '/pkgconfig/ubuntu32'
	elif self[ 'PLATFORM' ] == 'darwin':
		full += '/usr/lib/pkgconfig:' + self.root + '/pkgconfig/mac'

	os.environ[ 'PKG_CONFIG_PATH' ] = full
	self.package_list = flags

def packages( self, *packages ):
	"""Extracts compile and link flags for the specified packages and adds to the 
	current environment."""
	for package in packages:
		self.ParseConfig( pkgconfig_cmd( self, package ) )

def package_cflags( self, package ):
	"""Extracts compile flags for the specified packages."""
	return os.popen( pkgconfig_cmd( self, package, "--cflags" ) ).read( ).replace( '\n', '' )

def package_libs( self, package ):
	"""Extracts lib flags for the specified packages."""
	return os.popen( pkgconfig_cmd( self, package, "--libs" ) ).read( ).replace( '\n', '' )

def pkgconfig_cmd( self, package, switches = '--cflags --libs' ):
	"""General purpose accessor for pkg-config - will override to use bcomp prefix 
	when necessary."""
	command = 'pkg-config %s ' % switches
	if package in self.package_list.keys( ):
		command += '--define-variable=prefix=' + self.package_list[ package ] + ' '
		if self[ 'debug' ] == '1':
			command += '--define-variable=debug=-d '
	command += package
	return command


