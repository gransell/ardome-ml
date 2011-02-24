
projectTemplate = """

// !$*UTF8*$!
{
	archiveVersion = 1;
	classes = {
	};
	objectVersion = 45;
	objects = {
	
	/* Begin PBXBuildFile section */
		%for s in project.sources():
		%if s.format() == 'source':
		<%
			settings_string = ''
			if len(s.build_settings()):
				settings_string = 'settings = {'
				for setting_key in s.build_settings().keys():
					settings_string += setting_key + ' = \"' + s.build_settings()[setting_key] + '\";'
				settings_string += '};'
		%>
			${s.build_file_uuid()} = {isa = PBXBuildFile; fileRef = ${s.uuid()} /* ${s.name()} */; ${settings_string} };
		%endif
		%endfor
		%for t in project.targets():
		%for dep in t.dependencies():
			${dep.uuid()} /* ${dep.name()} */ = {isa = PBXBuildFile; fileRef = ${dep.target().product_uuid()} /* ${dep.target().file_name()} */; };
		%endfor
		%endfor
	/* End PBXBuildFile section */
	
	/* Begin PBXContainerItemProxy section */
		%for t in project.targets():
		%for proj_dep in t.project_dependencies():
			${proj_dep.proxy_uuid()} /* PBXContainerItemProxy */ = {
				isa = PBXContainerItemProxy;
				containerPortal = ${project.uuid()} /* Project object */;
				proxyType = 1;
				remoteGlobalIDString = ${proj_dep.target().uuid()};
				remoteInfo = ${proj_dep.target().name()};
			};
		%endfor
		%endfor
		%for agg_dep in project.aggregate_dependency_list():
			${agg_dep.proxy_uuid()} /* PBXContainerItemProxy */ = {
				isa = PBXContainerItemProxy;
				containerPortal = ${project.uuid()} /* Project object */;
				proxyType = 1;
				remoteGlobalIDString = ${agg_dep.target().uuid()};
				remoteInfo = ${agg_dep.target().name()};
			};
		%endfor
	/* End PBXContainerItemProxy section */
	
	/* Begin PBXFileReference section */
		/* Sources */
		%for s in project._sources :
			%if s.format() == 'source' :
			${s.uuid()} /* ${s.name()} */ = { isa = PBXFileReference; fileEncoding = 4; lastKnownFileType = ${s.type()}; name = ${s.name()}; path = ${s.path()}; sourceTree = SOURCE_ROOT; };
			%endif
		%endfor
		/* Headers */
		%for s in project._sources :
			%if s.format() == 'header' :
			${s.uuid()} /* ${s.name()} */ = { isa = PBXFileReference; fileEncoding = 4; lastKnownFileType = ${s.type()}; name = ${s.name()}; path = ${s.path()}; sourceTree = SOURCE_ROOT; };
			%endif
		%endfor
		/* Products */
		%for t in project.targets():
			${t.product_uuid()} /* ${t.file_name()} */ = {isa = PBXFileReference; explicitFileType = "${t.file_type()}"; includeInIndex = 0; path = ${t.file_name()}; sourceTree = BUILT_PRODUCTS_DIR; };
		%endfor
	/* End PBXFileReference section */


	/* Begin PBXGroup section */
		%for grp in project._groups :
			
			${grp.group_item_for_name('Headers').uuid()} = {
				isa = PBXGroup;
				children = (
				%for h in grp.group_item_for_name('Headers').sources() :
					${h.uuid()},
				%endfor
				);
				name = Headers;
				sourceTree = "<group>";
			};
			
			${grp.group_item_for_name('Sources').uuid()} = {
				isa = PBXGroup;
				children = (
				%for s in grp.group_item_for_name('Sources').sources() :
					${s.uuid()},
				%endfor
				);
				name = Sources;
				sourceTree = "<group>";
			};
			
			${grp.uuid()} = {
				isa = PBXGroup;
				children = (
					${grp.group_item_for_name('Headers').uuid()}, /* Headers */
					${grp.group_item_for_name('Sources').uuid()} /* Sources */
				);
				name = ${grp.name()};
				sourceTree = "<group>";
			};
		%endfor	
			
			034768DDFF38A45A11DB9C8B = {
				isa = PBXGroup;
				children = (
				%for t in project.targets():
					${t.product_uuid()} /* ${t.file_name()} */, 
				%endfor
				);
				name = Products;
				sourceTree = "<group>";
			};
			
			11609CA00F27C1C700579BAB = {
				isa = PBXGroup;
				children = (
					034768DDFF38A45A11DB9C8B,
				%for grp in project._groups:
					${grp.uuid()} /* ${grp.name()} */,
				%endfor
				);
				sourceTree = "<group>";
			};
	
	/* End PBXGroup section */
	
	/* Begin PBXHeadersBuildPhase section */
		%for t in project.targets():
			${t.headers_build_phase_uuid()} /* Headers */ = {
				isa = PBXHeadersBuildPhase;
				buildActionMask = 2147483647;
				files = (
				);
				runOnlyForDeploymentPostprocessing = 0;
			};
		%endfor
	/* End PBXHeadersBuildPhase section */

	/* Begin PBXSourcesBuildPhase section */
		%for t in project.targets():
			${t.sources_build_phase_uuid()} /* Sources */ = {
				isa = PBXSourcesBuildPhase;
				buildActionMask = 2147483647;
				files = (
				%for s in t.sources():
					${s.build_file_uuid()} /* ${s.name()} */,
				%endfor
				);
				runOnlyForDeploymentPostprocessing = 0;
			};
		%endfor
	/* End PBXSourcesBuildPhase section */

	/* Begin PBXFrameworksBuildPhase section */
		%for t in project.targets():
			${t.fmwk_build_phase_uuid()} /* Frameworks */ = {
				isa = PBXFrameworksBuildPhase;
				buildActionMask = 2147483647;
				files = (
				%for dep in t.dependencies():
					${dep.uuid()} /* ${dep.name()} */,
				%endfor
				);
				runOnlyForDeploymentPostprocessing = 0;
			};
		%endfor
	/* End PBXFrameworksBuildPhase section */
	
	/* Begin PBXNativeTarget section */
		%for t in project.targets():
			${t.uuid()} /* ${t.name()} */ = {
				isa = PBXNativeTarget;
				buildConfigurationList = ${t.build_configuration_uuid()} /* Build configuration list for PBXNativeTarget "${t.name()}" */;
				buildPhases = (
					${t.headers_build_phase_uuid()} /* Headers */,
					${t.sources_build_phase_uuid()} /* Sources */,
					${t.fmwk_build_phase_uuid()} /* Frameworks */,
				);
				buildRules = (
				);
				dependencies = (
				%for proj_dep in t.project_dependencies():
					${proj_dep.uuid()} ,
				%endfor
				);
				name = ${t.name()};
				productName = lib${t.name()};
				productReference = ${t.product_uuid()} /* ${t.file_name()} */;
				productType = "${t.target_type()}";
			};
		%endfor
	/* End PBXNativeTarget section */
	
	/* Begin PBXAggregateTarget section */
			11783807101EE833007279AC /* All */ = {
				isa = PBXAggregateTarget;
				buildConfigurationList = 11783814101EE84A007279AC /* Build configuration list for PBXAggregateTarget "All" */;
				buildPhases = (
				);
				dependencies = (
				%for agg_dep in project.aggregate_dependency_list():
					${agg_dep.uuid()} /* PBXContainerItemProxy */,
				%endfor
				);
				name = All;
				productName = All;
			};
	/* End PBXAggregateTarget section */

	/* Begin PBXProject section */
			${project.uuid()} /* Project object */ = {
				isa = PBXProject;
				buildConfigurationList = 11E6AD340FCFAB1F00E29B64 /* Default build configuration list for PBXProject */;
				compatibilityVersion = "Xcode 3.1";
				hasScannedForEncodings = 0;
				mainGroup = 11609CA00F27C1C700579BAB;
				projectDirPath = "";
				projectRoot = "";
				targets = (
				%for t in project.targets():
					${t.uuid()} /* ${t.name()} */,
				%endfor
				11783807101EE833007279AC /* All agregate target */,
				);
			};
	/* End PBXProject section */
	
	/* Begin PBXTargetDependency section */
		%for t in project.targets():
		%for proj_dep in t.project_dependencies():
			${proj_dep.uuid()} /* PBXTargetDependency */ = {
				isa = PBXTargetDependency;
				target = ${proj_dep.target().uuid()} /* ${proj_dep.target().name()} */;
				targetProxy = ${proj_dep.proxy_uuid()} /* PBXContainerItemProxy */;
			};
		%endfor
		%endfor
		%for agg_dep in project.aggregate_dependency_list():
			${agg_dep.uuid()} /* PBXTargetDependency */ = {
				isa = PBXTargetDependency;
				target = ${agg_dep.target().uuid()} /* ${agg_dep.target().name()} */;
				targetProxy = ${agg_dep.proxy_uuid()} /* PBXContainerItemProxy */;
			};
		%endfor
	/* End PBXTargetDependency section */

	/* Begin XCBuildConfiguration section */
		%for t in project.targets():
		%for bc in t.build_configurations():
			${bc.uuid()} /* ${bc.name()} */ = {
				isa = XCBuildConfiguration;
				buildSettings = {
				%for setting_key in bc.build_settings().keys():
					${setting_key} = ${bc.build_settings()[setting_key]};
				%endfor
					PRODUCT_NAME = ${t.name()};
				};
				name = ${bc.name()};
			};
		%endfor
		%endfor
		
			11783808101EE833007279AC /* Debug */ = {
				isa = XCBuildConfiguration;
				buildSettings = {
					COPY_PHASE_STRIP = NO;
					PRODUCT_NAME = All;
				};
				name = Debug;
			};
			11783809101EE833007279AC /* Release */ = {
				isa = XCBuildConfiguration;
				buildSettings = {
					COPY_PHASE_STRIP = YES;
					PRODUCT_NAME = All;
				};
				name = Release;
			};
		
			11E6AD320FCFAB1F00E29B64 /* Debug */ = {
				isa = XCBuildConfiguration;
				buildSettings = {
					COPY_PHASE_STRIP = NO;
				};
				name = Debug;
			};
			11E6AD330FCFAB1F00E29B64 /* Release */ = {
				isa = XCBuildConfiguration;
				buildSettings = {
					COPY_PHASE_STRIP = YES;
				};
				name = Release;
			};
	/* End XCBuildConfiguration section */


	/* Begin XCConfigurationList section */
		%for t in project.targets():
			${t.build_configuration_uuid()} /* Build configuration list for PBXNativeTarget "${t.name()}" */ = {
				isa = XCConfigurationList;
				buildConfigurations = (
				%for bc in t.build_configurations():
					${bc.uuid()} /* ${bc.name()} */,
				%endfor
				);
				defaultConfigurationIsVisible = 0;
				defaultConfigurationName = ${t.build_configurations()[0].name()};
			};
		%endfor
			
			11783814101EE84A007279AC /* Build configuration list for PBXAggregateTarget "All" */ = {
				isa = XCConfigurationList;
				buildConfigurations = (
					11783808101EE833007279AC /* Debug */,
					11783809101EE833007279AC /* Release */,
				);
				defaultConfigurationIsVisible = 0;
				defaultConfigurationName = Release;
			};
		
			11E6AD340FCFAB1F00E29B64 /* Default build configuration list for PBXProject */ = {
				isa = XCConfigurationList;
				buildConfigurations = (
					11E6AD320FCFAB1F00E29B64 /* Debug */,
					11E6AD330FCFAB1F00E29B64 /* Release */,
				);
				defaultConfigurationIsVisible = 0;
				defaultConfigurationName = Release;
			};
	/* End XCConfigurationList section */

	};
	rootObject = ${project.uuid()} /* Project object */;
}
"""

source_group_template = """
"""

import os
from mako.template import Template
from uuid import uuid4

def gen_xcode_uuid():
	return str(uuid4()).replace('-', '').upper()

class XcodeItem(object):
	"""docstring for XcodeItem"""
	def __init__(self, name):
		super(XcodeItem, self).__init__()
		self._uuid = gen_xcode_uuid( )
		self._name = name
	
	def uuid(self):
		return self._uuid
	
	def name(self):
		"""
		Returns the name of this item
		"""
		return self._name

file_type_to_source_type= { 'cpp' : 'sourcecode.cpp.cpp',
							'h' : 'sourcecode.c.h',
							'hpp' : 'sourcecode.c.h',
							'm' : 'sourcecode.c.objc',
							'mm' : 'sourcecode.cpp.objcpp', }

class XcodeSourceFile(XcodeItem):
	"""docstring for SourceFile"""
	def __init__(self, path, format):
		super(XcodeSourceFile, self).__init__( os.path.basename(str(path)) )
		self._path = path
		self._format = format
		self._type = ''
		ftype = path.split('.')[-1]
		if ftype in file_type_to_source_type.keys():
			self._type = ftype
		self._build_file_uuid = gen_xcode_uuid()
		self._build_settings = {}
	
	def path(self):
		return self._path
	
	def format(self):
		return self._format
	
	def type(self):
		return self._type
	
	def build_file_uuid(self):
		return self._build_file_uuid
	
	def build_settings(self):
		return self._build_settings

class XcodeGroup(XcodeItem):
	"""
	A xcode group item represents a container in the project. This container
	can contain other XcodeItems of any kind
	"""
	def __init__(self, name):
		super(XcodeGroup, self).__init__(name)
		self._children = []
		self._sources = []
	
	def children(self):
		return self._children
	
	def sources(self):
		return self._sources
	
	def group_item_for_name(self, name):
		"""docstring for group_item_for_name"""
		for ch in self._children :
			if isinstance(ch, XcodeGroup) and ch.name() == name:
				return ch
		return None

class XcodeTarget(XcodeItem):
	"""docstring for XcodeTarget"""
	def __init__(self, name):
		super(XcodeTarget, self).__init__(name)
		self._sources = []
		self._product_uuid = gen_xcode_uuid()
		self._build_file_uuid = gen_xcode_uuid()
		self._build_configurations = []
		self._build_configuration_uuid = gen_xcode_uuid()
		self._headers_build_phase_uuid = gen_xcode_uuid()
		self._sources_build_phase_uuid = gen_xcode_uuid()
		self._fmwk_build_phase_uuid = gen_xcode_uuid()
		self._build_settings = {}
		self._dependencies = []
		self._project_dependencies = []
	
	def sources(self):
		"""docstring for source_files"""
		return self._sources
	
	def source_item_for_path(self, path):
		"""docstring for source_item_for_path"""
		pass
	
	def file_type(self):
		raise Exception('Should subclass XcodeTarget')
	
	def target_type(self):
		raise Exception('Should subclass XcodeTarget')
	
	def file_name(self):
		raise Exception('Should subclass XcodeTarget')
	
	def product_uuid(self):
		return self._product_uuid
	
	def build_file_uuid(self):
		return self._build_file_uuid
	
	def build_configurations(self):
		return self._build_configurations
	
	def build_configuration_uuid(self):
		return self._build_configuration_uuid
	
	def headers_build_phase_uuid(self):
		return self._headers_build_phase_uuid
	
	def sources_build_phase_uuid(self):
		return self._sources_build_phase_uuid
	
	def fmwk_build_phase_uuid(self):
		return self._fmwk_build_phase_uuid
	
	def build_settings(self):
		return self._build_settings
	
	def dependencies(self):
		return self._dependencies
	
	def project_dependencies(self):
		return self._project_dependencies

class XcodeDylibTarget(XcodeTarget):
	"""docstring for XcodeDylibTarget"""
	def __init__(self, name):
		super(XcodeDylibTarget, self).__init__(name)
		self._build_settings['EXECUTABLE_PREFIX'] = 'lib'
	
	def file_type(self):
		return 'compiled.mach-o.dylib'
	
	def target_type(self):
		return 'com.apple.product-type.library.dynamic'
	
	def file_name(self):
		return 'lib' + self._name + '.dylib'

class XcodeExecutableTarget(XcodeTarget):
	"""docstring for XcodeDylibTarget"""
	def __init__(self, name):
		super(XcodeExecutableTarget, self).__init__(name)

	def file_type(self):
		return 'compiled.mach-o.executable'

	def target_type(self):
		return 'com.apple.product-type.tool'

	def file_name(self):
		return self._name
		
class XcodeBuildConfiguration(XcodeItem):
	"""docstring for XcodeBuildCofiguration"""
	def __init__(self, name, parent_target):
		super(XcodeBuildConfiguration, self).__init__(name)
		self._parent_target = parent_target
		self._build_settings = {}
		self._libs = set()
		self._framworks = set()
	
	def build_settings(self):
		for k in self._parent_target.build_settings():
			if k not in self._build_settings.keys():
				self._build_settings[k] = self._parent_target.build_settings()[k]
		return self._build_settings
	
	def libs(self):
		return self._libs
	
	def frameworks(self):
		return self._framworks

class XcodeTargetDependency(XcodeItem):
	"""docstring for XcodeTargetDependency"""
	def __init__(self, target):
		super(XcodeTargetDependency, self).__init__(target.file_name())
		self._target = target
	
	def target(self):
		return self._target
		

class XcodeDependencyProxy(XcodeItem):
	"""docstring for XcodeDependencyProxy"""
	def __init__(self, target):
		super(XcodeDependencyProxy, self).__init__('')
		self._target = target
		self._proxy_uuid = gen_xcode_uuid()
	
	def target(self):
		return self._target
	
	def proxy_uuid(self):
		return self._proxy_uuid
		

class XcodeProject(XcodeItem):
	"""docstring for XcodeProject"""
	def __init__(self):
		super(XcodeProject, self).__init__('')
		self._groups = []
		self._sources = []
		self._targets = []
		self._dependencies = []
		self._aggregate_dependency_list = []
	
	def source_item_for_path(self, path):
		"""docstring for source_item_for_path"""
		for s in self._sources :
			if s.path() == path :
				return s
		return None
	
	def sources(self):
		return self._sources
	
	def groups(self):
		return self._groups
	
	def targets(self):
		return self._targets
	
	def dependencies(self):
		return self._dependencies
	
	def aggregate_dependency_list(self):
		return self._aggregate_dependency_list

	def group_item_for_name(self, name):
		for ch in self._groups :
			if isinstance(ch, XcodeGroup) and ch.name() == name:
				return ch
		return None
	
	def target_with_name(self, name):
		for t in self._targets:
			if t.name() == name:
				return t
		return None
	
	def target_with_file_name(self, file_name):
		for t in self._targets:
			if file_name == t.file_name():
				return t
		return None
	
	def write( self, path, proj_name ):
		"""docstring for write"""
		project_bundle = os.path.join( path, proj_name + '.xcodeproj' )
		if not os.path.exists( project_bundle ) :
			os.makedirs( project_bundle )
		
		tmpl = Template(projectTemplate)
		project = tmpl.render( project = self )

		fd = open( os.path.join(project_bundle, 'project.pbxproj'), 'w' )
		fd.write( project )
