# (C) Ardendo SE 2008
#
# Released under the LGPL

import env
import os
import vs

class VsBuilder :
	def __init__( self, vsver ) :
		self.vs_version = vsver
		self.vs_solution = vs.VSSolution()
		self.vs_solution.target_dir = "/build/projects/" + self.vs_version + "/"
		self.vs_solution.relative_root = "./../../../"
		
	def project( self, env, lib, sources, project_type, subsystem=None, headers=None, pre=None, nopre=None, *keywords  ) :
	
		self.vs_solution.root = env.root
		
		if env['debug'] == '1': 
			config_name = 'Debug|Win32'
		else : config_name = 'Release|Win32'
			
		curr_cfg = vs.BuildConfiguration(lib, config_name, target_type= project_type )
		
		curr_cfg.compiler_options.sources_not_using_precomp = nopre
		
		if( pre ) : curr_cfg.compiler_options.precomp_headers = True, pre[1], pre[0]
		
		# print env.Dump()
		
		if env.has_key('LIBPATH') : curr_cfg.linker_options.additional_library_directories = env['LIBPATH']
		if env.has_key('LIBS') : curr_cfg.linker_options.additional_dependencies = env['LIBS']
		if env.has_key('CPPPATH') : curr_cfg.compiler_options.include_directories = env['CPPPATH']
		if env.has_key('CPPDEFINES') : curr_cfg.compiler_options.preprocessor_flags = env['CPPDEFINES']
		if config_name == 'Release|Win32' : curr_cfg.compiler_options.additional_options += "/O2"
		
		curr_cfg.set_vc_version(self.vs_version)
		curr_cfg.output_directory = env.subst('$stage_bin')
		curr_cfg.intermediate_directory = "$(SolutionDir)tmp" + "\\" + lib + "\\" + config_name.split("|")[0]
		
		curr_cfg.linker_options.target_type = project_type
		curr_cfg.linker_options.sub_system = subsystem
		
		existing_project = self.vs_solution.find_project_by_name( lib )
		
		if( existing_project ) : existing_project.configurations.append(curr_cfg)
		else :
			vcproj = vs.VSProject(  name = lib, 
									root_dir = env.root,
									relative_path = env.relative_path,
									configurations = [ curr_cfg ], 
									header_files = headers, 
									source_files = sources, 
									vc_version = self.vs_version )
														
			vcproj.file_name = lib + '.vcproj'
			# print vcproj.file_system_location
			self.vs_solution.projects.append(vcproj)

		dll_file = os.path.join( env['stage_bin'], lib + '.dll')
		lib_file = os.path.join( env['stage_libdir'], lib + '.dll')
		exp_file = os.path.join( env['stage_libdir'], lib + '.dll')
		pdb_file = os.path.join( env['stage_bin'], lib + '.dll')
		
		return [ dll_file, lib_file, exp_file, pdb_file]
		
	def shared_library( self, env, lib, sources, project_type="dll", headers=None, pre=None, nopre=None, *keywords ):
		return self.project(env, lib, sources, "dll", headers, pre, nopre, *keywords)
		
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
		return False
		
