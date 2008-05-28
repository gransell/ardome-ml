import os
import SCons.Util
import SCons.Platform
import SCons.Tool.msvs as default_msvs
from SCons.Script.SConscript import SConsEnvironment as BaseEnvironment

class VisualStudio:
	def __init__(self, version, regpath, install_path, vsvars_sub_path ):
		self.version = version
		self.registry_path = regpath
		self.registry_base_path = "Software\\Microsoft\\VisualStudio\\"
		self.base_install_path = os.path.join( SCons.Platform.win32.get_program_files_dir(), install_path)
		self.vsvars_sub_path = vsvars_sub_path
		self.install_path_internal = None
		
	def is_installed( self ) :
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
		

def run_vcvars_bat( env, vsver):
	
	file_name = os.path.join( os.path.split(__file__)[0], "call_vcvars.bat")
	
	contents = "@echo off\n"
	contents += "call \"" +  vsver.vsvars_path() + "\\vsvars32.bat\" > nul 2> nul\n"
	contents += "set\n"
	contents += "exit\n"

	bat_file = open(file_name, "w+")
	bat_file.write(contents)
	bat_file.close()
	
	to_open = "cmd /K \"" + file_name + "\""
	
	for line in os.popen( to_open ).read().split("\n"):
		tokens = line.split("=",1)
		if len(tokens) == 2 : env[ 'ENV' ][ tokens[0] ] = tokens[1]
		
def validate_vars(env):
	"""Validate the PCH and PCHSTOP construction variables."""
	if env.has_key('PCH') and env['PCH']:
		if not env.has_key('PCHSTOP'):
			raise SCons.Errors.UserError, "The PCHSTOP construction must be defined if PCH is defined."
		if not SCons.Util.is_String(env['PCHSTOP']):
			raise SCons.Errors.UserError, "The PCHSTOP construction variable must be a string: %r"%env['PCHSTOP']

def pch_emitter(target, source, env):
	"""Adds the object file target."""

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
	"""Sets up the PCH dependencies for an object file."""

	validate_vars(env)

	parent_emitter(target, source, env)

	if env.has_key('PCH') and env['PCH']:
		env.Depends(target, env['PCH'])

	return (target, source)

def static_object_emitter(target, source, env):
	return object_emitter(target, source, env,
						  SCons.Defaults.StaticObjectEmitter)

def shared_object_emitter(target, source, env):
	return object_emitter(target, source, env,
						  SCons.Defaults.SharedObjectEmitter)
		
def setup_object_builders( env ):
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
	env['BUILDERS']['RES'] = res_builder
	env['OBJPREFIX']	  = ''
	env['OBJSUFFIX']	  = '.obj'
	env['SHOBJPREFIX']	= '$OBJPREFIX'
	env['SHOBJSUFFIX']	= '$OBJSUFFIX'
	env['CFILESUFFIX'] = '.c'
	env['CXXFILESUFFIX'] = '.cc'

	env['PCHPDBFLAGS'] = SCons.Util.CLVar(['${(PDB and "/Yd") or ""}'])
	env['PCHCOM'] = '$CXX $CXXFLAGS $CPPFLAGS $_CPPDEFFLAGS $_CPPINCFLAGS /c $SOURCES /Fo${TARGETS[1]} /Yc$PCHSTOP /Fp${TARGETS[0]} $CCPDBFLAGS $PCHPDBFLAGS'
	env['BUILDERS']['PCH'] = pch_builder

		
def exists(env):
	for ver in VSVersions().all_versions() :
		if ver.is_installed() : return 1
	return 0
	
def current_vc_version( env ) :
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
	
	vsver = current_vc_version( env )
	
	run_vcvars_bat( env, vsver )
	setup_object_builders( env )
	setup_standard_environment( env )
	
	
