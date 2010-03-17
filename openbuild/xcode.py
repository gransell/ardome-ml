

import os

from xcodeproject import *

def _add_groups_and_sources( project, env, lib, sources, headers=None ):
	grp = project.group_item_for_name(str(lib))
	header_grp = None
	source_grp = None
	
	resulting_headers = []
	resulting_sources = []
	
	if grp is None :
		grp = XcodeGroup(str(lib))
		project.groups().append(grp)
		
		header_grp = XcodeGroup('Headers')
		grp.children().append(header_grp)
		source_grp = XcodeGroup('Sources')
		grp.children().append(source_grp)
	else :
		header_grp = grp.group_item_for_name('Headers')
		source_grp = grp.group_item_for_name('Sources')
	
	for s in sources :
		source_name = s
		settings = {}
		if isinstance( s, dict ):
			source_name = s.keys()[0]
			settings = s[source_name]
		source_path = os.path.join(env.root, env.relative_path, str(source_name))
		itm = project.source_item_for_path( source_path )
		if itm is None :
			itm = XcodeSourceFile( source_path, 'source' )
			itm.build_settings().update(settings)
			project.sources().append(itm)
			
		if not itm in source_grp.sources() : source_grp.sources().append(itm)
		resulting_sources.append(itm)
	
	if headers is not None :
		for h in headers :
			header_path = os.path.join(env.root, env.relative_path, str(h))
			itm = project.source_item_for_path( header_path )
			if itm is None:
				itm = XcodeSourceFile( header_path, 'header' )
				project.sources().append( itm )
			if not itm in header_grp.sources() : header_grp.sources().append(itm)
			resulting_headers.append(itm)
	
	return (resulting_headers, resulting_sources)

def _evaluate_paths_for_key( env, key ):
	dirs = []
	if( env.has_key(key) ):
		for val in env[key]:
			if isinstance(val, list):
				val = '='.join(val)
			dirs.append( str(val).replace('#/', env.root + '/') )
	return dirs


settings_name_map = {'CXXFLAGS' : 'COMPILER_FLAGS', }

stage_dir_map = { 'shared_library' : 'stage_libdir',
 				'program' : 'stage_bin'}

def _remap_setting_names( source ):
	for s in source.build_settings().keys():
		if s in settings_name_map:
			source.build_settings()[settings_name_map[s]] = source.build_settings()[s]
			del source.build_settings()[s]

def _add_build_configuration( target, target_type, env, pre=None ):
	conf = XcodeBuildConfiguration( ['Release', 'Debug'][env.debug], target )
	incs = _evaluate_paths_for_key( env, 'CPPPATH' )
	if len(incs):
		conf.build_settings()['HEADER_SEARCH_PATHS'] = '(' + str(incs).strip('[]').replace(', ', ',\n') + ')'
	lib_paths = _evaluate_paths_for_key( env, 'LIBPATH' )
	if len(lib_paths):
		conf.build_settings()['LIBRARY_SEARCH_PATHS'] = '(' + str(lib_paths).strip('[]').replace(', ', ',\n') + ',\n\'' + env.subst(env['stage_libdir']) + '\',)'
	macros = _evaluate_paths_for_key( env, 'CPPDEFINES' )
	if len(macros):
		conf.build_settings()['GCC_PREPROCESSOR_DEFINITIONS'] = '(' + str(macros).strip('[]').replace(', ', ',\n') + ')'
	
	fmwk_path = _evaluate_paths_for_key( env, 'FRAMEWORKPATH' )
	if len( fmwk_path ) :
		conf.build_settings()['FRAMEWORK_SEARCH_PATHS'] = '(' + str(fmwk_path).strip('[]').replace(', ', ',\n') + ')'

	
	libs = _evaluate_paths_for_key( env, 'LIBS' )
	fmwks = _evaluate_paths_for_key( env, 'FRAMEWORKS' )
	if len(libs) or len(fmwks):
		libs_and_fmwks = []
		for l in libs:
			if l.endswith('.a'):
				libs_and_fmwks.append(l)
			else:
				libs_and_fmwks.append('-l' + l)
		libs_and_fmwks.extend( ['-framework ' + f for f in fmwks] )
		conf.build_settings()['OTHER_LDFLAGS'] = '(' + str(libs_and_fmwks).strip('[]').replace(', ', ',\n') + ',\n\'-undefined dynamic_lookup\', -pthread, \"-fvisibility=default\")'
	
	if env.has_key('install_name'):
		conf.build_settings()['INSTALL_PATH'] = '"' + env['install_name'] + '"'
		
	conf.build_settings()['CONFIGURATION_TEMP_DIR'] = str(os.path.join( env.root, env.subst( env[ 'build_prefix' ] ), 'tmp', env.relative_path ))
	conf.build_settings()['CONFIGURATION_BUILD_DIR'] = str( os.path.join( env.root, env.subst( env[ stage_dir_map[target_type] ] ) ) )
	
	conf.build_settings()['GCC_INLINES_ARE_PRIVATE_EXTERN'] = 'NO'
	conf.build_settings()['GCC_CW_ASM_SYNTAX'] = 'NO'
	conf.build_settings()['GCC_ENABLE_PASCAL_STRINGS'] = 'NO'
	conf.build_settings()['PREBINDING'] = 'NO'
	conf.build_settings()['OTHER_CFLAGS'] = '(\"-pthread\", \"-fvisibility=default\")'
	conf.build_settings()['OTHER_CPLUSPLUSFLAGS'] = '(\"-pthread\", \"-fvisibility=default\")'
	
	if env.debug:
		conf.build_settings()['GCC_OPTIMIZATION_LEVEL'] = '0'
	
	if pre is not None and len(pre):
		conf.build_settings()['GCC_PRECOMPILE_PREFIX_HEADER'] = 'YES'
		conf.build_settings()['GCC_PREFIX_HEADER'] = '\'' + str(pre) + '\''
	
	target.build_configurations().append( conf )
	
	
def _resolve_precompiled_header_file( env, pre, header_items ):
	actuall_header = pre[1]
	for h in header_items:
		if actuall_header == h.path().split('/')[-1]:
			return h.path()
	
	return None

target_types = { 'shared_library' : XcodeDylibTarget,
 				'program' : XcodeExecutableTarget, }

def _add_target( project, env, lib, name, target_type, sources, headers, pre=None ):
	target = project.target_with_name(str(lib))
	if target is None:
		target = target_types[target_type](name)
		# Add the sources that should be compiled for this target
		target.sources().extend( sources )
		
		project.targets().append(target)
	
	resolved_pre = None
	# if pre is not None and len(pre):
	# 	resolved_pre = _resolve_precompiled_header_file( env, pre, headers )
	
	_add_build_configuration( target, target_type, env, resolved_pre )
	

class XcodeBuilder(object):
	def __init__(self):
		super(XcodeBuilder, self).__init__()
		self._project = XcodeProject()
	
	def shared_library( self, env, lib, sources, headers=None, pre=None, nopre=None, *keywords ):
		
		header_items, source_items = _add_groups_and_sources( self._project, env, lib, sources, headers )
		
		_add_target( self._project, env, lib, str(lib), 'shared_library', source_items, header_items, pre )
		
		return [os.path.join( env['stage_libdir'], 'lib' + str(lib) + '.dylib' )]
	
	def shared_object( self, env, sources, **keywords ):
		"""Just return the source file in this case since we want that and not the output that would
		have been generated by SharedObject"""
		result = sources
		if len(keywords):
			result = []
			for s in sources:
				result.append( {s : keywords} )
		return result
	
	def plugin( self, env, lib, sources, headers=None, pre=None, nopre=None, *keywords ):
		"""docstring for plugin"""
		return self.shared_library( env, lib, sources, headers, pre, nopre, *keywords )
	
	def program( self, env, lib, sources, headers=None, pre=None, nopre=None, *keywords ):
		header_items, source_items = _add_groups_and_sources( self._project, env, lib, sources, headers )
		_add_target( self._project, env, lib, str(lib), 'program', source_items, pre )
		return [os.path.join( env['stage_bin'], str(lib) )]
	
	def console_program( self, env, lib, sources, headers=None, pre=None, nopre=None, *keywords ):
		return self.program( env, lib, sources, headers, pre, nopre, *keywords )
	
	def done( self, env, project_name ):
		
		target_file_names = [t.file_name() for t in self._project.targets()]
		for k in env.dependencies.keys():
			evaluated_key = env.subst(k)
			libname = evaluated_key.split('/')[-1]
			target = self._project.target_with_file_name( libname )
			if target is not None and libname in target_file_names:
				deps = env.dependencies[k]
				for dep in deps:
					dep_file_name = env.subst(dep).split('/')[-1]
					dep_target = self._project.target_with_file_name(dep_file_name)
					if dep_target is not None:
						target.dependencies().append(XcodeTargetDependency(dep_target))
						
						dep_proxy = XcodeDependencyProxy(dep_target)
						target.project_dependencies().append(dep_proxy)
		
		for s in self._project.sources():
			_remap_setting_names( s )
		
		# Collect all the targets so that we can create a "All" target in the project
		for t in self._project.targets():
			proxy = XcodeDependencyProxy(t)
			self._project.aggregate_dependency_list().append(proxy)
		
		print 'Generating xcode project files. Products will not be built'
		proj_path = os.path.join( env.root, 'build', 'project' )
		if not os.path.exists( proj_path ) :
			os.makedirs( proj_path )
		
		self._project.write( proj_path, project_name )
		
		return False
		
	def _targetForName( self, name ):
		for t in self._targets :
			if t.name == name : return t
		return None
