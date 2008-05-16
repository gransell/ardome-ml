import os
import sys

class AMLEnvironment( Environment ):

	def __init__( self, args ):
		Environment.__init__( self )
		self.Append( ENV = { 'PATH' : os.environ['PATH'] } )
		self.Append( CPPPATH = [ '#/src', '.' ] )
		self.ConfigurePlatform( )
		self.Debug( args.get( 'debug', 0 ) )
		self.root = os.path.abspath( '.' )
		self.prefix = args.get('prefix', '/usr/local')
		self.distdir = args.get('distdir', '')
		self.libdir = 'lib'
		if os.uname( )[ 4 ] == 'x86_64':
			self.libdir = 'lib64'
		self.DefinePkgconfig( )

	def ConfigurePlatform( self ):
		if self['PLATFORM'] == 'darwin':
			self.Append( CPPDEFINES = [ 'OLIB_USE_UTF8' ] ) 
			self.Append( LINKFLAGS = '-undefined dynamic_lookup' )
		elif self['PLATFORM'] == 'posix':
			self.Append( CPPDEFINES = [ 'OLIB_USE_UTF8', 'OLIB_ON_LINUX' ] ) 
		elif self['PLATFORM'] == 'win32':
			self.Append( CPPDEFINES = [ 'OLIB_USE_UTF16', 'OLIB_ON_WINDOWS' ] ) 
		else:
			throw( "Unknown platform" )

	def DefinePkgconfig( self ):
		self.pkgconfig_flags = { }
		self.pkgconfig_path = ''
		for r, d, files in os.walk( 'bcomp' ):
			count = 0
			for f in files:
				if f.endswith( '.pc' ):
					pkg = f.replace( ".pc", "" )
					prefix = ( self.root + '/' + r ).rsplit( '/', 2 )[ 0 ]
					self.pkgconfig_flags[ pkg  ] = prefix
					count += 1
			if count > 0:
				if self.pkgconfig_path != "": self.pkgconfig_path += ':'
				self.pkgconfig_path += self.root + '/' + r

		# Incorrect, but we'll focus on ubuntu for now
		if self['PLATFORM'] == 'posix':
			if self.pkgconfig_path != "": self.pkgconfig_path += ':'
			arch = os.uname( )[ 4 ]
			if arch == 'x86_64':
				self.pkgconfig_path += self.root + '/pkgconfig/ubuntu64'
			else:
				self.pkgconfig_path += self.root + '/pkgconfig/ubuntu32'
		elif self['PLATFORM'] == 'darwin':
			if self.pkgconfig_path != "": self.pkgconfig_path += ':'
			self.pkgconfig_path += '/usr/lib/pkgconfig'

		os.environ[ 'PKG_CONFIG_PATH' ] = self.pkgconfig_path

	def Debug( self, debug ):
		self.debug = debug
		if debug:
			self.build_prefix = 'build/debug'
			self.Append( CCFLAGS = '-ggdb -O0')
		else:
			self.build_prefix = 'build/release'
			if self['PLATFORM'] == 'darwin':
				self.Append( CCFLAGS = ['-Oz', ]) 
				self.Append( OBJCFLAGS = ['-fobjc-gc'] )
			elif self['PLATFORM'] == 'win32':
	   			self.Append( CCFLAGS=['/O2', '/W3', '/EHsc'])
			elif self['PLATFORM'] == 'posix':
				self.Append( CCFLAGS=['-Wall', '-O4', '-pipe'])
			else:
				print "Unknown platform", main_env['PLATFORM']

	def CheckDependencies( self ):
		result = True
		temp = self.Clone( )
		try:
			temp.UseBoost( "boost_date_time", "boost_regex", "boost_thread", "boost_filesystem", "boost_python", "boost_signals" )
			temp.UseXML( )
			temp.UseAvformat( )
			temp.UseLoki( )
			temp.UseXerces( )
			temp.UseSDL( )
		except OSError, e:
			result = False
			print "Dependency check" + str( e )
		return result

	def stage_prefix( self ):
		return self.root + '/' + self.build_prefix

	def stage_include( self ):
		return self.stage_prefix( ) + '/include'

	def stage_libdir( self ):
		return self.stage_prefix( ) + '/' + self.libdir

	def PrefixLocationOIL( self ):
		return self.prefix + '/' + self.libdir + '/ardome-ml/openimagelib/plugins'

	def PrefixLocationOML( self ):
		return self.prefix + '/' + self.libdir + '/ardome-ml/openmedialib/plugins'

	def PluginLocationOIL( self ):
		return self.stage_libdir( ) + '/ardome-ml/openimagelib/plugins'

	def PluginLocationOML( self ):
		return self.stage_libdir( ) + '/ardome-ml/openmedialib/plugins'

	def pkgconfig_cmd( self, switch, part ):
		command = 'pkg-config ' + switch + ' '
		if part in self.pkgconfig_flags.keys( ):
			command += '--define-variable=prefix=' + self.pkgconfig_flags[ part ] + ' '
			if self.debug:
				command += '--define-variable=debug=-d '
		command += part
		return command

	def pkgconfig( self, part ):
		self.ParseConfig( self.pkgconfig_cmd( '--cflags --libs', part ) )
	
	def UseBoost(self, *parts):
		if self['PLATFORM'] == 'darwin' or self['PLATFORM'] == 'posix':
			for part in parts:
				self.pkgconfig( part )
		elif self['PLATFORM'] == 'win32':
			self.Append(CPPPATH = ['#/bcomp/boost/include'], LIBPATH = ['#/bcomp/boost/lib'])
			for part in parts:
				self.Append( LIBS = "-l%s" % part )

	def UseXML(self):
		if self['PLATFORM'] == 'darwin' or self['PLATFORM'] == 'posix':
			self.pkgconfig( 'libxml-2.0' )
		elif self['PLATFORM'] == 'win32':
			self.Append( LIBS = "-lXML2" )

	def UseAvformat( self ):
		if self['PLATFORM'] == 'darwin' or self['PLATFORM'] == 'posix':
			for part in [ 'libavformat', 'libavcodec', 'libavutil' ]:
				self.pkgconfig( part )
		elif self['PLATFORM'] == 'win32':
			pass

	def UseUUID( self ):
		if self['PLATFORM'] == 'posix':
			self.Append( LIBS = '-luuid' )

	def UseLoki( self ):
		if self['PLATFORM'] == 'darwin' or self['PLATFORM'] == 'posix':
			self.pkgconfig( 'loki' )
		else:
			self.Append(CPPPATH = ['#/bcomp/loki/include'])

	def UseXerces( self ):
		if self['PLATFORM'] == 'darwin' or self['PLATFORM'] == 'posix':
			self.pkgconfig( 'xerces' )
		elif self['PLATFORM'] == 'win32':
			self.Append( LIBS = "-lxerces-c" )

	def UseSDL( self ):
		if self['PLATFORM'] == 'darwin' or self['PLATFORM'] == 'posix':
			self.pkgconfig( "sdl" )
		else:
			throw( "TODO: SDL for windows" )

	def CoreLibs( self ):
		if self[ 'PLATFORM' ] == 'darwin':
			self[ 'FRAMEWORKS' ] = [ 'ExceptionHandling', 'Foundation', 'CoreServices' ]

	def InstallName( self, lib ):
		if self[ 'PLATFORM' ] == 'darwin':
			self.Append( LINKFLAGS = [ '-Wl,-install_name', '-Wl,@loader_path/%s.dylib' % lib ] )

	def Build( self, path ):
		self.SConscript( [ path + '/SConscript' ], exports=[ 'main_env' ] )

	def boost_filesystem_libs( self ):
		return os.popen( self.pkgconfig_cmd( "--libs", "boost_filesystem" ) ).read( ).replace( '\n', '' )

	def boost_thread_libs( self ):
		return os.popen( self.pkgconfig_cmd( "--libs", "boost_thread" ) ).read( ).replace( '\n', '' )

	def olib_core_cxxflags( self ):
		if self['PLATFORM'] == 'darwin':
			return '-DOLIB_USE_UTF8'
		elif self['PLATFORM'] == 'posix':
			return '-DOLIB_USE_UTF8 -DOLIB_ON_LINUX'
		elif self['PLATFORM'] == 'win32':
			return '-DOLIB_USE_UTF16 -DOLIB_ON_WINDOWS'
		else:
			throw( "Unknown platform" )

	def olib_ldflags( self ):
		if self['PLATFORM'] == 'darwin':
			return ''
		elif self['PLATFORM'] == 'posix':
			return '-Wl,--export-dynamic'
		else:
			throw( "Don't know" )

	def CreatePkgConfig( self ):
		if self['PLATFORM'] == 'darwin' or self['PLATFORM'] == 'posix':
			tokens = [ ( '@prefix@', main_env.prefix ),
					   ( '@exec_prefix@', '${prefix}/bin' ),
					   ( '@libdir@', '${prefix}/' + self.libdir ),
					   ( '@includedir@', '${prefix}/include' ),
					   ( '@OPENIMAGELIB_PLUGINPATH@', main_env.PrefixLocationOIL( ) ),
					   ( '@OPENMEDIALIB_PLUGINPATH@', main_env.PrefixLocationOML( ) ), 
					   ( '@OL_MAJOR@.@OL_MINOR@.@OL_SUB@', '1.0.0' ),
					   ( '@OPENCORELIB_LDFLAGS@', '-L${libdir} -lopencorelib_cl' ),
					   ( '@OPENPLUGINLIB_LDFLAGS@', '-lopenpluginlib_pl' ),
					   ( '@OPENIMAGELIB_LDFLAGS@', '-lopenimagelib_il' ),
					   ( '@OPENMEDIALIB_LDFLAGS@', '-lopenmedialib_ml' ),
					   ( '@BOOST_FILESYSTEM_LIBS@', self.boost_filesystem_libs( ) ),
					   ( '@BOOST_THREAD_LIBS@', self.boost_thread_libs( ) ),
					   ( '@OLIB_CORE_CXXFLAGS@', self.olib_core_cxxflags( ) ),
					   ( '@OLIB_LDFLAGS@', self.olib_ldflags( ) ) ]
			search_and_replace( 'ardome_ml.pc.in', 'ardome_ml.pc.out', tokens )

def search_and_replace( filename, out, tokens ):
	input = open( filename )
	output = open( out, 'w' )
	result = input.read( )
	for couple in tokens:
		result = result.replace( couple[ 0 ], couple[ 1 ] )
	output.write( result )
	output.close( )
	input.close( )

def add_options( opts ):
	opts.Add( 'prefix', "Directory of architecture independant files.", "/usr" )
	opts.Add( 'eprefix', "Directory of architecture dependant files.", "${%s}" % 'prefix' )
	opts.Add( 'bindir', "Directory of executables.", "${%s}/bin" % 'eprefix' )
	opts.Add( 'libdir', "Directory of libraries.", "${%s}/lib" % 'eprefix' )
	opts.Add( 'includedir', "Directory of header files.", "${%s}/include" % 'prefix' )
	opts.Add( 'distdir', "Directory to actually install to.  Prefix will be used inside this.",
                  "/" )

main_env = AMLEnvironment( ARGUMENTS )
opts = Options( 'options.conf', ARGUMENTS )
add_options( opts )
opts.Update( main_env )
opts.Save( 'options.conf', main_env )
Help( opts.GenerateHelpText( main_env ) )

main_env.Alias( target="install", source = main_env.Install(dir = main_env.distdir + '/' + main_env.prefix, source= "build/release/") )

if main_env.CheckDependencies( ):
	main_env.Build( 'src/openmedialib/ml' )
	main_env.Build( 'src/opencorelib/cl' )
	main_env.Build( 'src/openpluginlib/pl' )
	main_env.Build( 'src/openimagelib/il' )

	main_env.Build( 'src/openmedialib/plugins/avformat' )
	main_env.Build( 'src/openmedialib/plugins/gensys' )
	main_env.Build( 'src/openmedialib/plugins/template' )
	main_env.Build( 'src/openmedialib/plugins/sdl' )

	main_env.CreatePkgConfig( )

else:
	print "Dependencies missing - aborting"

