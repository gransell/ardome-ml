from aml import ml, pl, stack, thread_player, server

import string
import os
import math

class thread_stack( stack, pl.observer ):
	"""Combined stack and threaded player."""

	def __init__( self, player = None, printer = None, local = False ):
		"""Constructor - creates the threaded player and sets up the extended AML
		command set (to allow more comprehensive tranport controls and player
		aware functionality)."""

		if printer == None:
			printer = self.output

		stack.__init__( self, printer = printer )
		pl.observer.__init__( self )

		self.popper = 'dot'

		if player is None:
			self.player = thread_player( )
		else:
			self.player = player

		self.printer = printer
		self.server = None

		self.commands = { }
		
		VOCAB_SHELL = 'Shell Words'
		SYSTEM_SHELL = 'System Words'
		PLAYER_SHELL = 'Player Words'

		self.commands[ VOCAB_SHELL ] = { }

		self.commands[ VOCAB_SHELL ][ '.' ] = self.dot
		self.commands[ VOCAB_SHELL ][ 'clone' ] = self.clone
		self.commands[ VOCAB_SHELL ][ 'filters' ] = self.filters
		self.commands[ VOCAB_SHELL ][ 'props' ] = self.props
		self.commands[ VOCAB_SHELL ][ 'describe' ] = self.describe
		self.commands[ VOCAB_SHELL ][ 'include' ] = self.include_
		self.commands[ VOCAB_SHELL ][ 'help' ] = self.help

		if local:
			self.commands[ SYSTEM_SHELL ] = { }
			self.commands[ SYSTEM_SHELL ][ 'save' ] = self.save
			self.commands[ SYSTEM_SHELL ][ 'system' ] = self.system
			self.commands[ SYSTEM_SHELL ][ 'server' ] = self.start_server
			self.commands[ SYSTEM_SHELL ][ 'transport' ] = self.transport
			self.commands[ SYSTEM_SHELL ][ 'exit' ] = self.exit

		self.next_command = None
		self.incoming = ''

		self.aml = ml.create_filter( 'aml' )
		self.aml.property( 'filename' ).set( u'@' )
		self.aml_stdout = self.aml.property( 'stdout' )
		self.aml_write = self.aml.property( 'write' )

		self.handled = self.stack.property( "handled" )
		self.query = self.stack.property( "query" )
		self.query.attach( self )

		self.registration = self.player.register( self )
		self.commands[ PLAYER_SHELL ] = self.registration.commands

	def filters( self ):
		plugins = pl.discovery( pl.all_query_traits( "openmedialib", "", "", 0 ) )
		filters = [ ]
		for y in plugins:
			if y.type( ) == "filter":
				for f in y.extension( ):
					filters += [ str( f ) ]
		result = ''
		for f in sorted( filters ):
			if result == '':
				result = f
			else:
				result += ', ' + f
		self.output( result )

	def output( self, string ):
		if self.printer is None or self.printer == self.output:
			if string.endswith( '\n' ):
				print string,
			else:
				print string
		else:
			self.printer( string )

	def start_server( self ):
		if self.server is None:
			port = string.atoi( self.pop( ).get_uri( ) )
			try:
				self.server = server( self.player, port, port + 1 )
				self.server.start( )
				print "Server is started on port %d." % self.server.port
			except Exception, e:
				self.server = None
				raise Exception, "Server failed to start (%s)" % e
		else:
			raise Exception, "Server is already running on port %d." % self.server.port

	def transport( self ):
		if self.server is not None:
			os.spawnlp( os.P_NOWAIT, 'amltransport', 'amltransport', 'localhost', str( self.server.port ) )
		else:
			raise Exception, "Server is not started..."

	def describe( self ):
		self.push( 'dup' )
		item = self.pop( )
		self.aml.connect( item, 0 )
		text = self.aml_stdout.value_as_string( )
		self.output( text )
		self.aml_write.set( 1 )

	def dot( self ):
		"""Override the . to force play of the top of stack."""

		item = self.pop( )
		if item.get_frames( ) > 0:
			self.player.set( item )
		else:
			self.aml.connect( item, 0 )
			text = self.aml_stdout.value_as_string( )
			self.aml_write.set( 1 )
			if text != '':
				self.output( text )

	def clone_node( self, node ):
		"""Temporary - will migrate to C++ class."""

		if node is not None:
			i = 0
			while i < node.slots( ):
				self.clone_node( node.fetch_slot( i ) )
				i += 1
			self.push( node.get_uri( ) )
			for key in node.properties( ).get_keys( ):
				prop = node.properties( ).get_property( key )
				value = as_string( prop )
				if value is not None:
					self.push( '%s="%s"' % ( key, as_string( prop ) ) )

	def clone( self ):
		"""Clone the top of the stack - this is a necessary evil for general 
		reuse of the same media in the same project (a dup isn't sufficient
		for non-I-frame only media). Temporarily placed here, will move to
		C++ primitives."""

		self.push( 'dup' )
		tos = self.pop( )
		self.clone_node( tos )

	def props( self ):
		"""Display properties of the top of stack object. Temporary, will 
		migrate to C++ primitives."""

		self.push( 'dup' )
		input = self.pop( )
		if input is not None:
			properties = input.properties( )
			result = ''
			for key in properties.get_keys( ):
				result += str( key ) + ' '
			self.output( result )

	def help( self ):
		"""Nasty help output."""

		self.push( "dict" )

		for vocab in self.commands.keys( ):
			output = ''
			self.output( '' )
			self.output( vocab + ':' )
			self.output( '' )
			for cmd in sorted( self.commands[ vocab ] ):
				if output == '':
					output += cmd 
				else:
					output += ', ' + cmd 
			self.output( output )
	
	def exit( self ):
		"""Exit the thread."""

		self.player.exit( )

	def active( self ):
		"""Indicates if thread is still active or not."""

		return self.player.active( )

	def system( self ):
		"""Fork off a shell."""

		os.system( "bash" )

	def include_( self ):
		if self.next_command is None:
			self.next_command = self.include_
		else:
			self.next_command = None
			self.include( self.incoming )

	def save_print( self, message ):
		self.save_print_fd.write( message )

	def save( self ):
		if self.next_command is None:
			self.next_command = self.save
		else:
			self.next_command = None
			filename = self.incoming
			printer = self.printer
			try:
				self.printer = self.save_print
				self.save_print_fd = open( filename, 'w' )
				self.push( 'words' )
			finally:
				self.printer = printer

	def updated( self, subject ):
		"""Callback for query property - if the value of the query is found
		in the stack commands, then execute that and set the handled value to
		1 to indicate to the base implementation not to process this word."""

		command = self.query.value_as_wstring( )
		if self.next_command is not None:
			self.incoming = command
			self.next_command( )
			self.handled.set( 1 )
		else:
			for vocab in self.commands.keys( ):
				if str( command ) in self.commands[ vocab ].keys( ):
					cmd = self.commands[ vocab ][ command ]
					if isinstance( cmd, list ):
						for token in cmd:
							self.push( token )
					else:
						cmd( )
					self.handled.set( 1 )

def convert_list_to_string( list, quote = False ):
	result = ''
	for item in list:
		if quote:
			item = '"' + str( item ) + '"'
		if result != '':
			result += ' ' + str( item )
		else:
			result = str( item )
	return result

def as_string( prop ):
	if prop.is_a_double( ):
		return str( prop.value_as_double( ) )
	if prop.is_a_bool( ):
		return str( prop.value_as_bool( ) )
	if prop.is_a_image( ):
		return None
	if prop.is_a_input( ):
		return None
	if prop.is_a_int( ):
		return str( prop.value_as_int( ) )
	if prop.is_a_int64( ):
		return str( prop.value_as_int64( ) )
	if prop.is_a_int_list( ):
		return convert_list_to_string( prop.value_as_int_list( ) )
	if prop.is_a_string( ):
		return prop.value_as_string( )
	if prop.is_a_string_list( ):
		return convert_list_to_string( prop.value_as_string_list( ), True )
	if prop.is_a_uint64( ):
		return str( prop.value_as_uint64( ) )
	if prop.is_a_wstring( ):
		return str( prop.value_as_wstring( ) )
	if prop.is_a_wstring_list( ):
		return convert_list_to_string( prop.value_as_wstring_list( ), True )
	return None

