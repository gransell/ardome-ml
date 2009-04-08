from aml import ml, pl, full_path

import os
import time

class io( pl.observer ):
	def __init__( self, stack ):
		pl.observer.__init__( self )
		self.stack = stack
		self.collecting = False
		self.collection = [ ]

	def updated( self, subject ):
		text = self.stack.aml_stdout.value_as_string( )
		if not self.collecting:
			self.stack.output( text )
		else:
			self.collection.append( text )

	def collect( self, value ):
		self.collecting = value
		if value:
			self.collection = []
		return self.collection

class stack:
	"""Wrapper for the aml_stack c++ implementation."""

	def __init__( self, command = None, printer = None ):
		"""Constructor creates the stack instance and sets up io redirection."""

		self.stack = ml.create_input( 'aml_stack:' )
		self.popper = '.'

		self.command = self.stack.property( 'command' )
		self.stdout = self.stack.property( 'stdout' )
		self.result = self.stack.property( 'result' )

		self.aml = ml.create_filter( 'aml' )
		self.aml.property( 'filename' ).set( u'@' )
		self.aml_stdout = self.aml.property( 'stdout' )
		self.aml_write = self.aml.property( 'write' )

		self.io = io( self )
		self.aml_stdout.set_always_notify( True )
		self.aml_stdout.attach( self.io )

		self.printer = printer
		if printer is not None:
			self.stack.property( 'redirect' ).set( 1 )
		if command is not None:
			self.push( command )

		self.tokens = [ ]

	def output( self, string ):
		if string.endswith( '\n' ):
			print string,
		else:
			print string

	def serialise( self, item ):
		self.io.collect( True )
		self.aml.connect( item, 0 )
		self.aml_write.set( 1 )
		return self.io.collect( False )

	def deserialise( self, input ):
		for line in input:
			for inner in line.split( '\n' ):
				self.define( inner )
		return self.pop( )

	def define( self, command ):
		"""Tokenise and push a long string."""

		result = [ ]
		for token in command.split( ' ' ):
			if token.startswith( '#' ): break
			if len( result ) != 0 and ( result[ -1 ].count( '\"' ) % 2 != 0 or result[ -1 ].endswith( '\\' ) ):
				result[ -1 ] += ' ' + token
			elif token != '':
				result.append( token )

		tokens = [ ]
		for token in result:
			if token[ 0 ] == '\'' and token[ -1 ] == '\'':
				token = '\"' + token[ 1:-1 ] + '\"'
			else:
				token = token.replace( '\\ ', ' ' )
				#token = token.replace( '\"', '\'' )
			tokens.append( token )

		while len( tokens ):
			token = tokens.pop( 0 )
			self.push( token )

	def parse( self, command ):
		"""Tokenise and push a long string."""

		result = [ ]
		for token in command.split( ' ' ):
			if token.startswith( '#' ): break
			if len( result ) != 0 and ( result[ -1 ].count( '\"' ) % 2 != 0 or result[ -1 ].endswith( '\\' ) ):
				result[ -1 ] += ' ' + token
			elif token != '':
				result.append( token )

		self.tokens = [ ]
		for token in result:
			if token[ 0 ] == '\'' and token[ -1 ] == '\'':
				token = '\"' + token[ 1:-1 ] + '\"'
			else:
				token = token.replace( '\\ ', ' ' )
				#token = token.replace( '\"', '\'' )
			self.tokens.append( token )

		while len( self.tokens ):
			token = self.tokens.pop( 0 )
			self.push( token )

	def include( self, command ):
		"""Include an aml file from the aml site-packages directory."""

		if os.path.exists( command ):
			self.push( command )
		else:
			dir = full_path.rsplit( os.sep, 1 )[ 0 ]
			full = os.path.join( dir, command )
			if os.path.exists( full ):
				self.push( full )
			else:
				raise Exception( 'Unable to locate %s' % command )

	def push( self, *command ):
		"""Pushes commands on to the stack."""

		for cmd in command:
			if isinstance( cmd, str ):
				self.command.set( unicode( cmd ) )
			elif isinstance( cmd, unicode ):
				self.command.set( cmd )
			elif isinstance( cmd, list ):
				for entry in cmd:
					self.push( entry )
			elif isinstance( cmd, ml.input ):
				self.stack.connect( cmd, 0 )
				self.command.set( unicode( "recover" ) )
			else:
				self.command.set( unicode( cmd ) )

			if self.printer is not None:
				text = self.stdout.value_as_string( )
				if text != '':
					self.printer( self.stdout.value_as_string( ) )
				self.stdout.set_from_string( '' )

			result = self.result.value_as_wstring( )
			if result != "OK":
				raise Exception( result + " (" + str( cmd ) + ")" )

	def pop( self ):
		"""Returns the input object at the top of the stack"""

		self.push( self.popper )
		return self.stack.fetch_slot( 0 )

