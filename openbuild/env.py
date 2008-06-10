# (C) Ardendo SE 2008
#
# Released under the LGPL

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
from sets import Set

class Environment( BaseEnvironment ):

	"""Base environment for Ardendo AML/AMF and related builds."""
	
	vs_builder = None 
	if utils.vs() : vs_builder = vsbuild.VsBuilder( utils.vs() )
	
	already_installed = {}
	
	def __init__( self, opts, bmgr = None , **kw ):
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
		
		BaseEnvironment.__init__( self, ENV = os.environ , **kw )
		
		opts.Update( self )
		opts.Save( opts.file, self )
		
		# Check if we need to override default build behaviour.
		if bmgr : self.build_manager = bmgr
		elif utils.vs() : self.build_manager = Environment.vs_builder
		else: self.build_manager = None
		
		self.dependencies = {}

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
		self[ 'debug' ] = '1'
		self.debug = True
		self[ 'build_prefix' ] = '$debug_prefix'
		self[ 'stage_prefix' ] = utils.install_target( self )

		if self[ 'PLATFORM' ] == 'posix':
			self.Append( CCFLAGS = [ '-Wall', '-ggdb', '-O0' ] )
		elif self[ 'PLATFORM' ] == 'darwin':
			self.Append( CCFLAGS = [ '-Wall', '-gdwarf-2', '-O0' ] )
		elif self[ 'PLATFORM' ] == 'win32':
			self.Append( CPPDEFINES = [ '_WINDOWS', 'DEBUG', '_DEBUG'])
			# /W3 = Warning Level 3, 
			# /Od = Optimization Disabled , 
			# /GR = Enable Runtime Type Information, 
			# /MDd Multi Threaded Debug Dll Runtime
			self.Append( CCFLAGS = [ '/W3', '/Od', '/GR', '/EHsc', '/MDd' ] )
			self.Append( LINKFLAGS = [ '/DEBUG' ] )
		else:
			raise( 'Unknown platform: %s', self[ 'PLATFORM' ] )
	
	def prep_release( self ):
		self[ 'debug' ] = '0'
		self.debug = False
		self[ 'build_prefix' ] = '$release_prefix'
		self[ 'stage_prefix' ] = utils.install_target( self )

		if self[ 'PLATFORM' ] == 'posix':
			self.Append( CCFLAGS = [ '-Wall', '-O3', '-pipe' ] )
		elif self[ 'PLATFORM' ] == 'darwin':
			self.Append( CCFLAGS = [ '-Wall', '-O3', '-pipe' ] )
		elif self[ 'PLATFORM' ] == 'win32':
			self.Append( CPPDEFINES = [ '_WINDOWS', 'NDEBUG'])
			# /W3 = Warning Level 3, 
			# /O2 = Optimize For Speed, 
			# /GR = Enable Runtime Type Information, 
			# /EHsc Enable C++ Exceptions
			# /MD Multithreaded DLL runtime
			self.Append( CCFLAGS = [ '/W3', '/O2', '/EHsc', '/GR', '/MD' ] )
		else:
			raise( 'Unknown platform: %s', self[ 'PLATFORM' ] )

	def optional( self, *packages ):
		return self.package_manager.optional( self, *packages )

	def packages( self, *packages ) :
		self.package_manager.packages( self, *packages )

	def package_cflags( self, package ) :
		return self.package_manager.package_cflags( self, package )

	def package_libs( self, package ) :
		return self.package_manager.package_libs( self, package )

	def build_types( self ):
		builds = [ Environment.prep_release, Environment.prep_debug ]
		if self.release_install: builds.pop( 1 )
		elif self.debug_install: builds.pop( 0 )
		return builds

	def package_install( self ):
		if self[ 'PLATFORM' ] == 'win32': return

		for build_type in self.build_types( ):
			env = self.Clone( )
			build_type( env )
			for lib in env.package_manager.package_install_libs( env ):
				if os.path.isdir( lib ):
					env.install_dir( os.path.join( env[ 'stage_libdir' ], lib.rsplit( os.sep, 1 )[ -1 ] ), lib )
				elif not os.path.islink( lib ):
					env.Install( env[ 'stage_libdir' ], lib )
				else:
					# TODO: Fix sym link installs 
					pass
			for include in env.package_manager.package_install_include( env ):
				if os.path.isdir( include ):
					env.install_dir( os.path.join( env[ 'stage_include' ], include.rsplit( os.sep, 1 )[ -1 ] ), include )
				elif not os.path.islink( include ):
					env.Install( env[ 'stage_include' ], include )
				else:
					# TODO: Fix sym link installs 
					pass

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
		
	def requires( self, *packages ) :
		if 'requires' in dir( self.package_manager) : 
			self.package_manager.requires(self, *packages)
		
	def add_dependencies( self, result, dep , build_type) :
		if result[build_type] == None : return
		res_min = str(result[ build_type ][ 0 ])
		dep_min = str(dep[ build_type ][ 0 ]) 
		
		res_min = res_min[ res_min.rfind("\\") + 1 : res_min.rfind(".dll") ]
		dep_min = dep_min[ dep_min.rfind("\\") + 1 : dep_min.rfind(".dll") ]
		
		existing_deps = self.dependencies.get( res_min, None)
		if existing_deps == None :
			new_set = Set()
			new_set.add( dep_min )
			self.dependencies[ res_min ] = new_set
		else :
			existing_deps.add( dep_min )
		
		
	def build( self, path, deps = [], tools = [] ):
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
			local_env = self.Clone( tools = tools )
			local_env.full_path = os.path.join( local_env.root, path )
			local_env.relative_path = path
			
			build_type( local_env )

			result[ build_type ] = local_env.SConscript( [ os.path.join( path, 'SConscript' ) ], 
															build_dir=os.path.join( local_env[ 'build_prefix' ], 'tmp', path ), 
															duplicate=0, exports=[ 'local_env' ] )

			for dep in deps:
				if dep is not None:
					self.add_dependencies(result, dep, build_type)
						
					self.Requires( result[ build_type ], dep[ build_type ] )
					file = str( dep[ build_type ][ 0 ] )
					libpath, lib = file.rsplit( os.sep, 1 )
					libpath = os.path.join( self.root, libpath )
					lib = lib.rsplit( '.', 1 )[ 0 ]
					local_env.Append( LIBPATH = [libpath] )
					local_env.Append( LIBS = [lib] )
					
					# if self[ 'PLATFORM' ] != 'win32': 
						# libpath = '-L' + libpath
						# lib = lib.replace( 'lib', '-l', 1 )
					# else:
						# local_env.Append( LIBPATH = [libpath] )
						# local_env.Append( LIBS = [lib + ".lib"] )
						# libpath = '/LIBPATH:' + libpath
						# lib += '.lib'
					# local_env.Append( LINKFLAGS = [ libpath, lib ] )

		return result
		
	def setup_precompiled_headers( self, sources, pre = None , nopre = None ) :
		if nopre is not None: 
			sources.extend(self.Object(nopre, PCH=None, PCHSTOP=None))

		if self[ 'PLATFORM' ] == 'win32' and pre is not None:
			if len(pre) != 2 : raise SCons.Errors.UserError, "The pre varaible must be a tuple of (cpp-file, hpp-file)"
			self.Append( PCHSTOP = pre[1].replace("/", os.sep ), PCH = self.PCH(pre[0])[0] )
			if pre[0] not in sources: sources += [ pre[0] ]

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

		if "program" in dir(self.build_manager) : 
			return self.build_manager.program( self, lib, sources, headers, pre, nopre, *keywords )
			
		self.setup_precompiled_headers( sources, pre, nopre )
		self['PDB'] = lib + '.pdb'
		return self.Program( lib, sources, *keywords )
		
	def console_program( self, lib, sources, headers=None, pre=None, nopre=None, *keywords ):

		if "console_program" in dir(self.build_manager) : 
			return self.build_manager.console_program( self, lib, sources, headers, pre, nopre, *keywords )
			
		self.setup_precompiled_headers( sources, pre, nopre )
		self['PDB'] = lib + '.pdb'
		
		if self[ 'PLATFORM' ] == 'win32':
			self.Append( LINKFLAGS="/SUBSYSTEM:CONSOLE" )
		
		return self.Program( lib, sources, *keywords )
		
	def done( self, project_name = None ) :
		if "done" in dir(self.build_manager) : 
			self.build_manager.done( self, project_name )

	def Tool(self, tool, toolpath=None, **kw):
		if toolpath == None :
			toolpath = [ self.path_to_openbuild_tools() ]
		
		BaseEnvironment.Tool( self, tool, toolpath, **kw )

	def release( self, *kw ):
		for list in kw:
			for file in list:
				name = str( file )
				target = ''
				if name.endswith( '.dll' ):
					target = self[ 'stage_bin' ]
				elif name.endswith( '.exe' ):
					target = self[ 'stage_bin' ]
				elif name.endswith( '.pdb' ):
					target = self[ 'stage_bin' ]
				elif name.endswith( '.lib' ):
					target = self[ 'stage_libdir' ]
				elif name.endswith( '.so' ):
					target = self[ 'stage_libdir' ]
				elif name.endswith( '.dylib' ):
					target = self[ 'stage_libdir' ]
				elif name.endswith( '.exp' ):
					pass
				else:
					target = self[ 'stage_bin' ]
				if target != '':
					full = self.subst( os.path.join( target, name ) )
					if full not in Environment.already_installed:
						self.Install( target, name )
						Environment.already_installed[name] = [full]

	def install_dir( self, dst, src ):
		""" Installs the contents of src to dst, walking through the src directory and invoking 
			Install on every file found.

			Keyword arguments:
			dst -- destination directory
			src -- source directory
		"""

		dst = self.subst( utils.clean_path( dst ) )
		src = self.subst( utils.clean_path( src ) )

		if dst in Environment.already_installed.keys() :
			if src in Environment.already_installed[dst] : return
			else : Environment.already_installed[dst].append(src)
		else: Environment.already_installed[dst] = [src]
		
		for root, d, files in os.walk( src ):
			for f in files:
				full = os.path.join( root, f )
				# Only include if there's no hidden content here (ie: .svn existing in the path)
				if full.find( os.sep + '.' ) == -1:
					target_dir = dst + root.replace( src, '' )
					target_file = utils.clean_path( os.path.join( target_dir, f ) )
					if target_file not in Environment.already_installed:
						self.Install( target_dir, full )
						Environment.already_installed[target_file] = [full]
					elif Environment.already_installed[target_file] != [full]:
						print "Warning: trying to copy %s to %s, but %s is already there." % ( full, target_file, Environment.already_installed[target_file][ 0 ] )

	def generate_tester(source, target, env, for_signature):
		
		return '$SOURCE all_tests --log_level=all > $TARGET || (cat $TARGET; exit 1)'

		testrunner = Builder(generator = generate_tester)
		self.Append( BUILDERS = {'TestRunner': testrunner} )
	
	def run_test( self, *args ) :
		if "run_test" in dir(self.build_manager) : 
			return self.build_manager.run_test( self, *args )
		
		return self.TestRunner(*args)

