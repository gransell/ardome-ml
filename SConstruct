import os
import sys
import openbuild.opt
import openbuild.env
import openbuild.utils

class AMLEnvironment( openbuild.env.Environment ):

	def __init__( self, opts ):
		openbuild.env.Environment.__init__( self, opts )
		self.Append( ENV = { 'PATH' : os.environ['PATH'] } )
		self.Append( CPPPATH = [ '#/src', '.' ] )
		self.ConfigurePlatform( )
		self.Debug( )
		self[ 'il_plugin' ] = 'ardome-ml/openimagelib/plugins'
		self[ 'ml_plugin' ] = 'ardome-ml/openmedialib/plugins'
		self[ 'install_il_plugin' ] = os.path.join( self[ 'prefix' ], self[ 'libdir' ], self[ 'il_plugin' ] )
		self[ 'install_ml_plugin' ] = os.path.join( self[ 'prefix' ], self[ 'libdir' ], self[ 'ml_plugin' ] )
		self[ 'stage_il_plugin' ] = os.path.join( self[ 'stage_libdir' ], self[ 'il_plugin' ] )
		self[ 'stage_ml_plugin' ] = os.path.join( self[ 'stage_libdir' ], self[ 'ml_plugin' ] )
		self[ 'cl_include' ] = os.path.join( self[ 'stage_include' ], 'ardome-ml/opencorelib/cl' )
		self[ 'il_include' ] = os.path.join( self[ 'stage_include' ], 'ardome-ml/openimagelib/il' )
		self[ 'ml_include' ] = os.path.join( self[ 'stage_include' ], 'ardome-ml/openmedialib/ml' )
		self[ 'pl_include' ] = os.path.join( self[ 'stage_include' ], 'ardome-ml/openpluginlib/pl' )
		self[ 'pcos_include' ] = os.path.join( self[ 'pl_include' ], 'pcos' )

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
			self.Append( CCFLAGS = '-ggdb -O0')
		else:
			if self['PLATFORM'] == 'darwin':
				self.Append( CCFLAGS = ['-Oz', ]) 
				self.Append( OBJCFLAGS = ['-fobjc-gc'] )
			elif self['PLATFORM'] == 'win32':
	   			self.Append( CCFLAGS=['/O2', '/W3', '/EHsc'])
			elif self['PLATFORM'] == 'posix':
				self.Append( CCFLAGS=['-Wall', '-O4', '-pipe'])
			else:
				print "Unknown platform", self['PLATFORM']

	def CoreLibs( self ):
		if self[ 'PLATFORM' ] == 'darwin':
			self[ 'FRAMEWORKS' ] = [ 'ExceptionHandling', 'Foundation', 'CoreServices' ]

	def InstallName( self, lib ):
		if self[ 'PLATFORM' ] == 'darwin':
			self.Append( LINKFLAGS = [ '-Wl,-install_name', '-Wl,@loader_path/%s.dylib' % lib ] )

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
					   ( '@libdir@', os.path.join( '${prefix}', self[ 'libdir' ] ) ),
					   ( '@includedir@', '${prefix}/include' ),
					   ( '@OPENIMAGELIB_PLUGINPATH@', self[ 'install_il_plugin' ] ),
					   ( '@OPENMEDIALIB_PLUGINPATH@', self[ 'install_ml_plugin' ] ), 
					   ( '@OL_MAJOR@.@OL_MINOR@.@OL_SUB@', '1.0.0' ),
					   ( '@OPENCORELIB_LDFLAGS@', '-L${libdir} -lopencorelib_cl' ),
					   ( '@OPENPLUGINLIB_LDFLAGS@', '-lopenpluginlib_pl' ),
					   ( '@OPENIMAGELIB_LDFLAGS@', '-lopenimagelib_il' ),
					   ( '@OPENMEDIALIB_LDFLAGS@', '-lopenmedialib_ml' ),
					   ( '@BOOST_FILESYSTEM_LIBS@', self.package_libs( 'boost_filesystem' ) ),
					   ( '@BOOST_THREAD_LIBS@', self.package_libs( 'boost_thread' ) ),
					   ( '@OLIB_CORE_CXXFLAGS@', self.olib_core_cxxflags( ) ),
					   ( '@OLIB_LDFLAGS@', self.olib_ldflags( ) ) ]
			openbuild.utils.search_and_replace( 'ardome_ml.pc.in', 'ardome_ml.pc', tokens )
			self.Install( self[ 'stage_pkgconfig' ], 'ardome_ml.pc' )

opts = openbuild.opt.create_options( 'options.conf', ARGUMENTS )

env = AMLEnvironment( opts )

if env.check_dependencies( "boost_date_time", "boost_regex", "boost_thread", "boost_filesystem", "boost_python", "boost_signals", "libxml-2.0", "libavformat", "loki", "xerces", "sdl", "uuid" ):
	cl = env.build( 'src/opencorelib/cl' )
	pl = env.build( 'src/openpluginlib/pl' )
	ml = env.build( 'src/openmedialib/ml', [ pl ] )
	il = env.build( 'src/openimagelib/il', [ pl ] )

	env.build( 'src/openmedialib/plugins/avformat', [ pl, il, ml ] )
	env.build( 'src/openmedialib/plugins/gensys', [ pl, il, ml ] )
	env.build( 'src/openmedialib/plugins/template', [ pl, il, ml ] )
	env.build( 'src/openmedialib/plugins/sdl', [ pl, il, ml ] )

	env.CreatePkgConfig( )

else:
	print "Dependencies missing - aborting"

