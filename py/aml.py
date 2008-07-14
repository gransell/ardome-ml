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
elif platform.system( ) == 'Darwin':
	from AppKit import NSApplicationLoad
	NSApplicationLoad( )

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
import openimagelib as il
import openmedialib as ml
pl.init( dir_path )

class stack:
	"""Wrapper for the aml_stack c++ implementation."""

	def __init__( self, command = None ):
		"""Constructor creates the object"""
		self.stack = ml.create_input( "aml_stack:" )
		if command is not None:
			self.push( command )

	def define( self, command ):
		"""Tokenise and push a long string."""
		result = [ ]
		for token in command.split( ' ' ):
			if len( result ) != 0 and result[ -1 ].count( '\"' ) % 2 != 0:
				result[ -1 ] += ' ' + token
			else:
				result.append( token )
		for token in result:
			self.push( token )

	def push( self, *command ):
		"""Pushes commands on to the stack."""
		for cmd in command:
			if isinstance( cmd, str ):
				self.stack.property( "command" ).set( pl.to_wstring( cmd ) )
			elif isinstance( cmd, unicode ):
				self.stack.property( "command" ).set( cmd )
			elif isinstance( cmd, list ):
				for entry in cmd:
					self.push( entry )
			result = self.stack.property( "result" ).value_as_wstring( )
			if result != "OK":
				raise Exception( result )

	def pop( self ):
		"""Returns the input object at the top of the stack"""
		self.push( "." )
		return self.stack.fetch_slot( 0 )

def play( input, stores = None ):
	"""Courtesy function - just passes frames from input to stores in a non-interactive loop."""

	if stores is None or len( stores ) == 0:
		stores = [ ml.create_store( "preview:", input.fetch( ) ) ]
	elif not isinstance( stores, list ):
		stores = [ stores ]

	for store in stores:
		if store is None or not store.init( ):
			raise Exception( "Unable to initialise stores" )

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

