#!/usr/bin/env python

"""
Some scons code to build an application bundle

Adapted from:

http://www.scons.org/wiki/MacOSX

"""
import sys
import os

import SCons
from SCons.Script.SConscript import SConsEnvironment

def collect_sym_links( links, item ):
	if item[ 0 ] == '':
		stem = str( item[ 1 ] ).split( '.' )[ 0 ]
		dir, base = stem.rsplit( '/', 1 )
		if base.startswith( 'lib' ):
			for r, dirs, files in os.walk( dir ):
				for f in files:
					full = os.path.join( r, f )
					if f.startswith( base ) and os.path.islink( full ):
						links[ full ] = f

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

		TOOL_WRITE_VAL(env)
		# Type codes are BNDL for generic bundle and APPL for application.
		def MakeBundle(env,
				bndl_name,
				lib,
				ext_libs, 
				public_headers = [],	
				info_plist = None,
				icon_file='',
				signature='????',
				#key,
				#subst_dict=None,
				resources=[]):
			"""Install a bundle into its dir, in the proper format"""

			# add (and check) the extension to the bundle:
			if not bndl_name.split(".")[-1] == 'framework':
				bndl_name += '.framework'
			
			abs_bundle_path = env.subst( os.path.join(env[ 'stage_frameworks' ], bndl_name) )

			env.Install(abs_bundle_path + '/Versions/A/', lib)
			
			for item in ext_libs:
				env.Install( abs_bundle_path + '/Versions/A/lib/' + item[ 0 ], item[ 1 ] )
			
			if not os.path.exists(abs_bundle_path):
				os.makedirs( abs_bundle_path )
			else :
				os.utime( abs_bundle_path, None )
			if not os.path.exists( os.path.join(abs_bundle_path, 'Versions') ):
				os.makedirs( os.path.join(abs_bundle_path, 'Versions') )
			if not os.path.exists( os.path.join(abs_bundle_path, 'Versions/A/lib') ):
				os.makedirs( os.path.join(abs_bundle_path, 'Versions/A/lib') )
			
			# Set up links
			#Hack to get the paths correct
			if not os.path.islink(os.path.join(abs_bundle_path, 'Versions/lib')):
				os.symlink( 'A/lib', os.path.join(abs_bundle_path, 'Versions/lib') )
			if not os.path.islink(os.path.join(abs_bundle_path, 'Versions', 'Current')):
				os.symlink( 'A', os.path.join(abs_bundle_path, 'Versions', 'Current') )			
			if not os.path.islink(os.path.join(abs_bundle_path, str(lib))):
				os.symlink( os.path.join('Versions/Current/', str(lib)), os.path.join(abs_bundle_path, str(lib)) )
			if not os.path.islink(os.path.join(abs_bundle_path, 'Headers')):
				os.symlink( 'Versions/Current/Headers', os.path.join(abs_bundle_path, 'Headers') )
			if not os.path.islink(os.path.join(abs_bundle_path, 'Resources')):
				os.symlink( 'Versions/Current/Resources', os.path.join(abs_bundle_path, 'Resources') )
			
			links = {}
			for item in ext_libs:
				try:
					collect_sym_links( links, item )
				except Exception, e:
					continue
			
			for link in links.keys( ):
				full = abs_bundle_path+'/Versions/A/lib/' + links[ link ]
				if not os.path.islink( full ):
					os.symlink( os.readlink( link ), full )

			for item in resources:
				env.Install( abs_bundle_path + '/Versions/A/Resources/' + item[ 0 ], item[ 1 ] )
			if len(public_headers) != 0 :
				env.Install( abs_bundle_path + '/Versions/A/Headers', public_headers )
			if info_plist is not None :
				env.Install(abs_bundle_path + '/Versions/A/Resources/', info_plist)
			
			return [ SCons.Node.FS.default_fs.Dir(abs_bundle_path) ]

		# This is not a regular Builder; it's a wrapper function.
		# So just make it available as a method of Environment.
		SConsEnvironment.MakeBundle = MakeBundle
	
	

