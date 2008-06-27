import platform
import sys
import os

# Necessary hacks for linux - ensures characters are read correctly and that the global
# c++ namespace is used when loading plugins
if platform.system( ) == 'Linux':
	import locale
	locale.setlocale( locale.LC_CTYPE, '' )
	try:
		import dl
		sys.setdlopenflags( dl.RTLD_NOW | dl.RTLD_GLOBAL )
	except:
		sys.setdlopenflags( 257 )

# Get the full path to the running instance of this file (note that if you happen to be 
# in the site-packages dir, __file__ is reported as relative - hence the join here - however
# if the __file__ is absolute, the getcwd is ignored)
full_path = os.path.join( os.getcwd( ), __file__ )

# Determine the directory path to the plugins directory
if platform.system() == 'Windows' or platform.system() == 'Microsoft':
	dir_path = os.path.join( full_path.rsplit( os.sep, 3 )[ 0 ], 'bin', 'aml-plugins' )
else:
	dir_path = os.path.join( full_path.rsplit( os.sep, 3 )[ 0 ], 'ardome-ml', 'openmedialib', 'plugins' )

# Initialisate openpluginlib
import openpluginlib as pl
pl.init( dir_path )

def play( input, stores ):
	"""Courtesy function - just passes frames from input to stores in a non-interactive loop."""
	for store in stores:
		if store is None or not store.init( ):
			print "Unable to initialise stores"

	error = False

	try:
		count = input.get_frames( )
		while not error and count:
			frame = input.fetch( )
			input.seek( 1, True )
			for store in stores:
				if not store.push( frame ): 
					error = True
			count -= 1
	except KeyboardInterrupt:
		print "Aborted"

	if not error:
		for store in stores:
			store.complete( )

