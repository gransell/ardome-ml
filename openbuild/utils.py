# (C) Ardendo SE 2008
#
# Released under the LGPL

import os
import sys

def arch( ):
	"""Returns the architecture of the current system"""
	if os.name == 'posix': return os.uname( )[ 4 ]
	return 'i686'

def install_target( self ):
	"""Handle the install target logic - when install is specified, return the
	installation directory, otherwise, return the staging environment."""
	if self.release_install or self.debug_install:
		return self[ 'distdir' ] + self[ 'prefix' ] 
	else:
		return os.path.join( self.root, self[ 'build_prefix' ] )

def search_and_replace( filename, out, tokens ):
	"""Utility function which does a search and replace in a text file and 
	generates a new file - tokens are provided in the form of a python 
	associative array."""
	input = open( filename )
	output = open( out, 'w' )
	result = input.read( )
	for couple in tokens:
		result = result.replace( couple[ 0 ], couple[ 1 ] )
	output.write( result )
	output.close( )
	input.close( )

def vs() :
	for a in sys.argv:
		if "vs=" in a :
			return a.split("=")[1]
	return None

def verbose() :
	for a in sys.argv:
		if 'verbose=' in a and a.split("=")[1] == 'yes':
			return True
	return False

def xcode() :
	for a in sys.argv:
		if 'generate_xcode=' in a and a.split("=")[1] == 'yes':
			return True
	return False

def clean_path( str ):
	tokens = str.split( os.sep )
	result = ''
	if len( tokens ) > 1:
		for token in tokens:
			if token != '':
				result += os.sep + token
		if result[ 0 ] != str[ 0 ]:
			result = result[ 1: ]
		if str != result:
			print str, " = ", result
	else:
		result = str
	return result

def path_to_openbuild( ) :
	return os.path.split(__file__)[0]
		
def path_to_openbuild_tools( ) :
	return os.path.join( path_to_openbuild(), "Tools")

def ensure_output_path_exists( opath ) :
	try:
		if os.path.exists( os.path.dirname(opath) ) : return
		os.makedirs(  os.path.dirname(opath)  )
	except os.error, e:
		pass

def needs_rebuild( src, dst ):
	"""Checks if dst exists or src is more recently modified than dst"""
	result = True
	if os.path.exists( src ):
		if os.path.exists( dst ):
			result = os.path.getmtime( src ) > os.path.getmtime( dst )
		if result:
			print "rebuilding %s from %s" % ( dst, src )
	else:
		raise Exception, "Error: source file %s does not exist." % src
	return result

