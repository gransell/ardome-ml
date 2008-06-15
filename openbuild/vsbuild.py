# (C) Ardendo SE 2008
#
# Released under the LGPL

import env
import os
import vs

class VsBuilder :
	def __init__( self, vsver ) :
		print "Generating Visual Studio sln and vcproj files. scons will not build."
		self.vs_version = vsver
		self.vs_solution = vs.VSSolution()
		self.vs_solution.target_dir = "/build/projects/" + self.vs_version + "/"
		self.vs_solution.relative_root = "./../../../"
		
	def split_files( self, filterstr, sources ) :
		suffixes = tuple( filterstr.split(";") )
		notmatching = []
		matching = []		
		for str in sources:
			ppos = str.rfind(".")	
			if ppos != -1 : 	
				if str[ppos+1:] in suffixes : 
					matching.append(str)
				else : notmatching.append(str)
			else : notmatching.append(str)
		return notmatching, matching
		
	def prepare_source_files( self, config_name, sources, env, name, existing_files ) :
		""" This function allows compiler and link flags specific for a particular
			file to be added. Normally this is not needed, but for instance to avoid 
			the use of precompiled_headers for a file, this is needed. 
			
			Keyword arguments:
			config_name -- The name of the configuration for which the flags will be used
			sources -- All the source files for this configuration.
			env -- The current environment 
			name -- The name of the target that is going to be built
			existing_files -- If this function has been called before, 
								these are the SourceFiles created by the last call.
								Instead of creating new SourceFiles, the dictionary 
								keept for each file must be updated.
			"""
		if sources == None : return []
		
		notmatching, matching = self.split_files( "cpp;cc;cxx", sources )
		
		if existing_files == None :
			res = []
			for s in notmatching:
				res.append( vs.SourceFile( s, { config_name:''} ) )
			
			for s in matching:
				# Here we have cpp-files, add /Fo flags ...
				# tmppath = env.temporary_build_path(name) + "\\" + os.path.dirname(s)
				extra_flags = ''
				res.append( vs.SourceFile( s, { config_name: extra_flags } ) )
			return res
		else:
			for e in existing_files :
				if e.name in matching :
					# tmppath = env.temporary_build_path(name) + "\\" + os.path.dirname(e.name)
					extra_flags = ''
					e.additional_flags[config_name] = extra_flags
			return []

	def temporary_output_path( self, env, sources, name  ) :
		""" Selects the output path where most of the source-files
			will be built. This is to be able to share obj-files with
			scons. But since the procompiled header file always is regenerated, 
			it doesn't have any impact."""
		notmatching, matching = self.split_files( "cpp;cc;cxx", sources )
		possible_paths = {}
		for s in matching :
			tmppath = os.path.join( env.temporary_build_path(name), os.path.dirname(s))
			if possible_paths.has_key( tmppath ) : possible_paths[ tmppath ] += 1
			else : possible_paths[ tmppath] = 1
			
		best_path = ""
		best_count = -1
		for k, v in possible_paths.iteritems() :
			if v > best_count :
				best_path = k
				best_count = v
			
		return best_path
		
	def add_lib( self, libs ) :
		res = []
		for lib in libs:
			if not lib.endswith('.lib') : res.append( lib + '.lib')
			else : res.append(lib)
		return res
			
	def project( self, env, lib, sources, project_type, subsystem=None, headers=None, pre=None, nopre=None, *keywords  ) :
	
		self.vs_solution.root = env.root
		
		if env['debug'] == '1': config_name = 'Debug|Win32'
		else : config_name = 'Release|Win32'
			
		curr_cfg = vs.BuildConfiguration(lib, config_name, target_type= project_type )
		
		curr_cfg.compiler_options.sources_not_using_precomp = nopre

		
		if pre is not None : 
			curr_cfg.compiler_options.precomp_headers = True, pre[1], pre[0]
			sources.append( pre[0] )
			
		# print env.Dump()
		
		if env.has_key('LIBPATH') : curr_cfg.linker_options.additional_library_directories = env['LIBPATH']
		if env.has_key('LIBS') : curr_cfg.linker_options.additional_dependencies = self.add_lib( env['LIBS'] )
		if env.has_key('CPPPATH') : curr_cfg.compiler_options.include_directories = env['CPPPATH']
		if env.has_key('CPPDEFINES') : curr_cfg.compiler_options.preprocessor_flags = env['CPPDEFINES']
		if config_name == 'Release|Win32' : curr_cfg.compiler_options.additional_options += "/O2"
		
		curr_cfg.set_vc_version(self.vs_version)
		curr_cfg.output_directory = env.subst('$stage_bin')
		curr_cfg.intermediate_directory = self.temporary_output_path(env, sources, lib ) 
		
		curr_cfg.linker_options.target_type = project_type
		curr_cfg.linker_options.sub_system = subsystem
		
		existing_project = self.vs_solution.find_project_by_name( lib )
		
		if( existing_project ) : 
			existing_project.configurations.append(curr_cfg)
			self.prepare_source_files( config_name, sources, env, lib, existing_project.cpp_files )
		else :
			vcproj = vs.VSProject(  name = lib, 
									root_dir = env.root,
									relative_path = env.relative_path,
									configurations = [ curr_cfg ], 
									header_files = headers, 
									source_files = self.prepare_source_files(config_name, sources, env, lib, None), 
									vc_version = self.vs_version )
														
			vcproj.file_name = lib + '.vcproj'
			# print vcproj.file_system_location
			self.vs_solution.projects.append(vcproj)

		dll_file = os.path.join( env['stage_bin'], lib + '.dll')
		lib_file = os.path.join( env['stage_libdir'], lib + '.dll')
		exp_file = os.path.join( env['stage_libdir'], lib + '.dll')
		pdb_file = os.path.join( env['stage_bin'], lib + '.dll')
		
		return [ dll_file, lib_file, exp_file, pdb_file]
		
	def shared_library( self, env, lib, sources, headers=None, pre=None, nopre=None, *keywords ):
		return self.project(env=env, lib=lib, sources=sources, project_type="dll", subsystem=None, headers=headers, pre=pre, nopre=nopre, *keywords)
		
	def plugin( self, env, lib, sources, headers=None, pre=None, nopre=None, *keywords ):
		return self.project(env, lib, sources, "dll", None, headers, pre, nopre, *keywords)
		
	def program( self, env, lib, sources, headers=None, pre=None, nopre=None, *keywords ):
		return self.project(env, lib, sources, "exe", "windows", headers, pre, nopre, *keywords)
		
	def console_program( self, env, lib, sources, headers=None, pre=None, nopre=None, *keywords ):
		return self.project(env, lib, sources, "exe", "console", headers, pre, nopre, *keywords)
		
	def done( self, env , project_name ) :
		
		if project_name == None :
			project_name = 'solution'
		self.vs_solution.create_sln_and_vcproj_files( project_name + '.sln', env.dependencies, self.vs_version )
		print "Done generating sln and vcproj files in build/projects/" + self.vs_version
		return False
		
