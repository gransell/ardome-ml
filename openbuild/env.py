# (C) Ardendo SE 2008
#
# Released under the LGPL

import os
import sys
import shutil
from SCons.Script.SConscript import SConsEnvironment as BaseEnvironment
import SCons.Tool
import SCons.Script
import utils

if utils.vs( ):
	import vsbuild
if utils.xcode():
	from xcode import XcodeBuilder
	
from pkgconfig import PkgConfig as PkgConfig
from winconfig import WinConfig as WinConfig
from sets import Set
import types

class Environment( BaseEnvironment ):

	"""Base environment for Ardendo AML/AMF and related builds."""
	
	vs_builder = None 
	xcode_builder = None
	if utils.vs() : vs_builder = vsbuild.VsBuilder( utils.vs() )
	if utils.xcode() : xcode_builder = XcodeBuilder()
	
	already_installed = {}
	bundle_libraries = [ [], [] ]
	bundle_resources = [ [], [] ]
	
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
		
		self.root = os.path.abspath( '.' )
			
		self.toolpath = [ utils.path_to_openbuild_tools() ]
		self.options = opts
		
		BaseEnvironment.__init__( self, ENV = os.environ , **kw )
		
		opts.Update( self )
		# Remove the vs option if it is not given on the command-line.
		if utils.vs() == None and 'vs' in self._dict.keys( ): del self._dict['vs'] 
		opts.Save( opts.file, self )
		
		# Override the default scons hash based checking
		self.Decider( 'MD5-timestamp' )
		self.SetOption('implicit_cache', 1)
		self.SetOption('max_drift', 1)

		# Check if we need to override default build behaviour.
		if bmgr : self.build_manager = bmgr
		elif utils.vs() : 
			self.build_manager = Environment.vs_builder
			self['target'] = utils.vs()
		elif utils.xcode() :
			self.build_manager = Environment.xcode_builder
		else: 
			self.build_manager = None
		
		self.dependencies = {}

		# Instanciate a package manager for the current platform:
		if os.name == 'posix' : self.package_manager = PkgConfig( )
		else : self.package_manager = WinConfig( )
	
		SCons.Script.Help( opts.GenerateHelpText( self ) )

		self[ 'debug_prefix' ] = os.path.join( 'build', 'debug', self[ 'target' ] )
		self[ 'release_prefix' ] = os.path.join( 'build', 'release', self[ 'target' ] )
		#self[ 'libdir' ] = [ 'lib', 'lib64' ][ utils.arch( ) == 'x86_64' ]
		self[ 'libdir' ] = 'lib'
		self[ 'stage_include' ] = os.path.join( '$stage_prefix', 'include' )
		self[ 'stage_libdir' ] = os.path.join( '$stage_prefix', '$libdir' )
		self[ 'stage_pkgconfig' ] = os.path.join( '$stage_libdir', 'pkgconfig' )
		self[ 'stage_bin' ] = os.path.join( '$stage_prefix', 'bin' )
		self[ 'stage_frameworks' ] = os.path.join( '$stage_prefix', 'Frameworks' )
		self[ 'stage_share' ] = os.path.join( '$stage_prefix', 'share' )
		self[ 'stage_resources' ] = os.path.join( '$stage_prefix', 'Resources' )

		self.release_install = 'install' in sys.argv
		self.debug_install = 'debug-install' in sys.argv
		
		self.full_install = 'full_install' in sys.argv
		
		self.Alias( 'install', self[ 'distdir' ] + self[ 'prefix' ] )
		self.Alias( 'debug-install', self[ 'distdir' ] + self[ 'prefix' ] )
		self.Alias( 'full-install', self[ 'distdir' ] + self[ 'prefix' ] )
		
		self.Alias( 'debug', self['debug_prefix'] )
		self.Alias( 'release', self['release_prefix'] )

		self[ 'win_target_path' ] = '$stage_bin'

		self.Builder( action = '$RCCOM', suffix = '.rc' )

		self.install_packages( )
		self.package_list = self.package_manager.walk( self )

	def install_config( self, src, dst ):
		"""	Installs config files for bcomp usage.

			Keyword arguments:
			src -- source file (normally located in the config/[target])
			dst -- destination package (eg: bcomp/boost)
		"""

		src = src.replace( '/', os.sep )
		dst = dst.replace( '/', os.sep )
		if os.path.exists( src ):
			src_file = src.rsplit( os.sep, 1 )[ -1 ]
			if os.path.exists( dst ):
				dst_dir = None
				if src.endswith( '.wc' ): dst_dir = os.path.join( dst, self[ 'libdir' ], 'winconfig' )
				if src.endswith( '.pc' ): dst_dir = os.path.join( dst, self[ 'libdir' ], 'pkgconfig' )
				if dst_dir is not None:
					dst_file = os.path.join( dst_dir, src_file )
					if not os.path.exists( dst_dir ): os.makedirs( dst_dir )
					shutil.copyfile( src, dst_file )
				else:
					raise Exception, "Unable to identify %s as a win or pkg config file" % src
			else:
				raise Exception, "Unable to locate %s" % dst
		else:
			raise Exception, "Unable to locate %s" % src
		
	def temporary_build_path( self, target ) :
		""" Where will the given target be built by SCons """
		return os.path.join( self.root, self.subst( self[ 'build_prefix' ] ), 'tmp' , self.relative_path_to_sconscript )

	def qt_build_path( self, target ) :
		""" Where will the given target be built by SCons """
		rel_path = self.relative_path_to_sconscript
		path = os.path.join( self.root, rel_path, 'qt', [ 'release', 'debug' ][ int( self.debug ) ] )
		utils.ensure_output_path_exists( path )
		return path
		
	def prep_debug( self ):
		"""	Prepare the environment for debug use - provides some hard coded debug and link flags."""

		self[ 'debug' ] = '1'
		self.debug = True
		self[ 'build_prefix' ] = '$debug_prefix'
		self[ 'stage_prefix' ] = utils.install_target( self )

		if self[ 'PLATFORM' ] == 'posix':
			self.Append( CCFLAGS = [ '-Wall', '-ggdb', '-O0' ] )
			self.Append( CPPDEFINES = [ '_LARGEFILE_SOURCE' ] )
			self.Append( CPPDEFINES = [ '_FILE_OFFSET_BITS=64' ] )
		elif self[ 'PLATFORM' ] == 'darwin':
			self.Append( CCFLAGS = [ '-Wall', '-gdwarf-2', '-O0' ] )
		elif self[ 'PLATFORM' ] == 'win32':
			self.Append( CPPDEFINES = [ 'WIN32','_WINDOWS', 'DEBUG', '_DEBUG'])
			# /W3 = Warning Level 3, 
			# /Od = Optimization Disabled , 
			# /GR = Enable Runtime Type Information, 
			# /MDd Multi Threaded Debug Dll Runtime
			self.Append( CCFLAGS = [ '/W3', '/Od', '/GR', '/EHsc', '/MDd' ] )
			self.Append( LINKFLAGS = [ '/DEBUG' ] )
		else:
			raise( 'Unknown platform: %s', self[ 'PLATFORM' ] )
	
	def prep_release( self ):
		"""	Prepare the environment for release use - provides some hard coded debug and link flags."""

		self[ 'debug' ] = '0'
		self.debug = False
		self[ 'build_prefix' ] = '$release_prefix'
		self[ 'stage_prefix' ] = utils.install_target( self )

		if self[ 'PLATFORM' ] == 'posix':
			self.Append( CCFLAGS = [ '-Wall', '-O3', '-pipe' ] )
			self.Append( LINKFLAGS = [ '-s' ] )
			self.Append( CPPDEFINES = [ '_LARGEFILE_SOURCE' ] )
			self.Append( CPPDEFINES = [ '_FILE_OFFSET_BITS=64' ] )
		elif self[ 'PLATFORM' ] == 'darwin':
			self.Append( CCFLAGS = [ '-Wall', '-O3', '-pipe' ] )
		elif self[ 'PLATFORM' ] == 'win32':
			self.Append( CPPDEFINES = [ 'WIN32', '_WINDOWS', 'NDEBUG'])
			# /W3 = Warning Level 3, 
			# /O2 = Optimize For Speed, 
			# /GR = Enable Runtime Type Information, 
			# /EHsc Enable C++ Exceptions
			# /MD Multithreaded DLL runtime
			self.Append( CCFLAGS = [ '/W3', '/O2', '/EHsc', '/GR', '/MD' ] )
		else:
			raise( 'Unknown platform: %s', self[ 'PLATFORM' ] )

	def optional( self, *packages ):
		"""Tries to locate optional packages listed.

		Keyword arguments:
		packages - the list of package names.

		Returns:
		a dictionary of 'have_' + package = bool"""

		return self.package_manager.optional( self, *packages )

	def packages( self, *packages ) :
		"""Tries to locate packages - any not found will result in an exception.

		TODO: Handle exception gracefully and normalise for the platforms.

		Keyword arguments:
		packages - the list of package names."""

		self.package_manager.packages( self, *packages )

	def build_types( self ):
		"""	Returns the prep methods which are relevant to the current build."""

		builds = [ Environment.prep_release, Environment.prep_debug ]
		if self.release_install: builds.pop( 1 )
		elif self.debug_install: builds.pop( 0 )
		return builds

	def package_install( self ):
		"""	Installs includes, libs and frameworks listed in .pc files. """

		if self[ 'PLATFORM' ] == 'win32': return

		for build_type in self.build_types( ):
			env = self.Clone( )
			build_type( env )
			for dest, rule in [ ( 'stage_include', 'install_include' ), 
								( 'stage_libdir', 'install_libs' ), 
								( 'stage_frameworks', 'install_frameworks' ),
								( 'stage_bin', 'install_bin' ),
								( 'stage_resources', 'install_resources' ),
								( 'stage_share', 'install_share' ) ]:
				list = env.package_manager.package_install_list( env, rule )
				for key in list.keys( ):
					if key != '':
						env.copy_files( os.path.join( env[ dest ], key ), list[ key ] )
					else:
						env.copy_files( env[ dest ], list[ key ] )

	def check_dependencies( self, *packages ):
		"""Ensure that all the listed packages are found.

		Keyword arguments:
		packages -- the list of packages to check.
		"""

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
		"""	Method for .wc files to use for package requests.

		Keyword arguments:
		packages -- the list of packages to check.
		"""

		if 'requires' in dir( self.package_manager) : 
			self.package_manager.requires(self, *packages)
		
	def add_dependencies( self, result, dep , build_type) :
		"""	MATS: Unsure about this method - windows only? (dll and bash slash hard coding)."""
		
		if result[build_type] is None : return
		if dep[build_type] == None : return
		res_min = str(result[ build_type ][ 0 ])
		dep_min = str(dep[ build_type ][ 0 ])
		
		if self['PLATFORM'] == 'win32':
			res_min = res_min[ res_min.rfind("\\") + 1 : res_min.rfind(".dll") ]
			dep_min = dep_min[ dep_min.rfind("\\") + 1 : dep_min.rfind(".dll") ]
		
		existing_deps = self.dependencies.get( res_min, None)
		if existing_deps == None :
			new_set = Set()
			new_set.add( dep_min )
			self.dependencies[ res_min ] = new_set
		else :
			existing_deps.add( dep_min )
		
	def move_from_path_to_flags( self, paths, switch ):
		result= [ ]
		bcomp_dir = [ '#/bcomp', 'bcomp', os.path.join( self.root, 'bcomp' ), '/usr/local' ]
		for incdir in paths:
			found = False
			for bcomp in bcomp_dir:
				if incdir.startswith( bcomp ):
					found = True
					break
			if found:
				result += [ switch + incdir ]
				paths.remove( incdir )
		return result

	def build( self, path, deps = [], tools = [], externals = [] ):
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
		
		self.build_deps = deps
		self.build_exts = externals

		# Keep this path in the environment, is used by for instance vsbuild.
		self.relative_path_to_sconscript = path
		
		result = { }

		for build_type in self.build_types( ):
			local_env = self.Clone( tools = tools, LIBPATH = [ ] )
			local_env.full_path = os.path.join( local_env.root, path )
			local_env.relative_path = path
			
			build_type( local_env )

			build_dir = os.path.join( self.root, local_env.subst( local_env[ 'build_prefix' ] ), 'tmp', path )
			if not os.path.exists( build_dir ): os.makedirs( build_dir )

			result[ build_type ] = local_env.SConscript( [ os.path.join( path, 'SConscript' ) ], 
															build_dir=build_dir, 
															duplicate=0, exports=[ 'local_env' ] )

			for dep in deps:
				if dep is not None and dep[ build_type ] is not None:
					self.add_dependencies(result, dep, build_type)
						
					self.Depends( result[ build_type ], dep[ build_type ] )
					file = str( dep[ build_type ][ 0 ] )
					libpath, lib = file.rsplit( os.sep, 1 )
					libpath = os.path.join( self.root, libpath )
					lib = lib.rsplit( '.', 1 )[ 0 ]
					local_env.Append( LIBPATH = [libpath] )
					local_env.Append( LIBS = [lib] )
			
			for dep in externals:
				if dep is not None and dep[ build_type ] is not None:
					self.add_dependencies(result, dep, build_type)
					self.Depends( result[ build_type ], dep[ build_type ] )

			switch = [ '/I', '-I' ][ int( self[ 'PLATFORM' ] != 'win32' ) ]
			if local_env.has_key('CPPPATH') :
				paths = local_env[ 'CPPPATH' ]
				local_env.Append( CPPFLAGS = local_env.move_from_path_to_flags( paths, switch ) )
				local_env.Replace( CPPPATH = paths )

			switch = [ '/LIBPATH:', '-L' ][ int( self[ 'PLATFORM' ] != 'win32' ) ]
			if local_env.has_key('LIBPATH') :
				paths = local_env[ 'LIBPATH' ]
				local_env.Append( LINKFLAGS = local_env.move_from_path_to_flags( paths, switch ) )
				local_env.Replace( LIBPATH = paths )

		return result
		
	def create_string_from_construction_variable( self, env_var_name) :
		if not self.has_key(env_var_name) : 
			return ""
		res = ''
		#print 'env_var_name=', env_var_name, ', env[env_var_name]=', env[env_var_name]
		if isinstance(self[env_var_name], list) :
			for p in self[env_var_name] :
				res += p + ' ' 
		else :
			res += str( self[env_var_name] ) + ' '
				
		return res
		
	def setup_clr_compilation( self ) :
		ccflags = self.create_string_from_construction_variable('CPPFLAGS')
		ccflags += self.create_string_from_construction_variable('CCFLAGS')
		if '/clr' in ccflags:
			ccflags = ccflags.replace('/EHsc', '')
			ccflags += ' /EHs- /EHa '
			self.Replace( CCFLAGS = ccflags )
			self.Replace( CPPFLAGS = '' )
			
	def setup_precompiled_headers( self, sources, pre = None , nopre = None ) :
		"""	Adds recompiled headers to the environment for this build.

		Keyword arguments:
		sources -- the list of source file objects
		pre -- the tuple of ( cpp-file, hpp-file )
		nopre -- a list of files which precompiled headers are not applied to
		"""

		if nopre is not None: 
			for f in nopre:
				sources.extend(self.Object(f, PCH=None, PCHSTOP=None))

		if self[ 'PLATFORM' ] == 'win32' and pre is not None:
			if len(pre) != 2 : raise SCons.Errors.UserError, "The pre varaible must be a tuple of (cpp-file, hpp-file)"
			self.Append( PCHSTOP = pre[1].replace("/", os.sep ), PCH = self.PCH(pre[0])[0] )
			if pre[0] not in sources: sources += [ pre[0] ]
	
	def shared_object(self, sources, **keywords ):
		"""	Generate object files from the provided sources
		
			Arguments:
			sources -- The source files to compile into hared objects
			keywords -- All other parameters passed with keyword assignment.
		"""
		if "shared_object" in dir(self.build_manager):
			return self.build_manager.shared_object( self, sources, **keywords )
		
		if self[ 'PLATFORM' ] == 'win32':
			self.setup_clr_compilation()
		return self.SharedObject( sources, **keywords )

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
			self.Append( LINKFLAGS = [ '-Wl,-install_name', '-Wl,%slib%s.dylib' % ( self[ 'install_name' ], lib ) ] )

		self.setup_precompiled_headers( sources, pre, nopre )
	
		if self[ 'PLATFORM' ] == 'win32':
			self['PDB'] = lib + '.pdb'
			self.setup_clr_compilation()

		new_lib = self.SharedLibrary( lib, sources, *keywords )
		return new_lib
		
#	def framework( self, fmwk, sources, headers=None, public_headers=None, resources=None, pre=None, nopre=None, *keywords):
#		"""	Build a Framework (darwin only)
#			
#			Keyword arguments:
#			fmwk -- The name of the framework to build
#			sources -- A list of source files
#			headers -- A list of header files, not needed for the build, but could be used 
#						by certain build_managers that create IDE files.
#			public_headers -- Alist of header files that will be public in the framework
#			resources -- A list of resources that will be included in the framework
#			pre -- A tuple with two elements, the first is the name of the header file that is 
#					included in all source-files using precompiled headers, the second is the
#					name of the actual source file creating the precompiled header file itself.
#			nopre -- If pre is set, this parameter contains a list of source files (that should
#						be present in sources) that will not use precompiled headers. These files
#						do not include the header file in pre[0].
#			keywords -- All other parameters passed with keyword assignment.
#		"""
#
#		if self['PLATFORM'] != 'darwin':
#			raise Exception, "Framwork builds only supported on darwin."
#        
#		framework_name = '%s.framework' % fmwk
#		
#		if "shared_library" in dir(self.build_manager) :
#			return self.build_manager.framework( self, fmwk, sources, headers, public_headers, resources, pre, nopre, *keywords )
#
#		# self.Append( LINKFLAGS = [ '-Wl,-install_name', '-Wl,%s/%s/Versions/A/%s' % ( self[ 'install_name' ], framework_name, fmwk, fmwk ) ] )
#
#		self.setup_precompiled_headers( sources, pre, nopre )
#
#		loadable_module_obj = self.LoadableModule( target = fmwk, source = sources )
#
#		self.Install( framework_name + '/Versions/A', loadable_module_obj )
#		self.Install( framework_name +'/Versions/A/Headers', public_headers )
#		self.Install( framework_name +'/Versions/A/Resources', resources )


	def framework( self, fmwk_name, sources=None, headers=[], info_plist = None, resources = None, extra_libs = None, pre=None, nopre=None, *keywords ):
		import openbuild.Tools.mac
		openbuild.Tools.mac.TOOL_BUNDLE( self )

		libs = Environment.bundle_libraries[ int( self.debug ) ]
		bundle_resources = Environment.bundle_resources[ int( self.debug ) ]
		build = [ Environment.prep_release, Environment.prep_debug ][ int( self.debug ) ]

		self.Append( LINKFLAGS = [ '-Wl,-install_name', '-Wl,@loader_path/../Frameworks/%s.framework/%s' % (fmwk_name, fmwk_name) ] )

		lib = None
		for item in self.build_deps:
			if item[ build ] is not None:
				for file in item[ build ]:
					libs += [ [ '', file ] ]
		if sources is not None:
			self[ 'SHLIBPREFIX' ] = ''
			self[ 'SHLIBSUFFIX' ] = ''
			lib = self.SharedLibrary( fmwk_name, sources, *keywords )[0]
			
		for item in self.build_exts:
			if item[ build ] is not None:
				for file in item[ build ]:
					libs += [ [ '', file ] ]
		
		if resources is not None :
			bundle_resources.extend(resources)
		
		return self.MakeBundle( fmwk_name, lib, libs, headers, info_plist, resources=bundle_resources, *keywords )

	def plugin( self, lib, sources, headers=None, pre=None, nopre=None, *keywords ):
		"""	Build a plugin. See shared_library in this class for a detailed description. """
		
		if "plugin" in dir(self.build_manager) :
			return self.build_manager.plugin( self, lib, sources, headers, pre, nopre, *keywords )
		
		self.setup_precompiled_headers( sources, pre, nopre )
		
		if self[ 'PLATFORM' ] == 'win32':
			self['PDB'] = lib + '.pdb'
			self.setup_clr_compilation()

		new_lib = self.SharedLibrary( lib, sources, *keywords )
		return new_lib

	def program( self, lib, sources, headers=None, pre=None, nopre=None, *keywords ):
		"""	Build a program. See shared_library in this class for a detailed description. """

		if "program" in dir(self.build_manager) :
			return self.build_manager.program( self, lib, sources, headers, pre, nopre, *keywords )
			
		self.setup_precompiled_headers( sources, pre, nopre )
		
		if self[ 'PLATFORM' ] == 'win32':
			self['PDB'] = lib + '.pdb'
			self.Append( LINKFLAGS="/SUBSYSTEM:WINDOWS" )
			self.setup_clr_compilation()
			
		return self.Program( lib, sources, *keywords )
		
	def console_program( self, lib, sources, headers=None, pre=None, nopre=None, *keywords ):
		"""	Build a console program. See shared_library in this class for a detailed description. """

		if "console_program" in dir(self.build_manager) : 
			return self.build_manager.console_program( self, lib, sources, headers, pre, nopre, *keywords )
			
		self.setup_precompiled_headers( sources, pre, nopre )
		
		if self[ 'PLATFORM' ] == 'win32':
			self['PDB'] = lib + '.pdb'
			self.Append( LINKFLAGS="/SUBSYSTEM:CONSOLE" )
			self.setup_clr_compilation()
		
		return self.Program( lib, sources, *keywords )
	
	def create_moc_file( self, file_to_moc, output_path ) :
		"""QT specific: moc the file requested.

		Keyword arguments:
		file_to_moc -- input header file
		output_path - output file
		"""

		rel_path = file_to_moc[ : file_to_moc.rfind('.')] + '_moc.h'
		output_file = os.path.join( output_path, rel_path )
		input_file = str( self.File( file_to_moc ) )
		if utils.needs_rebuild( input_file, output_file ):
			utils.ensure_output_path_exists( output_file )
			command = self[ 'QT4_MOC' ].replace( '/', os.sep )
			if command.startswith( 'bcomp' ): command = self.root + os.sep + command
			moc_command = command + ' -o "' + output_file + '" "' + input_file + '"' #  2>&1 >nul'
			for line in os.popen( moc_command ).read().split("\n"):
				if line != "" : raise Exception, moc_command
		return output_file
		
	def create_resource_cpp_file( self, res_file, output_path ) :
		"""QT specific: rcc the file requested.

		Keyword arguments:
		res_file -- the resource file
		output_path - output file
		"""

		rel_path = res_file[ : res_file.rfind('.')] + '.cpp'
		output_file = os.path.join( output_path, rel_path )
		input_file = str( self.File( res_file ) )
		if utils.needs_rebuild( input_file, output_file ):
			utils.ensure_output_path_exists( output_file )
			command = self[ 'QT4_RCC' ].replace( '/', os.sep )
			if command.startswith( 'bcomp' ): command = self.root + os.sep + command
			moc_command = command + ' -o "' + output_file + '" "' + input_file + '"' #  2>&1 >nul'
			for line in os.popen( moc_command ).read().split("\n"):
				if line != "" : raise Exception, moc_command
		return output_file
	
	def create_moc_cpp( self, generated_header_files, pre, output_path ) :
		"""QT specific: Generate the moc.cpp file which includes all the moc output.

		Keyword arguments:
		generated_header_files -- a list of files created by create_moc_file
		pre -- precompiled headers tuple
		output_path -- the full path and name of the output file
		"""

		moc_cpp_path = os.path.join( output_path , "moc.cpp")
		utils.ensure_output_path_exists( moc_cpp_path )
		moc_cpp = open( moc_cpp_path, "w+")
		if self[ 'PLATFORM' ] == 'win32' and pre is not None:
			moc_cpp.write( '#include "' + pre[1] + '"\n'  )
		
		for gfile in generated_header_files:
			moc_cpp.write( '#include "' + gfile + '"\n')
			
		moc_cpp.write('\n')
		moc_cpp.close()
		return moc_cpp_path
	
	def qt_common( self, lib, moc_files, sources, resources=None, headers=None, pre=None, nopre=None, *keywords ) : 
		depfiles = []
		if nopre == None : nopre = []
	
		output_path = self.qt_build_path( lib ) 
		
		for moc_file in moc_files :
			depfiles.append( self.create_moc_file( moc_file, output_path) )
			
		if resources is not None :
			for res in resources :
				nopre.append( self.create_resource_cpp_file( res, output_path) )

		sources.append( self.create_moc_cpp( depfiles, pre, output_path ) )
		
		self.setup_precompiled_headers( sources, pre, nopre )
		
	def qt_library( self, lib, moc_files, sources, resources=None, headers=None, pre=None, nopre=None, *keywords ) : 
		"""	Build a qt library. See shared_library in this class for a detailed description. """

		if "qt_library" in dir(self.build_manager) : 
			return self.build_manager.qt_library( self, lib, moc_files, sources, resources, headers, pre, nopre, *keywords )

		self.qt_common(  lib, moc_files, sources, resources, headers, pre, nopre, *keywords )
		
		if self[ 'PLATFORM' ] == 'win32':
			self['PDB'] = lib + '.pdb'
			self.setup_clr_compilation()
				
		return self.SharedLibrary( lib, sources, *keywords )
		
	def qt_program( self, lib, moc_files, sources, resources=None, headers=None, pre=None, nopre=None, *keywords ) : 
		"""	Build a qt program. See shared_library in this class for a detailed description. """

		if "qt_program" in dir(self.build_manager) : 
			return self.build_manager.qt_program( self, lib, moc_files, sources, resources, headers, pre, nopre, *keywords )

		self.qt_common( lib, moc_files, sources, resources, headers, pre, nopre, *keywords )

		if self[ 'PLATFORM' ] == 'win32':
			self['PDB'] = lib + '.pdb'
			self.Append( LINKFLAGS="/SUBSYSTEM:WINDOWS" )
		
		return self.Program( lib, sources, *keywords )
		
	def done( self, project_name = None ) :
		"""Called by SConscruct when the sconscripts are parsed."""
		if "done" in dir(self.build_manager) : 
			return self.build_manager.done( self, project_name )
		else : return True

	def Tool(self, tool, toolpath=None, **kw):
		"""Override of the base Tool method."""
		if toolpath == None :
			toolpath = [ utils.path_to_openbuild_tools() ]
		BaseEnvironment.Tool( self, tool, toolpath, **kw )

	def release( self, *kw ):
		"""Identify files by type and install in an appropriate directory."""

		dict = { }

		for list in kw:
			if type( list ) is types.StringType:
				list = [ list ]
			for file in list:
				if 'path' in dir( file ):
					name = os.path.join( self.root, file.path )
				else:
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
				elif name.endswith( '.framework' ):
					target = self[ 'stage_frameworks' ]
				elif name.endswith( '.exp' ):
					pass
				else:
					target = self[ 'stage_bin' ]
				if target != '':
					full = self.subst( os.path.join( target, name ) )
					if full not in Environment.already_installed:
						if 'path' in dir( file ):
							if file.is_derived( ):
								self.Install( target, file )
							else:
								if target not in dict.keys( ): dict[ target ] = [ ]
								dict[ target ] += [ name ]
						else:
							if target not in dict.keys( ): dict[ target ] = [ ]
							dict[ target ] += [ name ]
						Environment.already_installed[name] = [full]

		for target in dict.keys( ):
			self.copy_files( target, dict[ target ] )

	def install_dir( self, dst, src ):
		"""Synonym for copy_files."""
		self.copy_files( dst, src )

	def copy_project_files( self, dst, srcs ):
		list = [ ]
		if type( srcs ) is types.StringType: srcs = [ srcs ]
		elif type( srcs ) is not types.ListType: srcs = [ srcs ]
		for src_list in srcs:
			if 'all_children' in dir( src_list ): src_list = src_list.all_children( )
			elif type( src_list ) is types.StringType: src_list = [ src_list ]
			elif type( src_list ) is not types.ListType: srcs = [ src_list ]
			for src in src_list:
				if 'path' in dir( src ):
					if src.is_derived( ):
						if self.build_manager is not None : continue
						self.Install( dst, src )
					else:
						list.append( os.path.join( self.root, src.path ) )
				else:
					list.append( os.path.join( self.root, self.relative_path_to_sconscript, src ) )

		self.copy_files( dst, list )

	def filter_framework( self, src, dst ):
		dst_dir = dst
		if dst_dir.find( '/' ) != -1:
			if dst_dir[ dst_dir.rfind( '/' ) : ] == src[ src.rfind( '/' ) : ]:
				dst_dir = dst_dir[ 0 : dst_dir.rfind( '/' ) ]

		source = self.subst( utils.clean_path( dst ) )
		target = self.subst( utils.clean_path( self[ 'stage_libdir' ] ) )
		resources = self.subst( utils.clean_path( self[ 'stage_resources' ] ) )
		if source.endswith( '/' ): source = source[ 0:-1 ]
		if target.endswith( '/' ): target = target[ 0:-1 ]
		if resources.endswith( '/' ): resources = resources[ 0:-1 ]
		if dst.startswith( target ):
			sub_dir = dst_dir[ len( target ) : ]
			if not sub_dir.startswith( '/pkgconfig' ) and not sub_dir.startswith( '/python' ):
				if not src.endswith( '.pc' ) and not src.endswith( '.a' ) and not src.endswith( '.py' ) and not src.endswith( '.pyc' ) and not src.endswith( '.aml' ) and not src.endswith( '.so' ) and not src.endswith('.la'):
					Environment.bundle_libraries[ int( self.debug ) ] += [ [ dst_dir[ len( target ) : ], source ] ]
		if dst.startswith( resources ):
			sub_dir = dst_dir[ len( resources ) : ]
			if sub_dir not in [ '/examples' ]:
				Environment.bundle_resources[ int( self.debug ) ] += [ [ sub_dir, source ] ]

	def copy_files( self, dst, srcs, perms = None ):
		""" Installs src to dst, walking through the src if needed and invoking 
			the appropriate install action on every file found.

			Keyword arguments:
			dst -- destination directory
			src -- source directory
		"""

		if type( srcs ) is types.StringType: srcs = [ srcs ]
		elif type( srcs ) is not types.ListType: srcs = [ srcs ]

		# Collect types into these lists
		all_dirs = [ ]
		all_links = [ ]
		all_files = [ ]
		file_count = 0
		link_count = 0

		# Get scons options
		execute = not self.GetOption( 'no_exec' )
		silent = self.GetOption( 'silent' )

		# Clean the dst path
		dst = self.subst( utils.clean_path( dst ) ).replace( '/', os.sep )

		for src in srcs:

			# Clean src and dst to ensure they're normalised
			src = self.subst( utils.clean_path( src ) ).replace( '/', os.sep )

			# Check that this combination has not been tried before
			if dst in Environment.already_installed.keys() :
				if src in Environment.already_installed[dst] : continue
				else : Environment.already_installed[dst].append(src)
			else: Environment.already_installed[dst] = [src]

			# Handle 
			if not os.path.exists( src ) and self.build_manager is not None : continue
			if not os.path.exists( src ) :
				raise OSError, "Unable to locate %s to copy to %s" % ( src, dst )

			elif os.path.isdir( src ):
				# Source is a directory, walk it and collect the files, dirs and links in it
				for root, dirs, files in os.walk( src ):

					# Ignore hidden directories
					if root.find( os.sep + '.' ) != -1: continue

					# Derive the destination directory from the src
					dst_dir = dst + root.replace( src, '' )

					# Add derived dir to list which should be created or removed
					all_dirs += [ dst_dir ]

					# Remove hidden directories and handle directory sym links
					for dir in dirs:
						if dir.startswith( '.' ):
							dirs.remove( dir )
						elif os.path.islink( os.path.join( root, dir ) ):
							dirs.remove( dir )
							all_links += [ ( dst_dir, dir, os.readlink( os.path.join( root, dir ) ) ) ]
						else:
							all_dirs += [ os.path.join( dst_dir, dir ) ]
	
					# Remove hidden files, add sym links and normal files
					for file in files:
						full_src = os.path.join( root, file )
						full_dst = os.path.join( dst_dir, file )
						if file.startswith( '.' ):
							pass
						elif os.path.islink( full_src ):
							all_links += [ ( dst_dir, file, os.readlink( full_src ) ) ]
						else:
							all_files += [ ( full_src, full_dst ) ]
	
			elif os.path.islink( src ):
	
				# Source is a sym link
				file = src.rsplit( os.sep, 1 )[ -1 ]
				if not dst.endswith( os.sep + file ):
					all_dirs += [ dst ]
				else:
					dst = dst.rsplit( os.sep, 1 )[ 0 ]
				all_links += [ ( dst, file, os.readlink( src ) ) ]
	
			else:
	
				# Source is a regular file
				file = src.rsplit( os.sep, 1 )[ -1 ]
				full_dst = dst
				if not dst.endswith( os.sep + file ):
					all_dirs += [ dst ]
					full_dst = os.path.join( dst, file )
				else:
					all_dirs += [ dst.rsplit( os.sep, 1 )[ 0 ] ]
				all_files += [ ( src, full_dst ) ]

		# Uninstall or install according to scons usage
		if self.GetOption( 'clean' ):
			for dir, file, link in all_links:
				full = os.path.join( dir, file )
				if os.path.islink( full ):
					link_count += 1
					if execute: os.remove( full )
			for src, dst in all_files:
				if os.path.exists( os.path.join( dst ) ):
					file_count += 1
					if execute: os.unlink( dst )
			for dir in all_dirs:
				if os.path.exists( dir ):
					if execute:
						try:
							os.removedirs( dir )
						except OSError, e:
							# Ignore non-empty directories
							pass

			if not silent: print 'Removed', file_count, 'files and', link_count, 'links from', dst 
		else:
			for dir in all_dirs:
				if not os.path.exists( dir ):
					if execute: os.makedirs( dir )
			for src_file, dst_file in all_files:
				self.filter_framework( src_file, dst_file )
				if not os.path.exists( dst_file ) or os.path.getmtime( src_file ) > os.path.getmtime( dst_file ):
					file_count += 1
					if execute: 
						shutil.copyfile( src_file, dst_file )
						shutil.copystat( src_file, dst_file )
						if perms is not None:
							os.chmod( dst_file, perms )
			for dir, file, link in all_links:
                                if not os.path.exists( dir ):
                                        if execute: os.makedirs( dir )
				full = os.path.join( dir, file )
				if not os.path.exists( full ):
					link_count += 1
					if execute: 
						if os.path.islink( full ): os.remove( full )
						os.chdir( dir )
						os.symlink( link, file )
						os.chdir( self.root )

			if not silent and file_count >= 1: print 'Copied', file_count, 'files and', link_count, 'links to', dst

