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
			
		self.root = os.path.abspath( '.' )
			
		self.toolpath = [ utils.path_to_openbuild_tools() ]
		self.options = opts
		
		BaseEnvironment.__init__( self, ENV = os.environ , **kw )
		
		opts.Update( self )
		# Remove the vs option if it is not given on the command-line.
		if utils.vs() == None and 'vs' in self._dict.keys( ): del self._dict['vs'] 
		opts.Save( opts.file, self )
		
		# Override the default scons hash based checking
		# TODO: Check logic for generated files
		# self.Decider( 'timestamp-match' )

		# Check if we need to override default build behaviour.
		if bmgr : self.build_manager = bmgr
		elif utils.vs() : 
			self.build_manager = Environment.vs_builder
			self['target'] = utils.vs()
		else: 
			self.build_manager = None
			
		
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
		self[ 'stage_frameworks' ] = os.path.join( '$stage_prefix', 'frameworks' )

		self.release_install = 'install' in sys.argv
		self.debug_install = 'debug-install' in sys.argv
		
		self.Alias( 'install', self[ 'distdir' ] + self[ 'prefix' ] )
		self.Alias( 'debug-install', self[ 'distdir' ] + self[ 'prefix' ] )

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
					if not os.path.exists( dst_file ) or os.path.getmtime( src ) > os.path.getmtime( dst_file ):
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
		self.ensure_output_path_exists( path )
		return path
		
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
		self[ 'debug' ] = '0'
		self.debug = False
		self[ 'build_prefix' ] = '$release_prefix'
		self[ 'stage_prefix' ] = utils.install_target( self )

		if self[ 'PLATFORM' ] == 'posix':
			self.Append( CCFLAGS = [ '-Wall', '-O3', '-pipe' ] )
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
			for dest, rule in [ ( 'stage_include', 'install_include' ), 
								( 'stage_libdir', 'install_libs' ), 
								( 'stage_frameworks', 'install_frameworks' ) ]:
				for file in env.package_manager.package_install_list( env, rule ):
					env.copy_files( os.path.join( env[ dest ], file.rsplit( os.sep, 1 )[ -1 ] ), file )

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
		
		# Keep this path in the environment, is used by for instance vsbuild.
		self.relative_path_to_sconscript = path
		
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
						
					self.Depends( result[ build_type ], dep[ build_type ] )
					file = str( dep[ build_type ][ 0 ] )
					libpath, lib = file.rsplit( os.sep, 1 )
					libpath = os.path.join( self.root, libpath )
					lib = lib.rsplit( '.', 1 )[ 0 ]
					local_env.Append( LIBPATH = [libpath] )
					local_env.Append( LIBS = [lib] )

		return result
		
	def setup_precompiled_headers( self, sources, pre = None , nopre = None ) :
		if nopre is not None: 
			for f in nopre:
				sources.extend(self.Object(f, PCH=None, PCHSTOP=None))

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
	
		if self[ 'PLATFORM' ] == 'win32':
			self['PDB'] = lib + '.pdb'
		
		return self.SharedLibrary( lib, sources, *keywords )
		
	def plugin( self, lib, sources, headers=None, pre=None, nopre=None, *keywords ):
		"""	Build a plugin. See shared_library in this class for a detailed description. """
		
		if "plugin" in dir(self.build_manager) :
			return self.build_manager.plugin( self, lib, sources, headers, pre, nopre, *keywords )
		
		self.setup_precompiled_headers( sources, pre, nopre )
		
		if self[ 'PLATFORM' ] == 'win32':
			self['PDB'] = lib + '.pdb'

		return self.SharedLibrary( lib, sources, *keywords )

	def program( self, lib, sources, headers=None, pre=None, nopre=None, *keywords ):
		"""	Build a program. See shared_library in this class for a detailed description. """

		if "program" in dir(self.build_manager) :
			return self.build_manager.program( self, lib, sources, headers, pre, nopre, *keywords )
			
		self.setup_precompiled_headers( sources, pre, nopre )
		
		if self[ 'PLATFORM' ] == 'win32':
			self['PDB'] = lib + '.pdb'
			self.Append( LINKFLAGS="/SUBSYSTEM:WINDOWS" )
			
		return self.Program( lib, sources, *keywords )
		
	def console_program( self, lib, sources, headers=None, pre=None, nopre=None, *keywords ):
		"""	Build a console program. See shared_library in this class for a detailed description. """

		if "console_program" in dir(self.build_manager) : 
			return self.build_manager.console_program( self, lib, sources, headers, pre, nopre, *keywords )
			
		self.setup_precompiled_headers( sources, pre, nopre )
		
		if self[ 'PLATFORM' ] == 'win32':
			self['PDB'] = lib + '.pdb'
			self.Append( LINKFLAGS="/SUBSYSTEM:CONSOLE" )
		
		return self.Program( lib, sources, *keywords )
	
	def ensure_output_path_exists( self, opath ) :
		try:
			if os.path.exists( os.path.dirname(opath) ) : return
			os.makedirs(  os.path.dirname(opath)  )
		except os.error, e:
			pass
	
	def needs_rebuild( self, src, dst ):
		result = True
		if os.path.exists( src ):
			if os.path.exists( dst ):
				result = os.path.getmtime( src ) > os.path.getmtime( dst )
			if result:
				print "rebuilding %s from %s" % ( dst, src )
		else:
			raise Exception, "Error: source file %s does not exist." % src
		return result

	def create_moc_file( self, file_to_moc, output_path ) :
		rel_path = file_to_moc[ : file_to_moc.rfind('.')] + '_moc.h'
		output_file = os.path.join( output_path, rel_path )
		input_file = str( self.File( file_to_moc ) )
		if self.needs_rebuild( input_file, output_file ):
			self.ensure_output_path_exists( output_file )
			command = self[ 'QT4_MOC' ].replace( '/', os.sep )
			if command.startswith( 'bcomp' ): command = self.root + os.sep + command
			moc_command = command + ' -o "' + output_file + '" "' + input_file + '"' #  2>&1 >nul'
			for line in os.popen( moc_command ).read().split("\n"):
				if line != "" : raise Exception, moc_command
		return output_file
		
	def create_resource_cpp_file( self, res_file, output_path ) :
		rel_path = res_file[ : res_file.rfind('.')] + '.cpp'
		output_file = os.path.join( output_path, rel_path )
		input_file = str( self.File( res_file ) )
		if self.needs_rebuild( input_file, output_file ):
			self.ensure_output_path_exists( output_file )
			command = self[ 'QT4_RCC' ].replace( '/', os.sep )
			if command.startswith( 'bcomp' ): command = self.root + os.sep + command
			moc_command = command + ' -o "' + output_file + '" "' + input_file + '"' #  2>&1 >nul'
			for line in os.popen( moc_command ).read().split("\n"):
				if line != "" : raise Exception, moc_command
		return output_file
	
	def create_moc_cpp( self, generated_header_files, pre, output_path ) :
		moc_cpp_path = os.path.join( output_path , "moc.cpp")
		self.ensure_output_path_exists( moc_cpp_path )
		moc_cpp = open( moc_cpp_path, "w+")
		if self[ 'PLATFORM' ] == 'win32' and pre is not None:
			moc_cpp.write( '#include "' + pre[1] + '"\n'  )
		
		for gfile in generated_header_files:
			moc_cpp.write( '#include "' + gfile + '"\n')
			
		moc_cpp.write('\n')
		moc_cpp.close()
		return moc_cpp_path
				
	def qt_program( self, lib, moc_files, sources, resources=None, headers=None, pre=None, nopre=None, *keywords ) : 

		if "qt_program" in dir(self.build_manager) : 
			return self.build_manager.qt_program( self, lib, moc_files, sources, resources, headers, pre, nopre, *keywords )

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

		if self[ 'PLATFORM' ] == 'win32':
			self['PDB'] = lib + '.pdb'
			self.Append( LINKFLAGS="/SUBSYSTEM:WINDOWS" )
		
		return self.Program( lib, sources, *keywords )
		
	def done( self, project_name = None ) :
		if "done" in dir(self.build_manager) : 
			return self.build_manager.done( self, project_name )
		else : return True

	def Tool(self, tool, toolpath=None, **kw):
		if toolpath == None :
			toolpath = [ utils.path_to_openbuild_tools() ]
		
		BaseEnvironment.Tool( self, tool, toolpath, **kw )

	def release( self, *kw ):
		for list in kw:
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
								self.copy_files( target, name )
						else:
							self.copy_files( target, name )
						Environment.already_installed[name] = [full]

	def install_dir( self, dst, src ):
		self.copy_files( dst, src )

	def copy_files( self, dst, src ):
		""" Installs src to dst, walking through the src if needed and invoking 
			the appropriate install action on every file found.

			Keyword arguments:
			dst -- destination directory
			src -- source directory
		"""

		# Get scons options
		execute = not self.GetOption( 'no_exec' )
		silent = self.GetOption( 'silent' )

		# Clean src and dst to ensure they're normalised
		dst = self.subst( utils.clean_path( dst ) ).replace( '/', os.sep )
		src = self.subst( utils.clean_path( src ) ).replace( '/', os.sep )

		# Check that this combination has not been tried before
		if dst in Environment.already_installed.keys() :
			if src in Environment.already_installed[dst] : return
			else : Environment.already_installed[dst].append(src)
		else: Environment.already_installed[dst] = [src]

		# Collect types into these lists
		all_dirs = [ ]
		all_links = [ ]
		all_files = [ ]

		# Handle 
		if not os.path.exists( src ) and self.build_manager is not None : return
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
						files.remove( file )
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
					if not silent: print 'rm link', full
					if execute: os.remove( full )
			for src, dst in all_files:
				if os.path.exists( os.path.join( dst ) ):
					if not silent: print 'rm', dst
					if execute: os.unlink( dst )
			for dir in all_dirs:
				if os.path.exists( dir ):
					if not silent: print 'rmdir', dir
					if execute:
						try:
							os.removedirs( dir )
						except OSError, e:
							# Ignore non-empty directories
							pass
		else:
			for dir in all_dirs:
				if not os.path.exists( dir ):
					if not silent: print 'mkdir', dir
					if execute: os.makedirs( dir )
			for src, dst in all_files:
				if not os.path.exists( dst ) or os.path.getmtime( src ) > os.path.getmtime( dst ):
					if not silent: print 'cp', src, dst
					if execute: shutil.copyfile( src, dst )
			for dir, file, link in all_links:
				full = os.path.join( dir, file )
				if not os.path.islink( full ) or os.path.getmtime( src ) > os.path.getmtime( full ):
					if not silent: print 'link', dir, file, link
					if execute: 
						if os.path.islink( full ): os.remove( full )
						os.chdir( dir )
						os.symlink( link, file )
						os.chdir( self.root )

	def generate_tester(source, target, env, for_signature):
		
		return '$SOURCE all_tests --log_level=all > $TARGET || (cat $TARGET; exit 1)'

		testrunner = Builder(generator = generate_tester)
		self.Append( BUILDERS = {'TestRunner': testrunner} )
	
	def run_test( self, *args ) :
		if "run_test" in dir(self.build_manager) : 
			return self.build_manager.run_test( self, *args )
		
		return self.TestRunner(*args)

