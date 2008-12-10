#!/usr/bin/env python

"""
Some scons code to build an application bundle

Adapted from:

http://www.scons.org/wiki/MacOSX

"""
import sys

import SCons
from SCons.Script.SConscript import SConsEnvironment

def TOOL_WRITE_VAL(env):
	#if tools_verbose:
	env.Append(TOOLS = 'WRITE_VAL')
	def write_val(target, source, env):
		"""Write the contents of the first source into the target.
		source is usually a Value() node, but could be a file."""
		f = open(str(target[0]), 'wb')
		f.write(source[0].get_contents())
		f.close()
	env['BUILDERS']['WriteVal'] = SCons.Builder.Builder(action=write_val)
	
def TOOL_BUNDLE(env):
	"""defines env.MakeBundle() for installing an application or library bundle into
	   its dir. A bundle has this structure: (filenames are case SENSITIVE)
	   sapphire.app/ (or sapphire.bundle)
		   Contents/
			   Info.plist (an XML key->value database; defined by BUNDLE_INFO_PLIST)
			   PkgInfo (trivially short; defined by value of BUNDLE_PKGINFO)
			   MacOS/
				   executable (the executable or shared lib, linked with Bundle())
			   Resources/
				   sapphire.icns (the icons file, for an application
	
	
	To use it:
	
	# add the tool to your environment:
	MakeBundle.TOOL_BUNDLE(env)

	#Set up the Bundle Builder:
	env.MakeBundle(ProgName, # name of the program or bundle (".app" or ".bundle" will be added if it's not there already 
			   app, # the executable Node
			   info_plist="Info.plist", # the Info.plist file
			   icon_file="Images/Shio.icns", # the icon file (referred to in Info.plist)
			   signature='????', # the application signature (like the creator code)
			   resources=['Data'] # any other resource files you want installed.
			   )
	
	"""

	if 'BUNDLE' in env['TOOLS']: return
	if sys.platform == 'darwin': # not much point in doing this anywhere else
		env.Append(TOOLS = 'BUNDLE')
		# env['BUNDLEDIRSUFFIX'] = '.bundle'
		# This requires another tool (defined in this file):
		TOOL_WRITE_VAL(env)
		# Type codes are BNDL for generic bundle and APPL for application.
		def MakeBundle(env,
				bundledir,
				libs,
				info_plist,
				public_headers = [],	
				icon_file='',
				signature='????',
				#key,
				#subst_dict=None,
				resources=[]):
			"""Install a bundle into its dir, in the proper format"""

			# add (and check) the extension to the bundle:
			if not bundledir.split(".")[-1] == 'framework':
				bundledir += '.framework'

			# Set up the directory structure
			for item in libs:
				env.Install( bundledir+'/Versions/A/' + item[ 0 ], item[ 1 ] )
			for item in resources:
				env.Install( bundledir+'/Versions/A/Resources/' + item[ 0 ], item[ 1 ] )
			env.Install( bundledir+'/Versions/A/Headers/', source = public_headers )
			env.Install(bundledir+'/Versiosn/A/Resources/', info_plist)
			#env.WriteVal(target=bundledir+'/Contents/PkgInfo', source=SCons.Node.Python.Value(typecode+signature))

			# install the resources
			#resources.append(icon_file)
			#for r in resources:
			#	if SCons.Util.is_List(r):
			#		env.InstallAs(join(bundledir+'/Contents/Resources',
			#						   r[1]),
			#					  r[0])
			#	else:
			#		env.Install(bundledir+'/Contents/Resources', r)
			return [ SCons.Node.FS.default_fs.Dir(bundledir) ]

		# This is not a regular Builder; it's a wrapper function.
		# So just make it available as a method of Environment.
		SConsEnvironment.MakeBundle = MakeBundle
	
	

