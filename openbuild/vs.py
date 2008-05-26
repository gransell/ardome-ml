import xml.dom.minidom
import uuid
import re
import StringIO


class CompilerOptions:
	""" A class that holds options related to the compiler tool in visual studio.
	
		This class is used by the BuildConfiguration class
		and is used to create the VCCLCompilerTool in the vcproj
		files."""
		
	def __init__(self) :
		""" Default constructor """
									
		self.preprocessor_flags = ""
		self.include_directories = []
		self.wchar_t_is_built_in = False
		self.runtime_type_info = True
		self.precomp_headers = True, "precompiled_headers.h", "precompiled_headers.cpp"
		self.sources_not_using_precomp = []
		self.warning_level = 3
		self.additional_options = ""
		
	def runtime_library( self, config_name ) :
		""" Returns the c++-runtime library to use
			
			This can be either:
				single or multi-threaded, 
				debug or release
				static or dll
			
			The function looks at the name of the configuration, 
			and relies on it having the word debug in it to determine
			which run-time to use. Visual Studio have the following convetion:
			0 - Multi Threaded Static 
			1 - Mulit Threaded Debug Static 
			2 - Multi Threaded Release DLL 
			3 - Multi Threaded Debug DLL 
			4 - Single Threaded Release  
			5 - Single Threaded Debug
			
			Keyword arguments:
			config_name --  [string] The name of the configuration. Should have 
							debug in it if its supposed to use a debug runtime. """
		m = re.compile("debug",re.I).search(config_name)
		if( m ) : return "3"
		return "2"

	def runtime_type_info_as_string( self ) :
		""" Returns the string 'TRUE' if runtime type info is going to be used, otherwise 'FALSE'"""
		if self.runtime_type_info  : return "TRUE"
		return "FALSE"

	def use_precompiled_headers( self ) :
		""" Returns a number as a string that tells Visual Studio if precompiled headers are going to be used.
		
			0 - Not using precomiled headers
			1 - Create precompiled headers (only used on the cpp-file that will create the pch-file)
			2 - Autiomatic use (not recommeded)
			3 - Use precompiled headers """	
		if self.precomp_headers[0] : return "3"
		return "0"

	def precompiled_header_file( self ) :
		""" Returns the name of the precompiled header file, if precompiled headers are used. """
		if not self.use_precompiled_headers() : return ""
		return self.precomp_headers[1]
			
	def precompiled_header_source_file( self, file_name ) :
		""" Returns a tuple of an integer and a file name
		
			The first number in the tuple is 1 if precompiled headers is used.
			The second string in the tuple is the name of the header file
			that generates the precompiled header file. 
			
			Keyword arguments:
			file_name -- The source file name for which the precompiled header flags 
						 needs to be set.   
			"""
		if self.use_precompiled_headers() == "0" : return 0, ""
		
		if self.sources_not_using_precomp :
			for src in self.sources_not_using_precomp :
				if file_name in src : return 0, ""
			
		if self.precomp_headers[2] in file_name : return 1, self.precomp_headers[1]
		return  3, self.precomp_headers[1]

	def debug_information_format( self, config_name ) :
		# 0 - Disabled
		# 1 - C7 Compatible
		# 2 - Line numbers only
		# 3 - Program Database
		# 4 - Program Database for Edit and Continue
		m = re.compile("debug",re.I).search(config_name)
		if( m ) : return "4"
		return "3"
			
	def include_directories_as_string( self ) :
		res = ""
		for lib in self.include_directories:
			if( len(res) > 0 ) : res += ";"
			res += "&quot;" + lib.replace("#/", "$(SolutionDir)") + "&quot;"
		return res
		
	def preprocessor_flags_as_string( self ) :
		res = ""
		for flag in self.preprocessor_flags:
			if( len(res) > 0 ) : res += ";"
			res += flag
		return res
		
class LinkerOptions:
	def __init__(self, name , target_type  ) :
		self.additional_library_directories = []
		self.additional_dependencies = []
		self.generate_debug_info = 1
		self.output_file = "$(OutDir)\\" + name + "." + target_type
		self.program_database_file = "$(OutDir)\\" + name + ".pdb"
		self.target_type = target_type
		
		if self.target_type == "dll" : self.import_library = "$(OutDir)\\" + name + ".lib" 
		else : self.import_library = ""
		
		self.sub_system = "windows"
		self.target_machine = "MachineX86"
	
	def incremental_linking( self, config_name ) :
		# 1 - No
		# 2 - Yes
		m = re.compile("debug",re.I).search(config_name)
		if( m ) : return "2"
		return "1"

	def generate_debug_information_as_string( self ) :
		 if( self.generate_debug_info) : return "TRUE"
		 return "FALSE"

	def program_database_file( self ) :
		if not self.generate_debug_info : return ""
		return self.program_database_file

	def subsystem(self) :
		# 1 = Console
		# 2 = Windows
		if self.sub_system  == "windows"  :
			return "2"
		return "1"
		
	def additional_dependencies_as_string( self ) :
		res = ""
		for dep in self.additional_dependencies:
			if len(res) == 0 : res += dep + ".lib"
			else : res += " " + dep + ".lib"
		return res
			
	def additional_library_directories_as_string( self ) :
		res = ""
		for lib in self.additional_library_directories:
			if( len(res) > 0 ) : res += ";"
			res += "&quot;" + lib.replace("#/", "$(SolutionDir)") + "&quot;"
		return res

class BuildConfiguration:
	def __init__( self, product_name, config_name, target_type = "dll" ):
		"""
			Keyword arguments:
			product_name -- [string] This will be used as the base for the exe or
							dll that will be created by visual studio project.
							An example could be 'amf_layout' """
		self.name = config_name
		self.product_name = product_name
		self.output_directory = ""
		self.intermediate_directory = ""
		self.target_type = target_type
		self.character_set = "unicode"
		self.compiler_options = CompilerOptions( product_name )
		self.linker_options = LinkerOptions( product_name, target_type )
		
	def set_vc_version( self, vc_version ):
		self.vc_version = vc_version
		self.output_directory = "$(SolutionDir)bin_" + self.vc_version + "\\" + self.name.replace("|","-")
		self.intermediate_directory = "$(SolutionDir)tmp_" + self.vc_version + "\\" + self.product_name + "\\" + self.name.replace("|","-") 
	 
	def binary_type( self ) :
		if( self.target_type == "dll") : return "2"
		if( self.target_type == "exe") : return "1"
		return "0"

	def character_type( self ) :
		if( self.character_set == "unicode") : return "1"
		return "0"
		
class VSProject :
	def __init__( self, name = "", configurations = None, header_files=None, source_files=None, vc_version="vc71" ) :
		self.name = name
		self.configurations = configurations
			
		self.header_files = header_files
		self.cpp_files = source_files
		self.vc_version = vc_version
		self.file_system_location = ""
		
		for conf in self.configurations:
			conf.set_vc_version( self.vc_version )
			if conf.compiler_options.precomp_headers[0] and conf.compiler_options.precomp_headers[2] not in self.cpp_files:
				self.cpp_files.append(conf.compiler_options.precomp_headers[2]) 
		
	def __str__(self) :
		os = StringIO.StringIO();
		os.write("<?xml version=\"1.0\" encoding=\"Windows-1252\"?>\n")
		self.handle_project(os)
		return os.getvalue()
		
	   
	def handle_project(self, os):
		project_header = """<VisualStudioProject	 
				ProjectType="Visual C++"
				Version="7.10"
				Name="%s"
				ProjectGUID="%s"
				RootNamespace="%s"
				Keyword="Win32Proj" >
		<Platforms>
			<Platform Name="Win32"/>
		</Platforms>"""

		os.write( project_header % (self.name , uuid.uuid4() , self.name) )

		os.write("\n<Configurations>\n")
		for config in self.configurations:
			self.handle_configuration(config, os)
			
		os.write("</Configurations>\n")
		os.write("<References />\n")

		os.write("<Files>\n")
		self.handle_cpp_files(os)
		self.handle_h_files(os)
		os.write("</Files>\n")
		os.write("<Globals/>\n")
		os.write("</VisualStudioProject>\n")

	def handle_configuration( self, config, os):
		config_start = """<Configuration	Name="%s"
			OutputDirectory="%s"
			IntermediateDirectory="%s"
			ConfigurationType="%s"
			CharacterSet="%s" >"""
		config_name = config.name
		os.write("\n");
		os.write( config_start % (  config.name, config.output_directory, config.intermediate_directory, 
									config.binary_type(), config.character_type() ) )

		self.handle_compiler(config, os)
		self.handle_linker(config, os)

		os.write("</Configuration>\n")


	def handle_compiler( self, config, os ) :
		compiler_tool = """ <Tool
					Name="VCCLCompilerTool"
					AdditionalOptions="%s"
					AdditionalIncludeDirectories="%s"
					PreprocessorDefinitions="%s"
					RuntimeLibrary="%s"
					RuntimeTypeInfo="%s"
					UsePrecompiledHeader="%s"
					PrecompiledHeaderThrough="%s"
					WarningLevel="%s"
					Detect64BitPortabilityProblems="FALSE"
					DebugInformationFormat="%s"/> """

		os.write( compiler_tool % ( config.compiler_options.additional_options,
								config.compiler_options.include_directories_as_string(),
								config.compiler_options.preprocessor_flags_as_string(),
								config.compiler_options.runtime_library( config.name ),
								config.compiler_options.runtime_type_info_as_string(),
								config.compiler_options.use_precompiled_headers(),
								config.compiler_options.precompiled_header_file(),
								config.compiler_options.warning_level,
								config.compiler_options.debug_information_format( config.name) ) )
								
   

	def handle_linker( linker, config, os ) :
		linker_tool = """ <Tool
			Name="VCLinkerTool"
			AdditionalDependencies="%s"
			OutputFile="%s"
			LinkIncremental="%s"
			AdditionalLibraryDirectories="%s"
			GenerateDebugInformation="%s"
			ProgramDatabaseFile="%s"
			SubSystem="%s"
			ImportLibrary="%s"
			TargetMachine="%s"/> """

		os.write ( linker_tool % (   config.linker_options.additional_dependencies_as_string(),
								config.linker_options.output_file,
								config.linker_options.incremental_linking( config.name),
								config.linker_options.additional_library_directories_as_string(),
								config.linker_options.generate_debug_information_as_string(),
								config.linker_options.program_database_file,
								config.linker_options.subsystem(),
								config.linker_options.import_library,
								config.linker_options.target_machine ) )

	def create_filter( self, filter_name, file_types ) :
		filter_element = """<Filter
				Name="%s"
				Filter="%s"
				UniqueIdentifier="{%s}"> """
		return filter_element % ( filter_name, file_types, uuid.uuid4() )


	def handle_file_configuration( self, cpp_file, config, os ) :
		config_element = """<FileConfiguration Name="%s">
			<Tool
				Name="VCCLCompilerTool"
				UsePrecompiledHeader="%d"
				PrecompiledHeaderThrough="%s"/>
			</FileConfiguration>"""
		precomp_flag, precomp_file = config.compiler_options.precompiled_header_source_file( cpp_file )
		os.write( config_element % ( config.name, precomp_flag, precomp_file) )

	def handle_src_file( self, cpp_file, os ) :
		os.write( "<File RelativePath=\".\\%s\" >\n" % cpp_file.replace("/", "\\") )

		for config in self.configurations :
			self.handle_file_configuration(cpp_file, config, os)

		os.write( "</File>\n" )

	def handle_header_file( self, h_file_path, os ) :
		os.write( "<File RelativePath=\".\\%s\" />\n" % h_file_path  )

	def handle_cpp_files( self, os ) :
		os.write( self.create_filter("Source Files", "cpp;c;cxx") + "\n")
		if self.cpp_files :
			self.cpp_files.sort()
			for cpp_file in self.cpp_files:
				self.handle_src_file(cpp_file, os)
		os.write("</Filter>\n")

	def handle_h_files( self, os ) :
		os.write( self.create_filter("Header Files", "h;hpp;hxx") + "\n")
		if self.header_files:
			self.header_files.sort()
			for h_file in self.header_files :
				self.handle_header_file(h_file, os)
		os.write( "</Filter>\n" )

class VSSolution :
	def __init__(self) :
		self.projects = []
		
	def find_project_by_name( self, project_name ) :
		for proj in self.projects:
			if proj.name == project_name : return proj
		return None
	 
	def create_sln_and_vcproj_files( self, sln_file_name ) :
		#Start to save projects
		for proj in self.projects:
			f = open( proj.file_system_location, "w");
			f.write( str(proj) );
			f.close();
		# Now, build the solution-file
		sln = open(sln_file_name, "w")
		visual_studio_guid = "8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942"
		sln.write("Microsoft Visual Studio Solution File, Format Version 8.00\n")
		for proj in self.projects:
			project_element = """Project("{%s}") = "%s", "%s", "{%s}" """
			sln.write( project_element % (visual_studio_guid, proj.name, proj.file_system_location, uuid.uuid4() ) )
			sln.write( "\n" )
			sln.write("\tProjectSection(ProjectDependencies) = postProject\n")
			sln.write("\tEndProjectSection\n")
			sln.write("EndProject\n")
			
		sln.write("Global\nEndGlobal\n")
		sln.close()
			
