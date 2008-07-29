from aml import ml, full_path

import os
import time

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
			if token.startswith( '#' ): break
			if len( result ) != 0 and ( result[ -1 ].count( '\"' ) % 2 != 0 or result[ -1 ].endswith( '\\' ) ):
				result[ -1 ] += ' ' + token
			elif token != '':
				result.append( token )
		for token in result:
			if token != '':
				if token[ 0 ] == '\"' and token[ -1 ] == '\"':
					token = token[ 1:-1 ]
				elif token[ 0 ] == '\'' and token[ -1 ] == '\'':
					token = token[ 1:-1 ]
				else:
					token = token.replace( '\\', '' )
					token = token.replace( '\"', '\'' )
				self.push( token )
				time.sleep( 0.0001 )

	def include( self, command ):
		"""Include an aml file from the aml site-packages directory."""

		dir = full_path.rsplit( os.sep, 1 )[ 0 ]
		self.push( os.path.join( dir, command ) )

	def push( self, *command ):
		"""Pushes commands on to the stack."""

		for cmd in command:
			if isinstance( cmd, str ):
				if cmd.startswith( 'http://' ): cmd = 'aml:' + cmd
				self.stack.property( "command" ).set( unicode( cmd ) )
			elif isinstance( cmd, unicode ):
				if cmd.startswith( 'http://' ): cmd = 'aml:' + cmd
				self.stack.property( "command" ).set( cmd )
			elif isinstance( cmd, list ):
				for entry in cmd:
					self.push( entry )
			elif isinstance( cmd, ml.input ):
				self.stack.connect( cmd, 0 )
				self.stack.property( "command" ).set( unicode( "recover" ) )
			else:
				self.stack.property( "command" ).set( unicode( cmd ) )
				
			result = self.stack.property( "result" ).value_as_wstring( )
			if result != "OK":
				raise Exception( result + " (" + str( cmd ) + ")" )

	def pop( self ):
		"""Returns the input object at the top of the stack"""

		self.push( "." )
		return self.stack.fetch_slot( 0 )

