import os
import re
import utils

def walk( self ):
	"""Walk the bcomp directory to pick out all the .wc files"""
	flags = { }
	for repo in [ 'pkgconfig/win32', 'bcomp/common', 'bcomp/' + self[ 'target' ] ]:
		for r, d, files in os.walk( os.path.join( self.root, repo ) ):
			for f in files:
				if f.endswith( '.wc' ):
					pkg = f.replace( '.wc', '' )
					prefix = os.path.join( r, '..', '..' )
					flags[ pkg ] = { 'prefix': prefix, 'file': os.path.join( r, f ) }
	self.package_list = flags

def remove_ws( tokens ):
	"""Remove whitespace and empty tokens from list"""
	result = [ ]
	for token in tokens:
		if token != '' and not token.isspace( ):
			result.append( token.strip( ) )
	return result

def read( filename ):
	"""Read the file, honour line continuations (ending in \) and return contents"""
	input = open( filename )
	result = input.read( )
	result = result.replace( '\\' + os.linesep, '' )
	input.close( )
	return result

def parse( file, overrides = [ ] ):
	"""Parse a .wc file with an optional set of variables"""

	# Read the data file
	data = read( file )

	# Variables are defined in the file as name=value
	variables = { }

	# Rules are defined in the file as name: value
	rules = { }

	# This regex is used to split each line to 'name', delimiter[, 'value' ]
	expression = re.compile( r'([^a-zA-Z0-9_\.])' )

	# Iterate throught the file and append to variables and rules as appropriate
	for line in data.splitlines( ):
		tokens = remove_ws( expression.split( line.strip( ), 1 ) )
		if len( tokens ) >= 2 and tokens[ 1 ] == '=':
			variables[ tokens[ 0 ] ] = str( tokens[ 2: ] )[ 2:-2 ]
		elif len( tokens ) >= 2 and tokens[ 1 ] == ':':
			rules[ tokens[ 0 ] ] = str( tokens[ 2: ] )[ 2:-2 ]
		elif len( tokens ) and tokens[ 0 ] == '#':
			pass
		elif len( tokens ) != 0:
			print( 'error in %s with line %s' % ( file, line ) )

	# Apply overrides to variables
	for name, value in overrides:
		variables[ name ] = value

	# Expand inter variable references (ie: execprefix=${prefix}/bin)
	for name in variables.keys( ):
		value = variables[ name ]
		for iter in variables.keys( ):
			value = value.replace( '${' + iter + '}', variables[ iter ] )
		variables[ name ] = value

	# Expand variables in the rules
	for name in rules.keys( ):
		value = rules[ name ]
		for iter in variables.keys( ):
			value = value.replace( '${' + iter + '}', variables[ iter ] )
		rules[ name ] = value

	# Merge variables and rules before the return
	rules.update( variables )

	return rules 

def obtain_rules( self, package, checked = [ ] ):
	"""Returns the compete set of rules for the package recursively calling this 
	function for any Requires usage."""

	if package in self.package_list.keys( ):
		# Make sure we only handle this package once
		checked.append( package )

		# Get the prefix of the package
		prefix = self.package_list[ package ][ 'prefix' ]

		# Set up the overrides associative array
		overrides = [ ( 'prefix', prefix ) ]

		# Parse the config file
		rules = parse( self.package_list[ package ][ 'file' ], overrides )

		# Ensure we have minimal CFlags and libs
		if 'CFlags' not in rules.keys( ): rules[ 'CFlags' ] = ''
		if 'Libs' not in rules.keys( ): rules[ 'Libs' ] = ''

		# Check the Requires rule
		if 'Requires' in rules.keys( ):
			# Split into tokens - note that package names may be interspersed with comparions to versions
			requires_list = rules[ 'Requires' ].split( )

			# Iterate until the list is empty
			while len( requires_list ):
				# Remove the package
				package = requires_list.pop( 0 )
				cmp = version = ''

				# Check follwing entries for comparitor and version
				if len( requires_list ) and requires_list[ 0 ] in [ '<', '<=', '=', '>=', '>' ]:
					cmp = requires_list.pop( 0 )
					version = requires_list.pop( 0 )

				# If we haven't checked before, check now and add to the main rules
				if package not in checked:
					deprules = obtain_rules( self, package, checked )
					if 'CFlags' in deprules.keys( ): rules[ 'CFlags' ] += ' ' + deprules[ 'CFlags' ]
					if 'Libs' in deprules.keys( ): rules[ 'Libs' ] += ' ' + deprules[ 'Libs' ]
					requires_list += rules[ 'Requires' ].split( )

		return rules
	else:
		print package

	return { }

def packages( self, *packages ):
	"""Add packages to the environment"""
	cflags = ''
	libflags = ''
	checked = []
	for package in packages:
		rules = obtain_rules( self, package, checked )
		if 'CFlags' in rules.keys( ):
			cflags += rules[ 'CFlags' ] + ' '
		if 'Libs' in rules.keys( ):
			libflags += rules[ 'Libs' ] + ' '
	if self[ 'PLATFORM' ] == 'win32':
		self.Append( CCFLAGS = cflags )
		self.Append( LIBFLAGS = libflags )
	else:
		self.MergeFlags( cflags + libflags )

def package_cflags( self, *packages ):
	"""Obtains the CFlags in the .wc file"""
	flags = ''
	checked = []
	for package in packages:
		rules = obtain_rules( self, package, checked )
		if 'CFlags' in rules.keys( ):
			flags += rules[ 'CFlags' ] + ' '
	return flags

def package_libs( self, *packages ):
	"""Obtains the Libs in the .wc file"""
	flags = ''
	checked = []
	for package in packages:
		rules = obtain_rules( self, package, checked )
		if 'CFlags' in rules.keys( ):
			flags += rules[ 'Libs' ] + ' '
	return flags

