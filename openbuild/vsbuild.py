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
		for src in sources:
			str = src
			if isinstance( src, vs.SourceFile ) : str = src.name
			ppos = str.rfind(".")	
			if ppos != -1 : 	
				if str[ppos+1:] in suffixes : 
					matching.append(src)
				else : notmatching.append(src)
			else : notmatching.append(src)
		return notmatching, matching
		
	def prepare_files( self, config_name, headers, env, name, existing_files ) :
		if headers == None : return []
		notmatching, matching = self.split_files( "hpp;h;hxx;rc;qrc", headers )
		res = []
		if existing_files == None :
			for hf in matching :
				if not isinstance(hf, vs.SourceFile ) :
					sf = vs.SourceFile( hf, { config_name:''} )
					sf.tools[config_name] = []
					res.append( sf )
				else : res.append(hf)
			return res
		else :
			for sf in existing_files :
				if sf.name in matching :
					sf.tools[ config_name ] = []
			return []
		
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
				if not isinstance( s, vs.SourceFile ):
					sf = vs.SourceFile( s, { config_name:''} ) 
					res.append( sf )
				else : res.append( s )
			
			for s in matching:
				if not isinstance( s, vs.SourceFile ):
					extra_flags = ''
					sf = vs.SourceFile( s, { config_name: extra_flags } )
				else : sf = s
				sf.tools[ config_name ].append( vs.CompilerTool() )
				res.append( sf )
			return res
		else:
			for sf in existing_files :
				if sf.name in matching :
					extra_flags = ''
					sf.additional_flags[config_name] = extra_flags
					sf.tools[ config_name ] = [ vs.CompilerTool() ]
			return []

	def temporary_output_path( self, env, sources, name  ) :
		""" Selects the output path where most of the source-files
			will be built. This is to be able to share obj-files with
			scons. But since the procompiled header file always is regenerated, 
			it doesn't have any impact."""
		notmatching, matching = self.split_files( "cpp;cc;cxx", sources )
		possible_paths = {}
		for s in matching :
			tmppath = os.path.join( env.temporary_build_path(name), os.path.dirname( str(s) ))
			tmppath = tmppath.replace("/","\\")
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
			
	def project( self, env, lib, sources, project_type, subsystem=None, headers=None, pre=None, nopre=None, extra_compiler_flags = None  ) :
	
		self.vs_solution.root = env.root
		
		if env['debug'] == '1': config_name = 'Debug|Win32'
		else : config_name = 'Release|Win32'
			
		curr_cfg = vs.BuildConfiguration(lib, config_name, target_type= project_type )
		
		curr_cfg.compiler_options.sources_not_using_precomp = self.prepare_source_files(config_name, nopre, env, lib, None) 
		
		if pre is not None : 
			curr_cfg.compiler_options.precomp_headers = True, pre[1], pre[0]
			sources.append( pre[0] )
			
		if env.has_key('LIBPATH') : curr_cfg.linker_options.additional_library_directories = env['LIBPATH']
		if env.has_key('LIBS') : curr_cfg.linker_options.additional_dependencies = self.add_lib( env['LIBS'] )
		if env.has_key('CPPPATH') : curr_cfg.compiler_options.include_directories = env['CPPPATH']
		if env.has_key('CPPDEFINES') : curr_cfg.compiler_options.preprocessor_flags = env['CPPDEFINES']
		if config_name == 'Release|Win32' : curr_cfg.compiler_options.additional_options += "/O2"
		
		if extra_compiler_flags is not None:
			curr_cfg.compiler_options.additional_options += ' ' + extra_compiler_flags
		
		curr_cfg.set_vc_version(self.vs_version)
		curr_cfg.output_directory = env.subst('$stage_bin')
		curr_cfg.intermediate_directory = self.temporary_output_path(env, sources, lib ) 
		
		curr_cfg.linker_options.target_type = project_type
		curr_cfg.linker_options.sub_system = subsystem
		
		existing_project = self.vs_solution.find_project_by_name( lib )
		
		if( existing_project ) : 
			existing_project.configurations.append(curr_cfg)
			self.prepare_source_files( config_name, sources, env, lib, existing_project.cpp_files )
			self.prepare_files( config_name, headers, env, lib, existing_project.header_files )
		else :
			vcproj = vs.VSProject(  name = lib, 
									root_dir = env.root,
									relative_path = env.relative_path,
									configurations = [ curr_cfg ], 
									header_files = self.prepare_files(config_name, headers, env, lib, None),
									source_files = self.prepare_source_files(config_name, sources, env, lib, None), 
									vc_version = self.vs_version )
														
			vcproj.file_name = lib + '.vcproj'
			self.vs_solution.projects.append(vcproj)

		dll_file = os.path.join( env['stage_bin'], lib + '.dll')
		lib_file = os.path.join( env['stage_libdir'], lib + '.lib')
		exp_file = os.path.join( env['stage_libdir'], lib + '.exp')
		pdb_file = os.path.join( env['stage_bin'], lib + '.pdb')
		
		return [ dll_file, lib_file, exp_file, pdb_file]
		
	def shared_library( self, env, lib, sources, headers=None, pre=None, nopre=None, *keywords ):
		return self.project(env=env, lib=lib, sources=sources, project_type="dll", subsystem=None, headers=headers, pre=pre, nopre=nopre )
		
	def plugin( self, env, lib, sources, headers=None, pre=None, nopre=None, *keywords ):
		return self.project(env, lib, sources, "dll", None, headers, pre, nopre)
		
	def program( self, env, lib, sources, headers=None, pre=None, nopre=None, *keywords ):
		return self.project(env, lib, sources, "exe", "windows", headers, pre, nopre)
		
	def console_program( self, env, lib, sources, headers=None, pre=None, nopre=None, *keywords ):
		return self.project(env, lib, sources, "exe", "console", headers, pre, nopre)
		
	def qt_program( self, env, lib, moc_files, sources, resources=None, headers=None, pre=None, nopre=None, *keywords ) : 
	
		if headers == None : headers = []
		if resources == None : resources = []
		if nopre == None : nopre = []
		
		sconscript_path = os.path.join( env.root, env.relative_path_to_sconscript ).replace("/", os.sep)
		
		moc_command = env[ 'QT4_MOC' ].replace( '/', os.sep )
		for mocf in moc_files :
			sf = vs.SourceFile( mocf )
			moc_tool = vs.QtMocTool( moc_command, sconscript_path )
			sf.tools['all_configurations'] = [ moc_tool ]
			cpp_file = moc_tool.output_file( sf ) 
			sources.append(  cpp_file[ len(sconscript_path) + 1: ]  )
			headers.append( sf )
			# nopre.append(  moc_tool.output_file( sf ) )
			
		rcc_command = env[ 'QT4_RCC' ].replace( '/', os.sep )
			
		for resf in resources :
			sf = vs.SourceFile( resf )
			rcc_tool = vs.QtRccTool( rcc_command, sconscript_path )
			sf.tools['all_configurations'] = [ rcc_tool ]
			cpp_file = rcc_tool.output_file( sf )
			sources.append( cpp_file[ len(sconscript_path) + 1 : ] )
			headers.append( sf )
			nopre.append( rcc_tool.output_file( sf ) )
			
		extra_compiler_flags = ''
		if self.vs_version == 'vs2003':
			extra_compiler_flags = '/Zm800'
			
		return self.project(env, lib, sources, "exe", "windows", headers, pre, nopre, extra_compiler_flags )

		
	def done( self, env , project_name ) :
		
		if project_name == None :
			project_name = 'solution'
		self.vs_solution.create_sln_and_vcproj_files( project_name + '.sln', env.dependencies, self.vs_version )
		print "Done generating sln and vcproj files in build/projects/" + self.vs_version
		return False
		
