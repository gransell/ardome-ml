import os
import sys
from SCons.Script.SConscript import SConsEnvironment as BaseEnvironment
import SCons.Tool
import SCons.Script
import utils
if utils.vs( ):
	import vsbuild
from pkgconfig import PkgConfig as PkgConfig
from winconfig import WinConfig as WinConfig

# if os.name == 'posix':
#	import pkgconfig as package_manager
#else:
#	import winconfig as package_manager

class Environment( BaseEnvironment ):

	"""Base environment for Ardendo AML/AMF and related builds."""

	def __init__( self, opts, bmgr = None , *kw ):
		"""	Constructor. The Options object is added to the environment, and all 
			common install and build related variables are defined as needed.
			
			Keyword arguments:
			opts -- 
			bmrg --  A possible build manager. The build manager can override functions
					in this class if required. The functions are build, shared_library 
					and plugin.
			kw -- The rest of the passed parameters to this constructor.
			"""
			
		self.toolpath = [ self.path_to_openbuild_tools() ]
		self.options = opts
		
		BaseEnvironment.__init__( self, ENV = os.environ ,*kw )
		
		opts.Update( self )
		opts.Save( opts.file, self )
		
		# Check if we need to override default build behaviour.
		if bmgr : self.build_manager = bmgr
		elif utils.vs() : self.build_manager = vsbuild.VsBuilder( utils.vs() )
		else: self.build_manager = None

		# Instanciate a package manager for the current platform:
		if os.name == 'posix' : self.package_manager = PkgConfig( )
		else : self.package_manager = WinConfig( )
	
		SCons.Script.Help( opts.GenerateHelpText( self ) )

		self[ 'debug_prefix' ] = os.path.join( 'build', 'debug', self[ 'target' ] )
		self[ 'release_prefix' ] = os.path.join( 'build', 'release', self[ 'target' ] )
		self[ 'libdir' ] = [ 'lib', 'lib64' ][ utils.arch( ) == 'x86_64' ]
		self[ 'stage_include' ] = os.path.join( '$stage_prefix', 'include' )
		self[ 'stage_libdir' ] = os.path.join( '$stage_prefix', '$libdir' )
		self[ 'stage_pkgconfig' ] = os.path.join( '$stage_libdir', 'pkgconfig' )
		self[ 'stage_bin' ] = os.path.join( '$stage_prefix', 'bin' )

		self.root = os.path.abspath( '.' )
		self.package_list = self.package_manager.walk( self )

		self.release_install = 'install' in sys.argv
		self.debug_install = 'debug-install' in sys.argv
		
		self.Alias( 'install', self[ 'distdir' ] + self[ 'prefix' ] )
		self.Alias( 'debug-install', self[ 'distdir' ] + self[ 'prefix' ] )

		self.Builder( action = '$RCCOM', suffix = '.rc' )

	def path_to_openbuild( self ) :
		return os.path.split(__file__)[0]
		
	def path_to_openbuild_tools( self ) :
		return os.path.join( self.path_to_openbuild(), "Tools")

	def prep_debug( self ):
		self[ 'debug' ] = '0'
		self[ 'build_prefix' ] = '$debug_prefix'
		self[ 'stage_prefix' ] = utils.install_target( self )

		if self[ 'PLATFORM' ] == 'posix':
			self.Append( CCFLAGS = [ '-Wall', '-ggdb', '-O0' ] )
		elif self[ 'PLATFORM' ] == 'darwin':
			self.Append( CCFLAGS = [ '-Wall', '-gdwarf-2', '-O0' ] )
		elif self[ 'PLATFORM' ] == 'win32':
			self.Append( CCFLAGS = [ '/W3', '/DEBUG', '/Od' ] )
		else:
			raise( 'Unknown platform: %s', self[ 'PLATFORM' ] )
	
	def prep_release( self ):
		self[ 'debug' ] = '1'
		self[ 'build_prefix' ] = '$release_prefix'
		self[ 'stage_prefix' ] = utils.install_target( self )

		if self[ 'PLATFORM' ] == 'posix':
			self.Append( CCFLAGS = [ '-Wall', '-O3', '-pipe' ] )
		elif self[ 'PLATFORM' ] == 'darwin':
			self.Append( CCFLAGS = [ '-Wall', '-O3', '-pipe' ] )
		elif self[ 'PLATFORM' ] == 'win32':
			self.Append( CCFLAGS = [ '/W3', '/O2', '/EHsc' ] )
		else:
			raise( 'Unknown platform: %s', self[ 'PLATFORM' ] )

	def packages( self, *packages ) :
		self.package_manager.packages( self, *packages )

	def package_cflags( self, package ) :
		return self.package_manager.package_cflags( self, package )

	def package_libs( self, package ) :
		return self.package_manager.package_libs( self, package )

	def check_dependencies( self, *packages ):
		"""Ensure that all the listed packages are found."""
		result = True
		for package in packages:
			try:
				temp = self.Clone( )
				temp.prep_release( )
				temp.package_manager.packages( temp, package )
			except OSError, e:
				result = False
				print "Dependency check" + str( e )
		return result
		
	def build( self, path, deps = [] ):
		"""	Invokes a SConscript, cloning the environment and linking against any inter
			project dependencies specified.
			
			Can  be overridden by a Visual Studio build_manager or an external
			build_manager.
			
			Keyword arguments:
			path -- The path to the SConscipt file that shold be parsed
			deps -- Any dependencies built by another Environment.build call
			returns a dictinary: buildtype ('debug'/'release') -> SCons representation of the generated
					result of the SConscript."""
			

		if "build" in dir(self.build_manager) : 
			return self.build_manager.build( self, path, deps )
			
		result = { }

		builds = [ Environment.prep_release, Environment.prep_debug ]
		if self.release_install: builds.pop( 1 )
		elif self.debug_install: builds.pop( 0 )

		for build_type in builds:
			local_env = self.Clone(  )
			build_type( local_env )

			result[ build_type ] = local_env.SConscript( [ os.path.join( path, 'SConscript' ) ], 
															build_dir=os.path.join( local_env[ 'build_prefix' ], path ), 
															duplicate=0, exports=[ 'local_env' ] )

			for dep in deps:
				if dep is not None:
					self.Requires( result[ build_type ], dep[ build_type ] )
					file = str( dep[ build_type ][ 0 ] )
					libpath, lib = file.rsplit( os.sep, 1 )
					libpath = os.path.join( self.root, libpath )
					lib = lib.rsplit( '.', 1 )[ 0 ]
					if self[ 'PLATFORM' ] != 'win32': 
						libpath = '-L' + libpath
						lib = lib.replace( 'lib', '-l', 1 )
					else:
						libpath = '/LIBPATH:' + libpath
						lib += '.lib'
					local_env.Append( LINKFLAGS = [ libpath, lib ] )

		return result
		
	def setup_precompiled_headers( self, sources, pre = None , nopre = None ) :
		if nopre is not None: 
			sources.extend(self.Object(nopre, PCH=None, PCHSTOP=None))

		if self[ 'PLATFORM' ] == 'win32' and pre is not None:
			if len(pre) != 2 : raise SCons.Errors.UserError, "The pre varaible must be a tuple of (cpp-file, hpp-file)"
			self.Append( PCHSTOP = pre[1], PCH = self.PCH(pre[0])[0] )

	def shared_library( self, lib, sources, headers=None, pre=None, nopre=None, *keywords ):
		"""	Build a shared library ( dll or so )
			
			Keyword arguments:
			lib -- The name of the library to build
			sources -- A list of c++ source files
			headers -- A list of c++ header files, not needed for the build, but could be used 
						by certain build_managers that create IDE files.
			pre -- A tuple with two elements, the first is the name of the header file that is 
					included in all source-files using precompiled headers, the second is the
					name of the actual source file creating the precompiled header file itself.
			nopre -- If pre is set, this parameter contains a list of source files (that should
						be present in sources) that will not use precompiled headers. These files
						do not include the header file in pre[0].
			keywords -- All other parameters passed with keyword assignment.
		"""
	
		if "shared_library" in dir(self.build_manager) : 
			return self.build_manager.shared_library( self, lib, sources, headers, pre, nopre, *keywords )
		
		if self[ 'PLATFORM' ] == 'darwin':
			self.Append( LINKFLAGS = [ '-Wl,-install_name', '-Wl,%s/lib%s.dylib' % ( self[ 'install_name' ], lib ) ] )

		self.setup_precompiled_headers( sources, pre, nopre )
		
		self['PDB'] = lib + '.pdb'
		
		return self.SharedLibrary( lib, sources, *keywords )
		
	def plugin( self, lib, sources, headers=None, pre=None, nopre=None, *keywords ):
		"""	Build a plugin. See shared_library in this class for a detailed description. """
		
		if "plugin" in dir(self.build_manager) : 
			return self.build_manager.plugin( self, lib, sources, headers, pre, nopre, *keywords )
		
		self.setup_precompiled_headers( sources, pre, nopre )
		
		self['PDB'] = lib + '.pdb'

		return self.SharedLibrary( lib, sources, *keywords )

	def program( self, lib, sources, headers=None, pre=None, nopre=None, *keywords ):

		self.setup_precompiled_headers( sources, pre, nopre )
		self['PDB'] = lib + '.pdb'
		return self.Program( lib, sources, *keywords )

	def Tool(self, tool, toolpath=None, **kw):
		if toolpath == None :
			toolpath = [ self.path_to_openbuild_tools() ]
		
		BaseEnvironment.Tool( self, tool, toolpath, **kw )
	


