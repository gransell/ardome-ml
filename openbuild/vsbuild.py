import env
import os
import vs

class VsBuilder :
	def __init__( self, vsver ) :
		self.vs_version = vsver
		self.vs_solution = vs.VSSolution()
		
	def dll_project( self, env, lib, sources, headers=None, pre=None, nopre=None, *keywords  ) :
		
		if env['debug'] == '1': 
			config_name = 'Debug|Win32'
		else : config_name = 'Release|Win32'
			
		curr_cfg = vs.BuildConfiguration(lib, config_name, target_type="dll" )
		
		curr_cfg.compiler_options.sources_not_using_precomp = nopre
		
		if( pre ) : curr_cfg.compiler_options.precomp_headers = True, pre[1], pre[0]
		
		# print env.Dump()
		
		curr_cfg.linker_options.additional_library_directories = env['LIBPATH']
		curr_cfg.linker_options.additional_dependencies = env['LIBS']
		curr_cfg.compiler_options.include_directories = env['CPPPATH']
		curr_cfg.compiler_options.preprocessor_flags = env['CPPDEFINES']
		
		curr_cfg.set_vc_version(self.vs_version)
		curr_cfg.output_directory = env['stage_libdir']
		curr_cfg.intermediate_directory = "$(SolutionDir)bin_" + self.vs_version + "\\" + config_name.replace("|","-")
		
		existing_project = self.vs_solution.find_project_by_name( lib )
		
		if( existing_project ) : existing_project.configurations.append(curr_cfg)
		else :
			vcproj = vs.VSProject(  name = lib, 
									root_dir = env.root,
									full_path = env.full_path,
									configurations = [ curr_cfg ], 
									header_files = headers, 
									source_files = sources, 
									vc_version = self.vs_version )
														
			vcproj.file_system_location = str( env.File( lib + '_scons.vcproj') )
			print vcproj.file_system_location
			self.vs_solution.projects.append(vcproj)

		dll_file = os.path.join( env['stage_libdir'], lib + '.dll')
		lib_file = os.path.join( env['stage_libdir'], lib + '.dll')
		exp_file = os.path.join( env['stage_libdir'], lib + '.dll')
		pdb_file = os.path.join( env['stage_libdir'], lib + '.dll')
		
		return [ dll_file, lib_file, exp_file, pdb_file]
		
	def shared_library( self, env, lib, sources, headers=None, pre=None, nopre=None, *keywords ):
		return self.dll_project(env, lib, sources, headers, pre, nopre, *keywords)
		
	def plugin( self, env, lib, sources, headers=None, pre=None, nopre=None, *keywords ):
		return self.dll_project(env, lib, sources, headers, pre, nopre, *keywords)
		
	def done( self, env ) :
		print "Done!"
		sln_file_name = str(env.File('main_scons_' + self.vs_version + '.sln' ))
		self.vs_solution.create_sln_and_vcproj_files(sln_file_name)
		
