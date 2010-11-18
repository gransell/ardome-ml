# (C) Ardendo SE 2008
#
# Released under the LGPL
 
import xml.dom.minidom
import uuid
import re
import StringIO
import os
import codecs
 
class CompilerTool:
	""" Will use the cl.exe to compile a file """
	def to_tool_xml( self, the_file, config, vsver ) :
		""" Keyword arguments:
			the_file -- The file (as a SourceFile) that will be built.
			config -- The current BuildConfiguration
			vsver -- The currently used version of visual studio"""
		toolstr = """ <Tool
				Name="VCCLCompilerTool"
				UsePrecompiledHeader="%d"
				PrecompiledHeaderThrough="%s"
				AdditionalOptions="%s"/> """
				
		precomp_flag, precomp_file = config.compiler_options.precompiled_header_source_file( the_file )
		
		if vsver == 'vs2008':
			if precomp_flag == 3 : precomp_flag = 2
		
		return toolstr % ( precomp_flag, os.path.basename( precomp_file ), the_file.get_additional_cl_flags(config.name) ) 
		
class QtMocTool :
	""" Will use moc.exe to create class meta-data """
	def __init__( self, qtmoc_full_path, root_path ) :
		""" Keyword arguments:
			qtmoc_full_path -- The full path to moc.exe
			root_path -- The path to the SConstruct file """
		self.qtmoc = qtmoc_full_path
		self.root_path = root_path
		
	def output_file( self, the_file ) :
		full_path = os.path.join( self.root_path, the_file.name )
		res = full_path[ : full_path.rfind(".") ] + "_moc.cpp"
		return res.replace("/", os.sep)
		
	def to_tool_xml( self, the_file, config, vsver ) :
		""" Creates a custom build moc tool. """
		
		# The -f flag forces the moc-compiler to insert an include at the top of the file.
		# The -o flag specifies the output file that moc will create """
		toolstr = """ 	<Tool
						Name="VCCustomBuildTool"
						Description="Running Qt moc tool on [%s] to create class meta-data"
						CommandLine="&quot;%s&quot; %s -f&quot;%s&quot; -o&quot;%s&quot; &quot;%s&quot;"
						Outputs="&quot;%s&quot;"/> """
						
		precomp_flag, precomp_file = config.compiler_options.precompiled_header_source_file( the_file )
		if precomp_flag != 0 : precomp_file = "-f&quot;%s&quot;" % (precomp_file)
		else: precomp_file = ""
		input_file = os.path.join( self.root_path, the_file.name ).replace("/", os.sep)
		output_file = self.output_file( the_file )
		return toolstr % ( the_file.name, self.qtmoc, precomp_file, input_file, output_file, input_file, output_file )
		
class QtRccTool : 
	def __init__( self, qtrcc_full_path, root_path ) :
		self.qtrcc = qtrcc_full_path
		self.root_path = root_path
		
	def output_file( self, the_file ) :
		full_path = os.path.join( self.root_path, the_file.name )
		res = full_path[ : full_path.rfind(".") ] + "_rcc.cpp"
		return res.replace("/", os.sep)
		
	def to_tool_xml( self, the_file, config, vsver ) :
		toolstr = """ 	<Tool
						Name="VCCustomBuildTool"
						Description="Running Qt rcc tool on [%s] to create resources linked into the executable"
						CommandLine="&quot;%s&quot; -o &quot;%s&quot; &quot;%s&quot;"
						Outputs="&quot;%s&quot;"/> """
						
		input_file = os.path.join( self.root_path, the_file.name ).replace("/", os.sep)
		output_file = self.output_file( the_file )
		return toolstr % ( the_file.name, self.qtrcc, output_file, input_file, output_file )

class SourceFile:
	def __init__( self, name, addflags = None ) :
		self.name = name
		self.tools = { 'Debug|Win32': [] , 'Release|Win32': [] }
		if addflags == None :
			self.additional_flags = { 'Debug|Win32':'', 'Release|Win32':''}
		else:
			self.additional_flags = addflags
			
	def __str__( self ) :
		return self.name
		
	def get_additional_cl_flags( self, config ) :
		return self.additional_flags[ config ]
		
	def get_tools( self, config ) :
		res = []
		if self.tools.has_key( config.name ):
			res += self.tools[ config.name ] 
		if self.tools.has_key( 'all_configurations' ) :
			res += self.tools['all_configurations'] 
		return res
		
class VS2003:
	
	def version_string( self ) :
		return "7.10"
		
	def compiler_tool( self, project, config, options ):
		tool_str = """ <Tool
					Name="VCCLCompilerTool"
					AdditionalOptions="%s"
					Optimization="%s"
					AdditionalIncludeDirectories="%s"
					PreprocessorDefinitions="%s"
					RuntimeLibrary="%s"
					TreatWChar_tAsBuiltInType="%s"
					RuntimeTypeInfo="%s"
					UsePrecompiledHeader="%s"
					PrecompiledHeaderThrough="%s"
					WarningLevel="%s"
					Detect64BitPortabilityProblems="FALSE"
					DebugInformationFormat="%s"/> """
					
		return tool_str % ( options.additional_options,
							options.optimization,
							options.include_directories_as_string( project.root_dir),
							options.preprocessor_flags_as_string(),
							options.runtime_library( config.name ),
							options.runtime_type_info_as_string(),
							options.wchar_t_built_in_as_string(),
							options.use_precompiled_headers(),
							os.path.basename( options.precompiled_header_file() ),
							options.warning_level,
							options.debug_information_format( config.name) )
							
	def file_configuration( self, config, the_file, filetype ) :
		if filetype == "source" or filetype == "header" or filetype == "resource":
			res = "<FileConfiguration Name=\"" + config.name + "\">"
			for tool in the_file.get_tools(config) :
				res += tool.to_tool_xml( the_file, config, "vs2003" )
			res += "</FileConfiguration>\n"
			return res		
		else : return ""
		
	def solution_file_header( self ) :
		return """Microsoft Visual Studio Solution File, Format Version 8.00"""
		
		
class VS2008 :
	
	def version_string( self ) :
		return "9,00"
		
	def compiler_tool( self, project, config, options ):
		tool_str = """ <Tool
					Name="VCCLCompilerTool"
					AdditionalOptions="%s"
					AdditionalIncludeDirectories="%s"
					PreprocessorDefinitions="%s"
					ExceptionHandling="%s"
					RuntimeLibrary="%s"
					RuntimeTypeInfo="%s"
					UsePrecompiledHeader="%s"
					PrecompiledHeaderThrough="%s"
					WarningLevel="%s"
					DebugInformationFormat="%s"/> """

		
		return tool_str % ( options.additional_options,
							options.include_directories_as_string( project.root_dir),
							options.preprocessor_flags_as_string(),
							options.exception_handling(),
							options.runtime_library( config.name ),
							options.runtime_type_info_as_string(),
							options.use_precompiled_headers(),
							os.path.basename( options.precompiled_header_file() ),
							options.warning_level,
							options.debug_information_format( config.name) )
										
	def file_configuration( self, config, the_file, filetype ) :
		if filetype == "source" or filetype == "header" or filetype == "resource":
			res = "<FileConfiguration Name=\"" + config.name + "\">"
			for tool in the_file.get_tools(config) :
				res += tool.to_tool_xml( the_file, config, "vs2008" )
			res += "</FileConfiguration>\n"
			return res		
		else : return ""	
		
	def solution_file_header( self ) :
		return "\nMicrosoft Visual Studio Solution File, Format Version 10.00\n# Visual Studio 2008"
		
		
def create_visual_studio( ver ) :
	if ver == "vs2003" : return VS2003()
	if ver == "vs2008" : return VS2008()
	if ver == "vs2008express" : return VS2008()
	raise Exception, "Unsupported vs version %s " % ver
		

class CompilerOptions:
	""" A class that holds options related to the compiler tool in visual studio.
	
		This class is used by the BuildConfiguration class
		and is used to create the VCCLCompilerTool in the vcproj
		files."""
		
	def __init__(self) :
		""" Default constructor """
									
		self.preprocessor_flags = ""
		self.include_directories = []
		self.runtime_type_info = True
		self.precomp_headers = False, "", ""
		self.sources_not_using_precomp = []
		self.warning_level = 3
		self.additional_options = ""
		self.vc_version = ""
		
	def set_vc_version( self, v ) :
		self.vc_version = v
		
	def get_vc_version( self ) :
		return self.vc_version
		
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
		
	def exception_handling( self ) :
        return "2"  #/EHa - Always use /EHa, even when compiling for native. Without it, if the code is called from a .NET module, 
                    #destructors won't be called if an exception is thrown!

	def runtime_type_info_as_string( self ) :
		""" Returns the string 'TRUE' if runtime type info is going to be used, otherwise 'FALSE'"""
		if self.runtime_type_info  : return "TRUE"
		return "FALSE"
		
	def wchar_t_built_in_as_string( self ) :
		# print self.additional_options
		if self.additional_options.find('/Zc:wchar_t') != -1 :
			return "TRUE"
		else : 
			return "FALSE"

	def use_precompiled_headers( self ) :
		""" Returns a number as a string that tells Visual Studio if precompiled headers are going to be used.
		
			0 - Not using precomiled headers
			1 - Create precompiled headers (only used on the cpp-file that will create the pch-file)
			2 - Autiomatic use (not recommeded)
			3 - Use precompiled headers """	
					
		if self.precomp_headers[0] : 
			if self.vc_version == 'vs2008':
				return "2"
			else:
				return "3"
				
		return "0"

	def precompiled_header_file( self ) :
		""" Returns the name of the precompiled header file, if precompiled headers are used. """
		if not self.use_precompiled_headers() : return ""
		return self.precomp_headers[1]
			
	def precompiled_header_source_file( self, the_file ) :
		""" Returns a tuple of an integer and a file name
		
			The first number in the tuple is 1 if precompiled headers is used.
			The second string in the tuple is the name of the header file
			that generates the precompiled header file. 
			
			Keyword arguments:
			the_file -- The source file name for which the precompiled header flags 
						 needs to be set.   
			"""
		if self.use_precompiled_headers() == "0" : return 0, ""
		
		if self.sources_not_using_precomp :
			for src in self.sources_not_using_precomp :
				if the_file.name in src.name : return 0, ""
			
		if self.precomp_headers[2] in the_file.name : return 1, self.precomp_headers[1]
		
		return int( self.use_precompiled_headers() ), self.precomp_headers[1]


	def debug_information_format( self, config_name ) :
		# 0 - Disabled
		# 1 - C7 Compatible
		# 2 - Line numbers only
		# 3 - Program Database
		# 4 - Program Database for Edit and Continue
		m = re.compile("debug",re.I).search(config_name)
		if( m ) : return "1"
		return "1"
			
	def include_directories_as_string( self, root_dir ) :
		res = ""
		for lib in self.include_directories:
			if len(lib) == 0 : continue
			if len(res) > 0  : res += ";"
			lib = lib.replace("\\\\", "\\")
			lib = lib.replace("#/", root_dir + "\\" )
			lib = lib.replace("/", "\\" )
			res += "&quot;" + lib + "&quot;"
		return res
		
	def preprocessor_flags_as_string( self ) :
		res = ""
		for flag in self.preprocessor_flags:
			if len(flag) == 0 : continue
			flag = flag.replace("\\\"", "")
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
		if self.sub_system == None : return "windows"
		if self.sub_system  == "windows"  :
			return "2"
		return "1"
		
	def additional_dependencies_as_string( self ) :
		res = ""
		for dep in self.additional_dependencies:
			if len(dep) == 0 : continue
			if len(res) == 0 : res += dep 
			else : res += " " + dep 
		return res
			
	def additional_library_directories_as_string( self, root_dir ) :
		res = ""
		for lib in self.additional_library_directories:
			if len(lib) == 0 : continue
			if( len(res) > 0 ) : res += ";"
			lib = lib.replace("\\\\", "\\")
			lib = lib.replace("#/", root_dir + "\\" )
			lib = lib.replace("/", "\\" )
			res += "&quot;" + lib + "&quot;"
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
		self.compiler_options = CompilerOptions()
		self.linker_options = LinkerOptions( product_name, target_type )
		
	def set_vc_version( self, vc_version ):
		self.vc_version = vc_version
		self.compiler_options.set_vc_version( self.vc_version )
		
	 
	def binary_type( self ) :
		if( self.target_type == "dll") : return "2"
		if( self.target_type == "exe") : return "1"
		return "0"

	def character_type( self ) :
		if( self.character_set == "unicode") : return "1"
		return "0"
		
class VSProject :
	def __init__( self, name, root_dir, relative_path, configurations = None, header_files=None, source_files=None, vc_version="vc71" ) :
		""" root_dir -- The directory where the SConstruct file resides
			full_path -- The directory where the SConsript file resides. """
			
		self.name = name
		self.configurations = configurations
		
		if header_files == None : header_files = []
		if source_files == None : source_files = []
		
		# for hf in header_files:
			# if isinstance( hf, SourceFile ) :
				# print "VSProject: hf.tools", hf.name, hf.tools
			# else : print hf
		
		self.header_files = header_files
		self.cpp_files = source_files
		self.vc_version = vc_version
		self.vs = create_visual_studio( vc_version )
		self.file_system_location = ""
		self.relative_path = relative_path
		self.root_dir = root_dir
		
		for conf in self.configurations:
			conf.set_vc_version( self.vc_version )
			#if conf.compiler_options.precomp_headers[0] and conf.compiler_options.precomp_headers[2] not in self.cpp_files:
			#	self.cpp_files.append(conf.compiler_options.precomp_headers[2]) 
		
	def __str__(self) :
		ostr = StringIO.StringIO();
		ostr.write("<?xml version=\"1.0\" encoding=\"Windows-1252\"?>\n")
		self.handle_project(ostr)
		return ostr.getvalue()
		
	   
	def handle_project(self, ostr):
		project_header = """<VisualStudioProject	 
				ProjectType="Visual C++"
				Version="%s"
				Name="%s"
				ProjectGUID="%s"
				RootNamespace="%s"
				Keyword="Win32Proj" >
		<Platforms>
			<Platform Name="Win32"/>
		</Platforms>"""

		ostr.write( project_header % ( self.vs.version_string() , self.name , uuid.uuid4() , self.name) )

		ostr.write("\n<Configurations>\n")
		for config in self.configurations:
			self.handle_configuration(config, ostr)
			
		ostr.write("</Configurations>\n")
		ostr.write("<References />\n")

		ostr.write("<Files>\n")
		self.handle_cpp_files(ostr)
		self.handle_h_files(ostr)
		self.handle_rc_files(ostr)
		ostr.write("</Files>\n")
		ostr.write("<Globals/>\n")
		ostr.write("</VisualStudioProject>\n")

	def handle_configuration( self, config, ostr):
		config_start = """<Configuration	Name="%s"
			OutputDirectory="%s"
			IntermediateDirectory="%s"
			ConfigurationType="%s"
			CharacterSet="%s" >"""
		config_name = config.name
		ostr.write("\n");
		ostr.write( config_start % (  config.name, config.output_directory, config.intermediate_directory, 
									config.binary_type(), config.character_type() ) )

		self.handle_compiler(config, ostr)
		self.handle_linker(config, ostr)
		self.handle_resource_compiler( config, ostr )

		ostr.write("</Configuration>\n")


	def handle_compiler( self, config, ostr ) :
		#print 'to write to sln: ', self.vs.compiler_tool( self, config, config.compiler_options )
		ostr.write( self.vs.compiler_tool( self, config, config.compiler_options ) )
								

	def handle_linker( self, config, ostr ) :
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

		ostr.write ( linker_tool % (   config.linker_options.additional_dependencies_as_string(),
								config.linker_options.output_file,
								config.linker_options.incremental_linking( config.name),
								config.linker_options.additional_library_directories_as_string( self.root_dir),
								config.linker_options.generate_debug_information_as_string(),
								config.linker_options.program_database_file,
								config.linker_options.subsystem(),
								config.linker_options.import_library,
								config.linker_options.target_machine ) )
								
	def handle_resource_compiler( self, config, ostr ) :
		res_tool = """ <Tool Name="VCResourceCompilerTool"/> """
		ostr.write( res_tool + "\n")

	def create_filter( self, filter_name, file_types ) :
		filter_element = """<Filter
				Name="%s"
				Filter="%s"
				UniqueIdentifier="{%s}"> """
		return filter_element % ( filter_name, file_types, uuid.uuid4() )


	def handle_file_configuration( self, the_file, filetype, config, ostr ) :
		ostr.write( self.vs.file_configuration( config, the_file, filetype ) )
		
	def get_files_with_filter( self, filterstr ) :
		suffixes = tuple( filterstr.split(";") )
		
		def suffix_filter( str ) :
			# str could be a SourceFile, with a name property which is the filename
			tmpstr = str
			if 'name' in dir( str ): tmpstr = str.name
			ppos = tmpstr.rfind(".")
			
			if ppos != -1 : tmpstr = tmpstr[ppos+1:]
			else : return False
			
			for ext in suffixes :
				if tmpstr == ext : return True
			
			return False
	
		to_filter = []
		
		if self.cpp_files is not None : to_filter += self.cpp_files
		if self.header_files is not None : 
			to_filter += self.header_files
		
		res_files = filter( suffix_filter , to_filter)
			
		def comparer( x, y ) :
			xtmp = x
			ytmp = y
			if isinstance(x, SourceFile) : xtmp = x.name
			if isinstance(y, SourceFile) : ytmp = y.name
			# Just sort on the actual file-name, not the full path.
			xtmp = os.path.split(xtmp)[1]
			ytmp = os.path.split(ytmp)[1]
			return cmp(xtmp, ytmp)
				
		res_files.sort( cmp = comparer )
		return res_files

	def handle_src_file( self, cpp_file, ostr ) :
		#print "self.relative_sln, self.relative_path, cpp_file ", self.relative_sln, self.relative_path, cpp_file
		thepath = os.path.join(self.relative_sln, self.relative_path, cpp_file.name )
		ostr.write( "<File RelativePath=\"%s\" >\n" % thepath.replace("/", "\\")  )
		for config in self.configurations :
			self.handle_file_configuration(cpp_file, "source", config, ostr )
		ostr.write( "</File>\n" )

	def handle_header_file( self, h_file_path, ostr ) :			
		thepath = os.path.join(self.relative_sln, self.relative_path, h_file_path.name )
		ostr.write( "<File RelativePath=\"%s\" >\n" % thepath.replace("/", "\\") )		
		for config in self.configurations :
			self.handle_file_configuration(h_file_path, "header", config, ostr )
		ostr.write( "</File>\n" )
		
	def handle_res_file( self, res_file_path, ostr ) :
		thepath = os.path.join(self.relative_sln, self.relative_path, res_file_path.name )
		ostr.write( "<File RelativePath=\"%s\" >\n" % thepath.replace("/", "\\") )		
		for config in self.configurations :
			self.handle_file_configuration(res_file_path, "resource", config, ostr )
		ostr.write( "</File>\n" )
		
	def cpp_extensions( self ) :
		return "cpp;c;cxx"
		
	def h_extensions( self ):
		return "h;hpp;hxx"
		
	def res_extensions( self ) :
		return "rc;ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe;resx;qrc"
		
	def handle_cpp_files( self, ostr ) :
		ostr.write( self.create_filter("Source Files", self.cpp_extensions() ) + "\n")
		files_to_use = self.get_files_with_filter( self.cpp_extensions() )
		for cpp_file in files_to_use:
			#print 'source file: ', cpp_file
			self.handle_src_file(cpp_file, ostr)
		ostr.write("</Filter>\n")
		
	def handle_rc_files( self, ostr ) :
		ostr.write( self.create_filter("Resource Files", self.res_extensions() ) + "\n")		
		files_to_use = self.get_files_with_filter( self.res_extensions() )
		for res_file in files_to_use:
			self.handle_res_file(res_file, ostr)
		ostr.write("</Filter>\n")

	def handle_h_files( self, ostr ) :
		ostr.write( self.create_filter("Header Files", self.h_extensions() ) + "\n")
		files_to_use = self.get_files_with_filter( self.h_extensions() )
		for h_file in files_to_use :
			self.handle_header_file(h_file, ostr)
		ostr.write( "</Filter>\n" )

class VSSolution :
	def __init__(self) :
		self.projects = []
		self.target_dir = ""
		
	def find_project_by_name( self, project_name ) :
		for proj in self.projects:
			if proj.name == project_name : return proj
		return None
	 
	def create_sln_and_vcproj_files( self, sln_file_name, deps, vs_version ) :
		#Start to save projects
		where_to_write = (self.root + self.target_dir).replace("/", "\\")
		try:
			os.makedirs(  where_to_write  )
		except os.error, e:
			pass
			
		# print "Saving sln-file", where_to_write
		for proj in self.projects:
			proj_file = self.root + self.target_dir +  proj.file_name
			proj_file = proj_file.replace("\\", "/")
			# print "Saving vcproj-file", proj_file
			proj.relative_sln = self.relative_root
			proj.guid = uuid.uuid4()
			f = open( proj_file, "w");
			f.write( str(proj) );
			f.close();
		# Now, build the solution-file
		vs = create_visual_studio(vs_version)
		
		sln = open(os.path.join( where_to_write, sln_file_name) , "w")
		if vs_version != "vs2003" : sln = codecs.EncodedFile( sln, "utf-8-sig")
		
		visual_studio_guid = "8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942"
			
		sln.write( vs.solution_file_header() + "\n")
			
		for proj in self.projects:
			project_element = """Project("{%s}") = "%s", "%s", "{%s}" """
			sln.write( project_element % (visual_studio_guid, proj.name, ".\\" + proj.file_name,  proj.guid ) )
			sln.write( "\n" )
			sln.write("\tProjectSection(ProjectDependencies) = postProject\n")
			if proj.name in deps :
				proj_deps = deps[ proj.name ]
				for d in proj_deps :
					dependent_project = self.find_project_by_name( d )
					# We might depend on pure object files, so these will not be found...
					if dependent_project is None : continue
					sln.write("\t\t{%s}={%s}\n" % (dependent_project.guid, dependent_project.guid ))
			
			sln.write("\tEndProjectSection\n")
			sln.write("EndProject\n")
			
		sln.write("Global\nEndGlobal\n")
		sln.close()
			
