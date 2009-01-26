# (C) Ardendo SE 2008
#
# Released under the LGPL

import os
import SCons.Util
import SCons.Platform
import SCons.Scanner
import SCons.Tool.msvs as default_msvs
from SCons.Script.SConscript import SConsEnvironment as BaseEnvironment

class VisualStudio:
	""" A class that describes a version of VisualStudio.
		Is it installed or not, if it is, what is the install
		path and the path to the vcvars.bat file """
		
	def __init__(self, version, regpath, install_path, vsvars_sub_path ):
		self.version = version
		self.registry_path = regpath
		self.registry_base_path = "Software\\Microsoft\\VisualStudio\\"
		self.base_install_path = os.path.join( SCons.Platform.win32.get_program_files_dir(), install_path)
		self.vsvars_sub_path = vsvars_sub_path
		self.install_path_internal = None
		
	def is_installed( self ) :
		""" For now, just use the registry for discovery.
			Could be refined in the future to look in the 
			default install path. See default version of msvc.py in 
			SCons/Tools/ 
			
			Calling this function will also set the internal install path
			since that is found in the registry. """
			
		if not self.install_path_internal is None : return True
		try:
			local_machine = SCons.Util.HKEY_LOCAL_MACHINE
			setup_key = SCons.Util.RegOpenKeyEx( local_machine, self.registry_base_path + self.registry_path )
			val = SCons.Util.RegQueryValueEx( setup_key, "ProductDir")
			if len(val) == 2 : 
				self.install_path_internal = val[0]
				return True
			else: return False
		except SCons.Util.RegError, e:
			# print e
			return False
	
	def install_path( self ) :
		if self.install_path_internal == None : 
			if self.is_installed() : return self.install_path_internal
			else  : return None
		else: return self.install_path_internal
		
	def vsvars_path( self) :
		if self.install_path_internal == None :
			if self.is_installed() : return os.path.join( self.install_path_internal, self.vsvars_sub_path )
			else  : return None
		else: return self.install_path_internal
		
class VSVersions:
	""" Provides all known versions of visual studio.
		Add more versions here as we find new ones. """
	vs2003 = VisualStudio( version = "vs2003",	
									regpath = "7.1\\Setup\\VS", 
									install_path = "Microsoft Visual Studio .NET 2003",
									vsvars_sub_path	= "Common7\\Tools")
									
	vs2005 = VisualStudio( version = "vs2005",	
									regpath = "8.0\\Setup\\VS", 
									install_path = "Microsoft Visual Studio 8",
									vsvars_sub_path	= "Common7\\Tools")
									
	vs2008 = VisualStudio( version = "vs2008",	
									regpath = "9.0\\Setup\\VS", 
									install_path = "Microsoft Visual Studio 9.0",
									vsvars_sub_path	= "Common7\\Tools")
									
	def all_versions( self ) : 
		return [ self.vs2008, self.vs2005, self.vs2003 ]
		
	def find_version( self, version ) :
		for ver in self.all_versions() :
			if ver.version == version : return ver
		return None
		
	def default_version( self ):
		for ver in self.all_versions() :
			if ver.is_installed() : return ver
		return None	
		

def run_vcvars_bat( env, vsver ):
	""" This file will run the correct version of vsvars.bat and
		copy the environment created by that call to the 
		environment passed to this function """
	
	file_name = os.path.join( env.root, "call_vcvars.bat")
	
	# The contents of the temporary bat-file.
	contents = "@echo off\n"
	contents += "call \"" +  vsver.vsvars_path() + "\\vsvars32.bat\" > nul 2> nul\n"
	contents += "set\n"
	contents += "exit\n"

	bat_file = open(file_name, "w+")
	bat_file.write(contents)
	bat_file.close()
	
	to_open = "cmd /K \"" + file_name + "\""
	
	bat_cmd_file = os.popen( to_open )
	for line in bat_cmd_file.read().split("\n"):
		tokens = line.split("=",1)
		if len(tokens) == 2 : env[ 'ENV' ][ tokens[0] ] = tokens[1]
	bat_cmd_file.close()
	os.remove( file_name )
	
		
def validate_vars(env):
	"""	Stolen from SCons/Tools/msvc.py
		Validate the PCH and PCHSTOP construction variables."""
	if env.has_key('PCH') and env['PCH']:
		if not env.has_key('PCHSTOP'):
			raise SCons.Errors.UserError, "The PCHSTOP construction must be defined if PCH is defined."
		if not SCons.Util.is_String(env['PCHSTOP']):
			raise SCons.Errors.UserError, "The PCHSTOP construction variable must be a string: %r"%env['PCHSTOP']

def pch_emitter(target, source, env):
	"""	Stolen from SCons/Tools/msvc.py
		Adds the object file target."""

	validate_vars(env)

	pch = None
	obj = None

	for t in target:
		if SCons.Util.splitext(str(t))[1] == '.pch':
			pch = t
		if SCons.Util.splitext(str(t))[1] == '.obj':
			obj = t

	if not obj:
		obj = SCons.Util.splitext(str(pch))[0]+'.obj'

	target = [pch, obj] # pch must be first, and obj second for the PCHCOM to work

	return (target, source)
		
def object_emitter(target, source, env, parent_emitter):
	"""	Stolen from SCons/Tools/msvc.py
		Sets up the PCH dependencies for an object file."""

	validate_vars(env)

	parent_emitter(target, source, env)

	if env.has_key('PCH') and env['PCH']:
		env.Depends(target, env['PCH'])

	return (target, source)

def static_object_emitter(target, source, env):
	""" Stolen from SCons/Tools/msvc.py """
	return object_emitter(target, source, env,
						  SCons.Defaults.StaticObjectEmitter)

def shared_object_emitter(target, source, env):
	""" Stolen from SCons/Tools/msvc.py """
	return object_emitter(target, source, env,
						  SCons.Defaults.SharedObjectEmitter)
						  
		
def setup_object_builders( env ):
	""" Creates the obj-file builders for msvc """
	c_source_files = ['.c', '.C']
	cpp_source_files = ['.cc', '.cpp', '.cxx', '.c++', '.C++']
	
	static_obj, shared_obj = SCons.Tool.createObjBuilders(env)

	for suffix in c_source_files:
		static_obj.add_action(suffix, SCons.Defaults.CAction)
		shared_obj.add_action(suffix, SCons.Defaults.ShCAction)
		static_obj.add_emitter(suffix, static_object_emitter)
		shared_obj.add_emitter(suffix, shared_object_emitter)

	for suffix in cpp_source_files:
		static_obj.add_action(suffix, SCons.Defaults.CXXAction)
		shared_obj.add_action(suffix, SCons.Defaults.ShCXXAction)
		static_obj.add_emitter(suffix, static_object_emitter)
		shared_obj.add_emitter(suffix, shared_object_emitter)
	
		
def setup_standard_environment( env ):
	""" Most of this code was stolen from the default setup of 
		msvc. The most important thing that was added was the
		res_builder to the SharedLibrary builder. """
		
	pch_action = SCons.Action.Action('$PCHCOM', '$PCHCOMSTR')
	pch_builder = SCons.Builder.Builder(action=pch_action, suffix='.pch',
										emitter=pch_emitter,
										source_scanner=SCons.Tool.SourceFileScanner)
										
	res_action = SCons.Action.Action('$RCCOM', '$RCCOMSTR')
	res_builder = SCons.Builder.Builder(action=res_action,
										src_suffix='.rc',
										suffix='.res',
										src_builder=[],
										source_scanner=SCons.Tool.SourceFileScanner)
	
	SCons.Tool.SourceFileScanner.add_scanner('.rc', SCons.Defaults.CScan)
	
	env['BUILDERS']['SharedLibrary'].add_src_builder(res_builder)
	env['BUILDERS']['Program'].add_src_builder(res_builder)
	
	env['CCPDBFLAGS'] = SCons.Util.CLVar(['${(PDB and "/Z7") or ""}'])
	env['CCPCHFLAGS'] = SCons.Util.CLVar(['${(PCH and "/Yu%s /Fp%s"%(PCHSTOP or "",File(PCH))) or ""}'])
	env['CCCOMFLAGS'] = '$CPPFLAGS $_CPPDEFFLAGS $_CPPINCFLAGS /c $SOURCES /Fo$TARGET $CCPCHFLAGS $CCPDBFLAGS'
	env['CC']		 = 'cl'
	env['CCFLAGS']	= SCons.Util.CLVar('/nologo')
	env['CFLAGS']	 = SCons.Util.CLVar('')
	env['CCCOM']	  = '$CC $CFLAGS $CCFLAGS $CCCOMFLAGS'
	env['SHCC']	   = '$CC'
	env['SHCCFLAGS']  = SCons.Util.CLVar('$CCFLAGS')
	env['SHCFLAGS']   = SCons.Util.CLVar('$CFLAGS')
	env['SHCCCOM']	= '$SHCC $SHCFLAGS $SHCCFLAGS $CCCOMFLAGS'
	env['CXX']		= '$CC'
	env['CXXFLAGS']   = SCons.Util.CLVar('$CCFLAGS $( /TP $)')
	env['CXXCOM']	 = '$CXX $CXXFLAGS $CCCOMFLAGS'
	env['SHCXX']	  = '$CXX'
	env['SHCXXFLAGS'] = SCons.Util.CLVar('$CXXFLAGS')
	env['SHCXXCOM']   = '$SHCXX $SHCXXFLAGS $CCCOMFLAGS'
	env['CPPDEFPREFIX']  = '/D'
	env['CPPDEFSUFFIX']  = ''
	env['INCPREFIX']  = '/I'
	env['INCSUFFIX']  = ''

	env['STATIC_AND_SHARED_OBJECTS_ARE_THE_SAME'] = 1

	env['RC'] = 'rc'
	env['RCFLAGS'] = SCons.Util.CLVar('')
	env['RCCOM'] = '$RC $_CPPDEFFLAGS $_CPPINCFLAGS $RCFLAGS /fo$TARGET $SOURCES'
	env['OBJPREFIX']	  = ''
	env['OBJSUFFIX']	  = '.obj'
	env['SHOBJPREFIX']	= '$OBJPREFIX'
	env['SHOBJSUFFIX']	= '$OBJSUFFIX'
	env['CFILESUFFIX'] = '.c'
	env['CXXFILESUFFIX'] = '.cc'

	env['PCHCOM'] = '$CXX $CXXFLAGS $CPPFLAGS $_CPPDEFFLAGS $_CPPINCFLAGS /c $SOURCES /Fo${TARGETS[1]} /Yc"$PCHSTOP" /Fp${TARGETS[0]} $CCPDBFLAGS $PCHPDBFLAGS'
	env['BUILDERS']['PCH'] = pch_builder
	

		
def exists(env):
	""" Required function that must be implemented by all tools.
		Checks for all versions if they are installed. If
		at least one is, return true """
	for ver in VSVersions().all_versions() :
		if ver.is_installed() : return 1
	return 0
	
def current_vc_version( env ) :
	""" Get the currently selected version of VisualStudio.
		If the passed environment is a openbuild.env it
		will have an options variable, if that has a 
		target key, its value will be used.
		
		If the compiler_name is still not known, we are
		looking for a TOOL variable in env. If that 
		also fail, we call VSVersions.default_version(). """
		
	compiler_name = None
	
	if 'options' in dir(env) :
		base_env = BaseEnvironment()
		env.options.Update(base_env)
		compiler_name = base_env.Dictionary().get('target', None ) 
	
	if compiler_name == None :
		compiler_name = env.Dictionary().get('TOOL', None) 
		
	vsver = None
	if compiler_name is not None :
		vsver = VSVersions().find_version( compiler_name )
		
	if vsver == None : vsver = VSVersions().default_version()
	return vsver
	
def generate(env):
	""" Setup the environment to use one version of VisualStudio"""
	
	vsver = current_vc_version( env )
	
	run_vcvars_bat( env, vsver )
	setup_object_builders( env )
	setup_standard_environment( env )
	
	
