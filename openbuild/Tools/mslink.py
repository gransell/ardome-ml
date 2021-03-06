# (C) Ardendo SE 2008
#
# Released under the LGPL

import os
import SCons.Tool
import SCons.Action
import SCons.Defaults
import SCons.Errors
import SCons.Platform.win32
import SCons.Tool
import SCons.Util
import msvc

def pdbGenerator(env, target, source, for_signature):
	try:
		return ['/PDB:%s' % target[0].attributes.pdb, '/DEBUG']
	except (AttributeError, IndexError):
		return None

def windowsShlinkTargets(target, source, env, for_signature):
	listCmd = []
	dll = env.FindIxes(target, 'SHLIBPREFIX', 'SHLIBSUFFIX')
	if dll: listCmd.append("/out:%s"%dll.get_string(for_signature))

	implib = env.FindIxes(target, 'LIBPREFIX', 'LIBSUFFIX')
	if implib: listCmd.append("/implib:%s"%implib.get_string(for_signature))

	return listCmd

def windowsShlinkSources(target, source, env, for_signature):
	listCmd = []

	deffile = env.FindIxes(source, "WINDOWSDEFPREFIX", "WINDOWSDEFSUFFIX")
	for src in source:
		if src == deffile:
			# Treat this source as a .def file.
			listCmd.append("/def:%s" % src.get_string(for_signature))
		else:
			# Just treat it as a generic source file.
			listCmd.append(src)
	return listCmd

def windowsLibEmitter(target, source, env):
	SCons.Tool.msvc.validate_vars(env)

	extratargets = []
	extrasources = []

	dll = env.FindIxes(target, "SHLIBPREFIX", "SHLIBSUFFIX")
	no_import_lib = env.get('no_import_lib', 0)

	if not dll:
		raise SCons.Errors.UserError, "A shared library should have exactly one target with the suffix: %s" % env.subst("$SHLIBSUFFIX")

	insert_def = env.subst("$WINDOWS_INSERT_DEF")
	if not insert_def in ['', '0', 0] and \
	   not env.FindIxes(source, "WINDOWSDEFPREFIX", "WINDOWSDEFSUFFIX"):

		# append a def file to the list of sources
		extrasources.append(
			env.ReplaceIxes(dll,
							"SHLIBPREFIX", "SHLIBSUFFIX",
							"WINDOWSDEFPREFIX", "WINDOWSDEFSUFFIX"))

	version_num, suite = SCons.Tool.msvs.msvs_parse_version(env.get('MSVS_VERSION', '6.0'))
	# if version_num >= 8.0 : # and env.get('WINDOWS_INSERT_MANIFEST', 0):
		# #print "Will add MANIFEST"
		# # # MSVC 8 automatically generates .manifest files that must be installed
		# extratargets.append(
			# env.ReplaceIxes(dll,
							# "SHLIBPREFIX", "SHLIBSUFFIX",
							# "WINDOWSSHLIBMANIFESTPREFIX", "WINDOWSSHLIBMANIFESTSUFFIX"))

	if env.has_key('PDB') and env['PDB']:
		pdb = env.arg2nodes('$PDB', target=target, source=source)[0]
		extratargets.append(pdb)
		target[0].attributes.pdb = pdb

	if not no_import_lib and \
	   not env.FindIxes(target, "LIBPREFIX", "LIBSUFFIX"):
		# Append an import library to the list of targets.
		extratargets.append(
			env.ReplaceIxes(dll,
							"SHLIBPREFIX", "SHLIBSUFFIX",
							"LIBPREFIX", "LIBSUFFIX"))
		# and .exp file is created if there are exports from a DLL
		extratargets.append(
			env.ReplaceIxes(dll,
							"SHLIBPREFIX", "SHLIBSUFFIX",
							"WINDOWSEXPPREFIX", "WINDOWSEXPSUFFIX"))

	return (target+extratargets, source+extrasources)

def prog_emitter(target, source, env):
	SCons.Tool.msvc.validate_vars(env)

	extratargets = []

	exe = env.FindIxes(target, "PROGPREFIX", "PROGSUFFIX")
	if not exe:
		raise SCons.Errors.UserError, "An executable should have exactly one target with the suffix: %s" % env.subst("$PROGSUFFIX")

	# version_num, suite = SCons.Tool.msvs.msvs_parse_version(env.get('MSVS_VERSION', '6.0'))
	# if version_num >= 8.0 : # and env.get('WINDOWS_INSERT_MANIFEST', 0):
		# # print "Will add manifest"
		# # # MSVC 8 automatically generates .manifest files that have to be installed
		# extratargets.append(
			# env.ReplaceIxes(exe,
							# "PROGPREFIX", "PROGSUFFIX",
							 # "WINDOWSPROGMANIFESTPREFIX", "WINDOWSPROGMANIFESTSUFFIX"))

	if env.has_key('PDB') and env['PDB']:
		pdb = env.arg2nodes('$PDB', target=target, source=source)[0]
		extratargets.append(pdb)
		target[0].attributes.pdb = pdb

	return (target+extratargets,source)

def RegServerFunc(target, source, env):
	if env.has_key('register') and env['register']:
		ret = regServerAction([target[0]], [source[0]], env)
		if ret:
			raise SCons.Errors.UserError, "Unable to register %s" % target[0]
		else:
			print "Registered %s sucessfully" % target[0]
		return ret
	return 0

regServerAction = SCons.Action.Action("$REGSVRCOM", "$REGSVRCOMSTR")
regServerCheck = SCons.Action.Action(RegServerFunc, None)

mergeManifestActionProg = SCons.Action.Action('mt.exe -nologo -manifest ${TARGET}.manifest -outputresource:${TARGET};1')
mergeManifestActionDll = SCons.Action.Action('mt.exe -nologo -manifest ${TARGET}.manifest -outputresource:${TARGET};2')

shlibLinkAction = SCons.Action.Action('${TEMPFILE("$SHLINK $SHLINKFLAGS $_SHLINK_TARGETS $( $_LIBDIRFLAGS $) $_LIBFLAGS $_PDB $_SHLINK_SOURCES")}')
progLinkAction = SCons.Action.Action('${TEMPFILE("$LINK $LINKFLAGS /OUT:$TARGET.windows $( $_LIBDIRFLAGS $) $_LIBFLAGS $_PDB $SOURCES.windows")}')

compositeLinkAction = shlibLinkAction + regServerCheck
compositeProgLinkAction = progLinkAction 

shlibLinkActionManifest = SCons.Action.Action('${TEMPFILE("$SHLINK $SHLINKFLAGS /MANIFEST $_SHLINK_TARGETS $( $_LIBDIRFLAGS $) $_LIBFLAGS $_PDB $_SHLINK_SOURCES")}')
progLinkActionManifest = SCons.Action.Action('${TEMPFILE("$LINK $LINKFLAGS /MANIFEST /OUT:$TARGET.windows $( $_LIBDIRFLAGS $) $_LIBFLAGS $_PDB $SOURCES.windows")}')

compositeLinkActionMergeManifest = shlibLinkActionManifest + mergeManifestActionDll + regServerCheck
compositeProgLinkActionMergeManifest = progLinkActionManifest + mergeManifestActionProg

def exists(env):
	return msvc.exists(env)
	
def generate(env):
	SCons.Tool.createSharedLibBuilder(env)
	SCons.Tool.createStaticLibBuilder(env)
	SCons.Tool.createProgBuilder(env)
	
	current_visual_studio_version = msvc.current_vc_version( env ) 
	msvc.run_vcvars_bat( env, current_visual_studio_version )
	
	env['SHLINKFLAGS'] = SCons.Util.CLVar('$LINKFLAGS /dll')
	env['LINKFLAGS']   = SCons.Util.CLVar('/nologo')
	
	if current_visual_studio_version.version > 'vs2003' :
		env['SHLINKCOM']  =  compositeLinkActionMergeManifest
		env['LINKCOM'] = compositeProgLinkActionMergeManifest
	else:
		env['SHLINKCOM']  =  compositeLinkAction
		env['LINKCOM'] = compositeProgLinkAction
		
	env['_SHLINK_TARGETS'] = windowsShlinkTargets
	env['_SHLINK_SOURCES'] = windowsShlinkSources
	
	env.Append(SHLIBEMITTER = [windowsLibEmitter])
	env['LINK']	= 'link'
	env['SHLINK']	= 'link'
	env['_PDB'] = pdbGenerator
	# This command is run when building a program or a static library,, see shlibLinkAction above for the command used for linking shared libraries.
	
	env.Append(PROGEMITTER = [prog_emitter])
	env['LIBDIRPREFIX']='/LIBPATH:'
	env['LIBDIRSUFFIX']=''
	env['LIBLINKPREFIX']=''
	env['LIBLINKSUFFIX']='$LIBSUFFIX'

	env['WIN32DEFPREFIX']		= ''
	env['WIN32DEFSUFFIX']		= '.def'
	env['WIN32_INSERT_DEF']	  = 0
	env['WINDOWSDEFPREFIX']	  = '${WIN32DEFPREFIX}'
	env['WINDOWSDEFSUFFIX']	  = '${WIN32DEFSUFFIX}'
	env['WINDOWS_INSERT_DEF']	= '${WIN32_INSERT_DEF}'

	env['WIN32EXPPREFIX']		= ''
	env['WIN32EXPSUFFIX']		= '.exp'
	env['WINDOWSEXPPREFIX']	  = '${WIN32EXPPREFIX}'
	env['WINDOWSEXPSUFFIX']	  = '${WIN32EXPSUFFIX}'

	env['WINDOWSSHLIBMANIFESTPREFIX'] = ''
	env['WINDOWSSHLIBMANIFESTSUFFIX'] = '${SHLIBSUFFIX}.manifest'
	env['WINDOWSPROGMANIFESTPREFIX']  = ''
	env['WINDOWSPROGMANIFESTSUFFIX']  = '${PROGSUFFIX}.manifest'

	env['REGSVRACTION'] = regServerCheck
	env['REGSVR'] = os.path.join(SCons.Platform.win32.get_system_root(),'System32','regsvr32')
	env['REGSVRFLAGS'] = '/s '
	env['REGSVRCOM'] = '$REGSVR $REGSVRFLAGS ${TARGET.windows}'
	
	
	
