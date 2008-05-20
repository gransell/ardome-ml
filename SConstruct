import os
import sys
import openbuild.env

class AMLEnvironment( openbuild.env.Environment ):

	def __init__( self, opts ):
		openbuild.env.Environment.__init__( self )
		opts.Update( self )
		self.install = False
		self.Append( ENV = { 'PATH' : os.environ['PATH'] } )
		self.Append( CPPPATH = [ '#/src', '.' ] )
		self.ConfigurePlatform( )
		self.Debug( )
		self.root = os.path.abspath( '.' )
		self.libdir = 'lib'
		if self[ 'PLATFORM' ] != 'win32' and os.uname( )[ 4 ] == 'x86_64':
			self.libdir = 'lib64'

	def SetInstall( self ):
		self.install = 'install' in sys.argv
		return self.stage_prefix( )

	def ConfigurePlatform( self ):
		if self['PLATFORM'] == 'darwin':
			self.Append( CPPDEFINES = [ 'OLIB_USE_UTF8' ] ) 
			self.Append( LINKFLAGS = '-undefined dynamic_lookup' )
		elif self['PLATFORM'] == 'posix':
			self.Append( CPPDEFINES = [ 'OLIB_USE_UTF8', 'OLIB_ON_LINUX' ] ) 
		elif self['PLATFORM'] == 'win32':
			self.Append( CPPDEFINES = [ 'OLIB_USE_UTF16' ] ) 
		else:
			throw( "Unknown platform" )

	def Debug( self ):
		if self[ 'debug' ] == '1':
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
			temp.packages( "boost_date_time", "boost_regex", "boost_thread", "boost_filesystem", "boost_python", "boost_signals", "libxml-2.0", "libavformat", "loki", "xerces", "sdl", "uuid" )
		except OSError, e:
			result = False
			print "Dependency check" + str( e )
		return result

	def stage_prefix( self ):
		if not self.install:
			return self.root + '/' + self.build_prefix
		else:
			return self[ 'distdir' ] + '/' + self[ 'prefix' ]

	def stage_include( self ):
		return self.stage_prefix( ) + '/include'

	def stage_libdir( self ):
		return self.stage_prefix( ) + '/' + self.libdir

	def PrefixLocationOIL( self ):
		return self[ 'prefix' ] + '/' + self.libdir + '/ardome-ml/openimagelib/plugins'

	def PrefixLocationOML( self ):
		return self[ 'prefix' ] + '/' + self.libdir + '/ardome-ml/openmedialib/plugins'

	def PluginLocationOIL( self ):
		return self.stage_libdir( ) + '/ardome-ml/openimagelib/plugins'

	def PluginLocationOML( self ):
		return self.stage_libdir( ) + '/ardome-ml/openmedialib/plugins'

	def CoreLibs( self ):
		if self[ 'PLATFORM' ] == 'darwin':
			self[ 'FRAMEWORKS' ] = [ 'ExceptionHandling', 'Foundation', 'CoreServices' ]

	def InstallName( self, lib ):
		if self[ 'PLATFORM' ] == 'darwin':
			self.Append( LINKFLAGS = [ '-Wl,-install_name', '-Wl,@loader_path/%s.dylib' % lib ] )

	def build( self, path, deps = [] ):
		local_env = self.Clone( )
		local_env.Append( LIBS = deps )
		return local_env.SConscript( [ path + '/SConscript' ], exports=[ 'local_env' ] )

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
			tokens = [ ( '@prefix@', self[ 'prefix' ] ),
					   ( '@exec_prefix@', '${prefix}/bin' ),
					   ( '@libdir@', '${prefix}/' + self.libdir ),
					   ( '@includedir@', '${prefix}/include' ),
					   ( '@OPENIMAGELIB_PLUGINPATH@', self.PrefixLocationOIL( ) ),
					   ( '@OPENMEDIALIB_PLUGINPATH@', self.PrefixLocationOML( ) ), 
					   ( '@OL_MAJOR@.@OL_MINOR@.@OL_SUB@', '1.0.0' ),
					   ( '@OPENCORELIB_LDFLAGS@', '-L${libdir} -lopencorelib_cl' ),
					   ( '@OPENPLUGINLIB_LDFLAGS@', '-lopenpluginlib_pl' ),
					   ( '@OPENIMAGELIB_LDFLAGS@', '-lopenimagelib_il' ),
					   ( '@OPENMEDIALIB_LDFLAGS@', '-lopenmedialib_ml' ),
					   ( '@BOOST_FILESYSTEM_LIBS@', self.package_libs( 'boost_filesystem' ) ),
					   ( '@BOOST_THREAD_LIBS@', self.package_libs( 'boost_thread' ) ),
					   ( '@OLIB_CORE_CXXFLAGS@', self.olib_core_cxxflags( ) ),
					   ( '@OLIB_LDFLAGS@', self.olib_ldflags( ) ) ]
			search_and_replace( 'ardome_ml.pc.in', 'ardome_ml.pc', tokens )
			self.Install( self.stage_libdir( ) + '/pkgconfig/', 'ardome_ml.pc' )

def search_and_replace( filename, out, tokens ):
	input = open( filename )
	output = open( out, 'w' )
	result = input.read( )
	for couple in tokens:
		result = result.replace( couple[ 0 ], couple[ 1 ] )
	output.write( result )
	output.close( )
	input.close( )

opts = Options( 'options.conf', ARGUMENTS )

opts.Add( 'prefix', "Directory of architecture independant files.", "/usr/local" )
opts.Add( 'distdir', "Directory to actually install to.  Prefix will be used inside this.", "" )
opts.Add( 'debug', "Debug or release - 1 or 0 resp.", '0' )

main_env = AMLEnvironment( opts )

opts.Save( 'options.conf', main_env )
Help( opts.GenerateHelpText( main_env ) )

main_env.Alias( target = "install", source = main_env.SetInstall( ) )

if main_env.CheckDependencies( ):
	cl = main_env.build( 'src/opencorelib/cl' )
	pl = main_env.build( 'src/openpluginlib/pl' )
	ml = main_env.build( 'src/openmedialib/ml', [ pl ] )
	il = main_env.build( 'src/openimagelib/il', [ pl ] )

	main_env.build( 'src/openmedialib/plugins/avformat', [ pl, il, ml ] )
	main_env.build( 'src/openmedialib/plugins/gensys', [ pl, il, ml ] )
	main_env.build( 'src/openmedialib/plugins/template', [ pl, il, ml ] )
	main_env.build( 'src/openmedialib/plugins/sdl', [ pl, il, ml ] )

	main_env.CreatePkgConfig( )

else:
	print "Dependencies missing - aborting"

