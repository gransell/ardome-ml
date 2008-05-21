import os
import sys

def arch( ):
	"""Returns the architecture of the current system"""
	if os.name == 'posix': return os.uname( )[ 4 ]
	return 'i686'

def install_target( self ):
	"""Handle the install target logic - when install is specified, return the
	installation directory, otherwise, return the staging environment."""
	self.install = 'install' in sys.argv
	if self.install:
		return os.path.join( self[ 'distdir' ], self[ 'prefix' ] )
	return os.path.join( self.root, self[ 'build_prefix' ] )

def search_and_replace( filename, out, tokens ):
	"""Utility function which does a search and replace in a text file - tokens
	are provided in the form of a python associative array."""
	input = open( filename )
	output = open( out, 'w' )
	result = input.read( )
	for couple in tokens:
		result = result.replace( couple[ 0 ], couple[ 1 ] )
	output.write( result )
	output.close( )
	input.close( )
