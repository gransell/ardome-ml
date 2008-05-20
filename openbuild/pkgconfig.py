import os

def arch( ):
	return os.uname( )[ 4 ]

def walk( self ):
	flags = { }
	paths = { }
	for r, d, files in os.walk( self.root + '/bcomp' ):
		for f in files:
			if f.endswith( '.pc' ):
				pkg = f.replace( ".pc", "" )
				prefix = r.rsplit( '/', 2 )[ 0 ]
				pkgconfig_flags[ pkg ] = prefix
				paths[ prefix ] = 1

	full = ''
	for path in paths:
		full += path + ':'

	if self[ 'PLATFORM' ] == 'posix':
		if arch( ) == 'x86_64':
			full += self.root + '/pkgconfig/ubuntu64'
		else:
			full += self.root + '/pkgconfig/ubuntu32'
	elif self[ 'PLATFORM' ] == 'darwin':
		full += '/usr/lib/pkgconfig:' + self.root + '/pkgconfig/mac'

	os.environ[ 'PKG_CONFIG_PATH' ] = full
	return flags

def packages( self, *packages ):
	for package in packages:
		self.ParseConfig( pkgconfig_cmd( self, package ) )

def package_cflags( self, package ):
	return os.popen( pkgconfig_cmd( self, package, "--cflags" ) ).read( ).replace( '\n', '' )

def package_libs( self, package ):
	return os.popen( pkgconfig_cmd( self, package, "--libs" ) ).read( ).replace( '\n', '' )

def pkgconfig_cmd( self, package, switches = '--cflags --libs' ):
	command = 'pkg-config %s ' % switches
	if package in self.package_list.keys( ):
		command += '--define-variable=prefix=' + self.package_list[ package ] + ' '
		if self[ 'debug' ] == '1':
			command += '--define-variable=debug=-d '
	command += package
	return command


