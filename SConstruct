# (C) Ardendo SE 2008
#
# Released under the LGPL

import os
import sys
import glob
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
			self[ 'stage_il_plugin' ] = os.path.join( '$stage_libdir', '$il_plugin' )
			self[ 'stage_ml_plugin' ] = os.path.join( '$stage_libdir', '$ml_plugin' )
			self[ 'stage_examples' ] = os.path.join( '$stage_prefix', 'share', 'amf', 'examples' )
			self[ 'stage_mf_plugin' ] = os.path.join( '$stage_prefix', 'share', 'amf', 'plugins' )
		else:
			self[ 'stage_libdir' ] = os.path.join( '$stage_prefix', 'lib' )
			self[ 'stage_bin' ] = os.path.join( '$stage_prefix', 'bin' )
			self[ 'il_plugin' ] = os.path.join( 'bin', 'aml-plugins' )
			self[ 'ml_plugin' ] = os.path.join( 'bin', 'aml-plugins' )
			self[ 'mf_plugin' ] = os.path.join( 'bin', 'plugins' )
			self[ 'examples' ] = os.path.join( 'bin', 'examples' )
			self[ 'install_il_plugin' ] = os.path.join( '$prefix', '$il_plugins' )
			self[ 'install_ml_plugin' ] = os.path.join( '$prefix', '$ml_plugins' )
			self[ 'stage_il_plugin' ] = os.path.join( '$stage_prefix', '$il_plugin' )
			self[ 'stage_ml_plugin' ] = os.path.join( '$stage_prefix', '$ml_plugin' )
			self[ 'stage_mf_plugin' ] = os.path.join( '$stage_prefix', '$mf_plugin' )
			self[ 'stage_examples' ] = os.path.join( '$stage_prefix', '$examples' )

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
			self.Append( CPPDEFINES = ['OLIB_USE_UTF16', 'AMF_ON_WINDOWS', 'WIN32', 'UNICODE', '_UNICODE', 'ML_PLUGIN_EXPORTS'] )
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
			for type in self.build_types( ):
				clone = self.Clone( )
				type( clone )
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
		for type in self.build_types( ):
			clone = self.Clone( )
			type( clone )
			use = clone.subst( clone[ 'stage_libdir' ] )
			clone.install_dir( os.path.join( use, 'openbuild' ), os.path.join( clone.root, 'openbuild' ) )
			if clone[ 'PLATFORM' ] != 'win32':
				path = os.path.join( 'pkgconfig', clone[ 'target' ] )
				list = glob.glob( os.path.join( path, "*.pc" ) )
				for package in list:
					if package.find( os.sep + 'build_' ) == -1 and package.endswith( '.pc' ):
						clone.Install( os.path.join( use, 'openbuild', 'pkgconfig' ), package )

opts = openbuild.opt.create_options( 'options.conf', ARGUMENTS )

env = AMLEnvironment( opts )


if env.check_externals( ):

	cl = env.build( 'src/opencorelib/cl' )
	
	# Build to test opencorelib:
	cl_unit_test = env.build('test/opencorelib/unit_tests/', [ cl ] )
	cl_test_plugin = env.build( 'test/opencorelib/unit_tests/plugin_test_assembly/', [cl] )
	
	pl = env.build( 'src/openpluginlib/pl' )
	il = env.build( 'src/openimagelib/il', [ pl ] )
	ml = env.build( 'src/openmedialib/ml', [ pl, il ] )
	
	env.build( 'src/openmedialib/plugins/avformat', [ pl, il, ml ] )
	env.build( 'src/openmedialib/plugins/gensys', [ pl, il, ml ] )
	env.build( 'src/openmedialib/plugins/template', [ pl, il, ml ] )
	env.build( 'src/openmedialib/plugins/sdl', [ pl, il, ml ] )
	env.build( 'src/openmedialib/plugins/openal', [ pl, il, ml ] )

	env.create_package( )
	env.install_openbuild( )
	
	
	env.done( 'ardome-ml' )

else:
	print "Dependencies missing - aborting"

