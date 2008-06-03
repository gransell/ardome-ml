import glob
import os
import re
import utils

class WinConfig :

	#def __init__(self) :
		
	def walk( self, env ):
		"""Walk the bcomp directory to pick out all the .wc files"""
		flags = { }
		shared = os.path.join( __file__.rsplit( '/', 1 )[ 0 ], 'pkgconfig' )
		for repo in [ os.path.join( 'pkgconfig', 'win32' ), os.path.join( 'bcomp', 'common' ), os.path.join( 'bcomp', env[ 'target' ] ) ]  :
			for r, d, files in os.walk( os.path.join( env.root, repo ) ):
				for f in files:
					if f.endswith( '.wc' ):
						pkg = f.replace( '.wc', '' )
						prefix = r.rsplit( '\\', 2 )[ 0 ]
						flags[ pkg ] = { 'prefix': prefix, 'file': os.path.join( r, f ) }
		return flags

	def remove_ws( self, tokens ):
		"""Remove whitespace and empty tokens from list"""
		result = [ ]
		for token in tokens:
			if token != '' and not token.isspace( ):
				result.append( token.strip( ) )
		return result

	def read( self, filename ):
		"""Read the file, honour line continuations (ending in \) and return contents"""
		input = open( filename )
		result = input.read( )
		result = result.replace( ' \\' + os.linesep, '' )
		input.close( )
		return result

	def parse( self, file, overrides = [ ] ):
		"""Parse a .wc file with an optional set of variables"""

		# Read the data file
		data = self.read( file )

		# Variables are defined in the file as name=value
		variables = { }

		# Rules are defined in the file as name: value
		rules = { }

		# This regex is used to split each line to 'name', delimiter[, 'value' ]
		expression = re.compile( r'([^a-zA-Z0-9_\.])' )

		# Iterate throught the file and append to variables and rules as appropriate
		for line in data.splitlines( ):
			tokens = self.remove_ws( expression.split( line.strip( ), 1 ) )
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

	def obtain_rules( self, env, package, checked = [ ] ):
		"""Returns the compete set of rules for the package recursively calling this 
		function for any Requires usage."""

		if package in env.package_list.keys( ):
			# Make sure we only handle this package once
			checked.append( package )

			# Get the prefix of the package
			prefix = env.package_list[ package ][ 'prefix' ]

			# Set up the overrides associative array
			overrides = [ ( 'prefix', prefix ) , ('debug', '${debug_flag}')] 

			# Parse the config file
			rules = self.parse( env.package_list[ package ][ 'file' ], overrides )

			# Ensure we have minimal CFlags and libs
			if 'CFlags' not in rules.keys( ): rules[ 'CFlags' ] = ''
			if 'CppDefines' not in rules.keys( ): rules[ 'CppDefines' ] = ''
			if 'Libs' not in rules.keys( ): rules[ 'Libs' ] = ''
			if 'LibPath' not in rules.keys( ): rules[ 'LibPath' ] = ''
			if 'CppPath' not in rules.keys( ): rules[ 'CppPath' ] = ''

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
						deprules = self.obtain_rules( env, package, checked )
						if 'CFlags' in deprules.keys( ): 
							rules[ 'CFlags' ] = deprules[ 'CFlags' ] + ' ' + rules[ 'CFlags' ]
						if 'CppDefines' in deprules.keys( ): 
							rules[ 'CppDefines' ] = deprules[ 'CppDefines' ] + ' ' + rules[ 'CppDefines' ]
						if 'Libs' in deprules.keys( ): 
							rules[ 'Libs' ] = deprules[ 'Libs' ] + ' ' + rules[ 'Libs' ]
						if 'LibPath' in deprules.keys( ): 
							rules[ 'LibPath' ] = deprules[ 'LibPath' ] + ' ' + rules[ 'LibPath' ]
						if 'CppPath' in deprules.keys( ): 
							rules[ 'CppPath' ] = deprules[ 'CppPath' ] + ' ' + rules[ 'CppPath' ]
						requires_list += rules[ 'Requires' ].split( )

			return rules
		else:
			raise Exception, 'Unable to locate %s' % package

		return { }

	def packages( self, env, *packages ):
		"""Add packages to the environment"""
		cflags = ''
		cppdefines = ''
		libflags = ''
		libpath = ''
		cpp_path = ''
		checked = []
		for package in packages:
			rules = self.obtain_rules( env, package, checked )
			if 'CFlags' in rules.keys( ):
				cflags += ' ' + rules[ 'CFlags' ]
			if 'CppDefines' in rules.keys( ):
				cppdefines += ' ' + rules[ 'CppDefines' ]
			if 'Libs' in rules.keys( ):
				libflags += ' ' + rules[ 'Libs' ]
			if 'LibPath' in rules.keys( ):
				libpath += ' ' + rules[ 'LibPath' ]
			if 'CppPath' in rules.keys( ):
				cpp_path += ' ' + rules[ 'CppPath' ]
		if env[ 'PLATFORM' ] == 'win32':
			env.Append( CCFLAGS = cflags )
			env.Append( CPPDEFINES = cppdefines.split( ' ' ) )
			env.Append( CPPPATH = cpp_path.split( ' ' ) )
			env.Append( LIBPATH = libpath.split( ' ' ) )
			env.Append( LIBS = libflags.split( ' ' ) )
		else:
			env.MergeFlags( cflags + libflags )

	def optional( self, env, *packages ):
		"""Extracts compile and link flags for the specified packages and adds to the 
		current environment along with a have_package variable to allow dependency checks."""
		result = {}
		for package in packages:
			try:
				env.packages( package )
				result[ 'have_' + package ] = 1
			except Exception, e:
				result[ 'have_' + package ] = 0
		return result

	def package_cflags( self, env, *packages ):
		"""Obtains the CFlags in the .wc file"""
		flags = ''
		checked = []
		for package in packages:
			rules = self.obtain_rules( env, package, checked )
			if 'CFlags' in rules.keys( ):
				flags += rules[ 'CFlags' ] + ' '
		return flags

	def package_libs( self, env, *packages ):
		"""Obtains the Libs in the .wc file"""
		flags = ''
		checked = []
		for package in packages:
			rules = self.obtain_rules( env, package, checked )
			if 'CFlags' in rules.keys( ):
				flags += rules[ 'Libs' ] + ' '
		return flags

	def package_install_include( self, env ):
		result = []
		checked = []
		for package in env.package_list.keys( ):
			if env.package_list[ package ][ 'prefix' ].startswith( os.path.join( env.root, 'bcomp' ) ):
				rules = self.obtain_rules( env, package, checked )
				if 'install_include' in rules.keys( ):
					include = rules[ 'install_include' ]
					result += glob.glob( include )
		return result

	def package_install_libs( self, env ):
		result = []
		checked = []
		for package in env.package_list.keys( ):
			if env.package_list[ package ][ 'prefix' ].startswith( os.path.join( env.root, 'bcomp' ) ):
				rules = self.obtain_rules( env, package, checked )
				if 'install_libs' in rules.keys( ):
					libs = rules[ 'install_libs' ]
					result += glob.glob( libs )
		return result

