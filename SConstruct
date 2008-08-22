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

	def install_packages( self ):
		if self[ 'target' ] == 'vs2003':
			self.install_config( 'config/common/loki.wc', 'bcomp/common/loki-0.1.6' )
			self.install_config( 'config/common/sdl.wc', 'bcomp/common/sdl' )
			self.install_config( 'config/common/libavformat.wc', 'bcomp/common/ffmpeg' )
			self.install_config( 'config/vs2003/xerces.wc', 'bcomp/vs2003/xerces-c_2_8_0-x86-windows-vc_7_1' )
			for package in [ 'boost_python.wc', 'boost_filesystem.wc', 'boost_thread.wc', 'boost_regex.wc', 
							 'boost_date_time.wc', 'boost_unit_test_framework.wc', 'boost.wc', 'boost_signals.wc' ]:
				self.install_config( 'config/vs2003/' + package, 'bcomp/vs2003/boost_1_34_1' )
		elif self[ 'target' ] == 'osx':
			self.install_config( 'config/osx/boost.pc', 'bcomp/boost' )
			self.install_config( 'config/osx/boost_date_time.pc', 'bcomp/boost' )
			self.install_config( 'config/osx/boost_filesystem.pc', 'bcomp/boost' )
			self.install_config( 'config/osx/boost_python.pc', 'bcomp/boost' )
			self.install_config( 'config/osx/boost_regex.pc', 'bcomp/boost' )
			self.install_config( 'config/osx/boost_signals.pc', 'bcomp/boost' )
			self.install_config( 'config/osx/boost_thread.pc', 'bcomp/boost' )
			self.install_config( 'config/osx/boost_unit_test_framework.pc', 'bcomp/boost' )
			self.install_config( 'config/osx/libavcodec.pc', 'bcomp/ffmpeg' )
			self.install_config( 'config/osx/libavdevice.pc', 'bcomp/ffmpeg' )
			self.install_config( 'config/osx/libavformat.pc', 'bcomp/ffmpeg' )
			self.install_config( 'config/osx/libavutil.pc', 'bcomp/ffmpeg' )
			self.install_config( 'config/osx/libswscale.pc', 'bcomp/ffmpeg' )
			self.install_config( 'config/osx/loki.pc', 'bcomp/loki' )
			self.install_config( 'config/osx/sdl.pc', 'bcomp/SDL' )
			self.install_config( 'config/osx/xerces.pc', 'bcomp/xercesc' )
		elif self[ 'target' ] == 'ubuntu32':
			if os.path.exists( 'bcomp/ffmpeg' ):
				for package in [ 'libavcodec.pc', 'libavdevice.pc', 'libavformat.pc', 'libavutil.pc', 'libswscale.pc' ]:
					self.install_config( 'config/ubuntu32/' + package, 'bcomp/ffmpeg' )
		elif self[ 'target' ] == 'ubuntu64':
			if os.path.exists( 'bcomp/ffmpeg' ):
				for package in [ 'libavcodec.pc', 'libavdevice.pc', 'libavformat.pc', 'libavutil.pc', 'libswscale.pc' ]:
					self.install_config( 'config/ubuntu64/' + package, 'bcomp/ffmpeg' )
		elif self[ 'target' ] == 'lsb_3_1_32':
			self.install_config( 'config/lsb_3_1_32/boost.pc', 'bcomp/boost' )
			self.install_config( 'config/lsb_3_1_32/boost_date_time.pc', 'bcomp/boost' )
			self.install_config( 'config/lsb_3_1_32/boost_filesystem.pc', 'bcomp/boost' )
			self.install_config( 'config/lsb_3_1_32/boost_python.pc', 'bcomp/boost' )
			self.install_config( 'config/lsb_3_1_32/boost_regex.pc', 'bcomp/boost' )
			self.install_config( 'config/lsb_3_1_32/boost_signals.pc', 'bcomp/boost' )
			self.install_config( 'config/lsb_3_1_32/boost_thread.pc', 'bcomp/boost' )
			self.install_config( 'config/lsb_3_1_32/boost_unit_test_framework.pc', 'bcomp/boost' )
			self.install_config( 'config/lsb_3_1_32/loki.pc', 'bcomp/loki-0.1.6' )
			self.install_config( 'config/lsb_3_1_32/xerces.pc', 'bcomp/xercesc' )
			self.install_config( 'config/lsb_3_1_32/libavcodec.pc', 'bcomp/ffmpeg' )
			self.install_config( 'config/lsb_3_1_32/libavdevice.pc', 'bcomp/ffmpeg' )
			self.install_config( 'config/lsb_3_1_32/libavformat.pc', 'bcomp/ffmpeg' )
			self.install_config( 'config/lsb_3_1_32/libavutil.pc', 'bcomp/ffmpeg' )
			self.install_config( 'config/lsb_3_1_32/sdl.pc', 'bcomp/sdl' )
		elif self[ 'target' ] == 'lsb_3_1_64':
			self.install_config( 'config/lsb_3_1_64/boost.pc', 'bcomp/boost' )
			self.install_config( 'config/lsb_3_1_64/boost_date_time.pc', 'bcomp/boost' )
			self.install_config( 'config/lsb_3_1_64/boost_filesystem.pc', 'bcomp/boost' )
			self.install_config( 'config/lsb_3_1_64/boost_python.pc', 'bcomp/boost' )
			self.install_config( 'config/lsb_3_1_64/boost_regex.pc', 'bcomp/boost' )
			self.install_config( 'config/lsb_3_1_64/boost_signals.pc', 'bcomp/boost' )
			self.install_config( 'config/lsb_3_1_64/boost_thread.pc', 'bcomp/boost' )
			self.install_config( 'config/lsb_3_1_64/boost_unit_test_framework.pc', 'bcomp/boost' )
			self.install_config( 'config/lsb_3_1_64/loki.pc', 'bcomp/loki-0.1.6' )
			self.install_config( 'config/lsb_3_1_64/xerces.pc', 'bcomp/xercesc' )
			self.install_config( 'config/lsb_3_1_64/libavcodec.pc', 'bcomp/ffmpeg' )
			self.install_config( 'config/lsb_3_1_64/libavdevice.pc', 'bcomp/ffmpeg' )
			self.install_config( 'config/lsb_3_1_64/libavformat.pc', 'bcomp/ffmpeg' )
			self.install_config( 'config/lsb_3_1_64/libavutil.pc', 'bcomp/ffmpeg' )
			self.install_config( 'config/lsb_3_1_64/sdl.pc', 'bcomp/sdl' )

	def configure_platform( self ):
		if self['PLATFORM'] == 'posix' or self[ 'PLATFORM' ] == 'darwin':
			if self[ 'PLATFORM' ] == 'darwin':
				self[ 'il_plugin' ] = ''
				self[ 'ml_plugin' ] = ''
				self[ 'stage_mf_schemas' ] = os.path.join( '$stage_prefix', 'Resources', 'schemas' )
				self[ 'stage_examples' ] = os.path.join( '$stage_prefix', 'Resources', 'examples' )
				self[ 'stage_mf_plugin' ] = os.path.join( '$stage_prefix', 'lib' )
			else:
				self[ 'il_plugin' ] = os.path.join( 'ardome-ml', 'openimagelib', 'plugins' )
				self[ 'ml_plugin' ] = os.path.join( 'ardome-ml', 'openmedialib', 'plugins' )
				self[ 'stage_mf_schemas' ] = os.path.join( '$stage_prefix', 'share', 'amf', 'schemas' )
				self[ 'stage_examples' ] = os.path.join( '$stage_prefix', 'share', 'amf', 'examples' )
				self[ 'stage_mf_plugin' ] = os.path.join( '$stage_prefix', 'share', 'amf', 'plugins' )
				
			self[ 'install_il_plugin' ] = os.path.join( '$prefix', '$libdir', '$il_plugin' )
			self[ 'install_ml_plugin' ] = os.path.join( '$prefix', '$libdir', '$ml_plugin' )
			self[ 'stage_il_plugin' ] = os.path.join( '$stage_libdir', '$il_plugin' )
			self[ 'stage_ml_plugin' ] = os.path.join( '$stage_libdir', '$ml_plugin' )
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
			self[ 'stage_mf_schemas' ] = os.path.join( '$stage_prefix', 'schemas' )
			self[ 'stage_examples' ] = os.path.join( '$stage_prefix', '$examples' )

		self[ 'cl_include' ] = os.path.join( '$stage_include', 'ardome-ml', 'opencorelib', 'cl' )
		self[ 'il_include' ] = os.path.join( '$stage_include', 'ardome-ml', 'openimagelib', 'il' )
		self[ 'ml_include' ] = os.path.join( '$stage_include', 'ardome-ml', 'openmedialib', 'ml' )
		self[ 'pl_include' ] = os.path.join( '$stage_include', 'ardome-ml', 'openpluginlib', 'pl' )
		self[ 'pcos_include' ] = os.path.join( '$pl_include', 'pcos' )

		if self[ 'target' ] == 'vs2003' or self[ 'target' ] == 'vs2008':
			self[ 'python_packages' ] = os.path.join( '$stage_libdir', 'site-packages' )
		else:
			version = sys.version_info
			self[ 'python_packages' ] = os.path.join( '$stage_libdir', 'python%d.%d' % ( version[ 0 ], version[ 1 ] ), 'site-packages' )

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

	def have_boost_python( self ):
		clone = self.Clone( )
		clone.prep_release( )
		has_python = clone.optional( 'boost_python' )[ 'have_boost_python' ]
		if has_python:
			clone.copy_files( self[ 'python_packages' ], 'py' )
			clone.copy_files( self[ 'stage_bin' ], 'scripts', perms = 0755 )
		return has_python

opts = openbuild.opt.create_options( 'options.conf', ARGUMENTS )

env = AMLEnvironment( opts )

if env.check_externals( ):

	cl = env.build( 'src/opencorelib/cl' )
	
	# Build to test opencorelib:
	cl_unit_test = env.build('test/opencorelib/unit_tests/', [ cl ] )
	cl_test_plugin = env.build( 'test/opencorelib/unit_tests/plugin_test_assembly/', [cl] )
	
	pl = env.build( 'src/openpluginlib/pl', [ cl ] )
	il = env.build( 'src/openimagelib/il', [ pl ] )
	ml = env.build( 'src/openmedialib/ml', [ pl, cl, il ] )
	
	env.build( 'src/openmedialib/plugins/avformat', [ cl, pl, il, ml ] )
	env.build( 'src/openmedialib/plugins/gensys', [ cl, pl, il, ml ] )
	env.build( 'src/openmedialib/plugins/template', [ cl, pl, il, ml ] )
	env.build( 'src/openmedialib/plugins/sdl', [ cl, pl, il, ml ] )
	env.build( 'src/openmedialib/plugins/openal', [ cl, pl, il, ml ] )

	if env.have_boost_python( ):
		env.build( 'src/openpluginlib/py', [ cl, pl, il, ml ] )
		env.build( 'src/openimagelib/py', [ cl, pl, il ] )
		env.build( 'src/openmedialib/py', [ cl, pl, il, ml ] )

	env.create_package( )
	env.install_openbuild( )

	env.package_install( )

	# Makes it possible for the visual studio builder to terminate scons.
	if not env.done( 'ardome-ml' ) : 
		exit()

else:
	print "Dependencies missing - aborting"

