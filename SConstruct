import os
import sys
import openbuild.opt
import openbuild.env
import openbuild.utils

class AMLEnvironment( openbuild.env.Environment ):

	def __init__( self, opts ):
		openbuild.env.Environment.__init__( self, opts )

		self.configure_platform( )

		self.Append( CPPPATH = [ '#/src', '.' ] )

	def configure_platform( self ):
		if self[ 'PLATFORM' ] == 'darwin' or self['PLATFORM'] == 'posix':
			self[ 'il_plugin' ] = os.path.join( 'ardome-ml', 'openimagelib', 'plugins' )
			self[ 'ml_plugin' ] = os.path.join( 'ardome-ml', 'openmedialib', 'plugins' )
			self[ 'install_il_plugin' ] = os.path.join( '$prefix', '$libdir', '$il_plugin' )
			self[ 'install_ml_plugin' ] = os.path.join( '$prefix', '$libdir', '$ml_plugin' )
		else:
			self.libdir = 'bin'
			self[ 'il_plugin' ] = ''
			self[ 'ml_plugin' ] = ''
			self[ 'install_il_plugin' ] = os.path.join( '$prefix', 'bin' )
			self[ 'install_ml_plugin' ] = os.path.join( '$prefix', 'bin' )

		self[ 'stage_il_plugin' ] = os.path.join( '$stage_libdir', '$il_plugin' )
		self[ 'stage_ml_plugin' ] = os.path.join( '$stage_libdir', '$ml_plugin' )
		self[ 'cl_include' ] = os.path.join( '$stage_include', 'ardome-ml', 'opencorelib', 'cl' )
		self[ 'il_include' ] = os.path.join( '$stage_include', 'ardome-ml', 'openimagelib', 'il' )
		self[ 'ml_include' ] = os.path.join( '$stage_include', 'ardome-ml', 'openmedialib', 'ml' )
		self[ 'pl_include' ] = os.path.join( '$stage_include', 'ardome-ml', 'openpluginlib', 'pl' )
		self[ 'pcos_include' ] = os.path.join( '$pl_include', 'pcos' )

		if self['PLATFORM'] == 'darwin':
			self.Append( CPPDEFINES = [ 'OLIB_USE_UTF8' ] ) 
			self.Append( LINKFLAGS = '-undefined dynamic_lookup' )
			self.Append( OBJCFLAGS = [ '-fobjc-gc'] )
		elif self['PLATFORM'] == 'posix':
			self.Append( CPPDEFINES = [ 'OLIB_USE_UTF8', 'OLIB_ON_LINUX' ] ) 
		elif self['PLATFORM'] == 'win32':
			self.Append( CPPDEFINES = [ 'OLIB_USE_UTF16' ] ) 
			self.Append( CCFLAGS = ['/EHsc', '/MD', '/GS', '/GR', '/W3', '/TP', '/Zm800'] )
			self.Append( CPPDEFINES = ['OLIB_USE_UTF16', 'AMF_ON_WINDOWS', 'WIN32', '_WINDOWS', 'UNICODE', '_UNICODE', 'ML_PLUGIN_EXPORTS'] )
		else:
			raise( "Unknown platform" )

	def check_externals( self ):
		if self[ 'PLATFORM' ] == 'posix' or self[ 'PLATFORM' ] == 'darwin':
			return self.check_dependencies( "boost_date_time", "boost_regex", "boost_thread", "boost_filesystem", "boost_python", "boost_signals", "libxml-2.0", "libavformat", "loki", "xerces", "sdl", "uuid" )
		else:
			return self.check_dependencies( "boost_date_time", "boost_regex", "boost_thread", "boost_filesystem", "boost_python", "boost_signals", "libavformat", "loki", "xerces", "sdl" )

	def olib_core_cxxflags( self ):
		if self['PLATFORM'] == 'darwin':
			return '-DOLIB_USE_UTF8'
		elif self['PLATFORM'] == 'posix':
			return '-DOLIB_USE_UTF8 -DOLIB_ON_LINUX'
		elif self['PLATFORM'] == 'win32':
			return '-DOLIB_USE_UTF16'
		else:
			raise( "Unknown platform" )

	def olib_ldflags( self ):
		if self['PLATFORM'] == 'darwin':
			return ''
		elif self['PLATFORM'] == 'posix':
			return '-Wl,--export-dynamic'
		else:
			raise( "Don't know" )

	def create_package( self ):
		if self['PLATFORM'] == 'darwin' or self['PLATFORM'] == 'posix':
			clone = self.Clone( )
			clone.prep_release( )
			tokens = [ ( '@prefix@', clone[ 'prefix' ] ),
					   ( '@exec_prefix@', '${prefix}/bin' ),
					   ( '@libdir@', os.path.join( '${prefix}', clone[ 'libdir' ] ) ),
					   ( '@includedir@', '${prefix}/include' ),
					   ( '@OPENIMAGELIB_PLUGINPATH@', clone[ 'install_il_plugin' ] ),
					   ( '@OPENMEDIALIB_PLUGINPATH@', clone[ 'install_ml_plugin' ] ), 
					   ( '@OL_MAJOR@.@OL_MINOR@.@OL_SUB@', '1.0.0' ),
					   ( '@OPENCORELIB_LDFLAGS@', '-L${libdir} -lopencorelib_cl' ),
					   ( '@OPENPLUGINLIB_LDFLAGS@', '-lopenpluginlib_pl' ),
					   ( '@OPENIMAGELIB_LDFLAGS@', '-lopenimagelib_il' ),
					   ( '@OPENMEDIALIB_LDFLAGS@', '-lopenmedialib_ml' ),
					   ( '@BOOST_FILESYSTEM_LIBS@', clone.package_libs( 'boost_filesystem' ) ),
					   ( '@BOOST_THREAD_LIBS@', clone.package_libs( 'boost_thread' ) ),
					   ( '@OLIB_CORE_CXXFLAGS@', clone.olib_core_cxxflags( ) ),
					   ( '@OLIB_LDFLAGS@', clone.olib_ldflags( ) ) ]
			openbuild.utils.search_and_replace( 'ardome_ml.pc.in', 'ardome_ml.pc', tokens )
			clone.Install( clone[ 'stage_pkgconfig' ], 'ardome_ml.pc' )

	def install_openbuild( self ):
		use = ''
		start = os.path.join( self[ 'prefix' ], self[ 'libdir' ], 'python' )
		for path in sys.path:
			if path.startswith( start ) and path.endswith( 'site-packages' ):
				use = path
				break
		if use != '':
			self.Install( os.path.join( self[ 'distdir' ] + use, 'openbuild' ), Glob( 'openbuild/*.py' ) )
		else:
			print "Couldn't find python site-packages in " + self[ 'prefix' ]

opts = openbuild.opt.create_options( 'options.conf', ARGUMENTS )

env = AMLEnvironment( opts )

if env.check_externals( ):

	cl = env.build( 'src/opencorelib/cl' )
	pl = env.build( 'src/openpluginlib/pl' )
	il = env.build( 'src/openimagelib/il', [ pl ] )
	ml = env.build( 'src/openmedialib/ml', [ pl, il ] )
	
	env.build( 'src/openmedialib/plugins/avformat', [ pl, il, ml ] )
	env.build( 'src/openmedialib/plugins/gensys', [ pl, il, ml ] )
	env.build( 'src/openmedialib/plugins/template', [ pl, il, ml ] )
	env.build( 'src/openmedialib/plugins/sdl', [ pl, il, ml ] )

	env.create_package( )
	env.install_openbuild( )

else:
	print "Dependencies missing - aborting"

