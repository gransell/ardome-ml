from aml import ml, pl, stack, thread_player, server

import string
import os
import math
import re
import time

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
		AUTHOR_SHELL = 'Authoring Words'

		self.commands[ VOCAB_SHELL ] = { }
		self.commands[ VOCAB_SHELL ][ '.' ] = self.dot
		self.commands[ VOCAB_SHELL ][ 's.' ] = self.seek_dot
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
			self.commands[ SYSTEM_SHELL ][ 'utransport' ] = self.utransport
			self.commands[ SYSTEM_SHELL ][ 'exit' ] = self.exit

		self.commands[ AUTHOR_SHELL ] = { }
		self.commands[ AUTHOR_SHELL ][ 'contains' ] = self.contains
		self.commands[ AUTHOR_SHELL ][ 'contains.' ] = self.contains_dot
		self.commands[ AUTHOR_SHELL ][ 'find' ] = self.find
		self.commands[ AUTHOR_SHELL ][ 'find.' ] = self.find_dot
		self.commands[ AUTHOR_SHELL ][ 'find_all' ] = self.find_all
		self.commands[ AUTHOR_SHELL ][ 'find_all.' ] = self.find_all_dot
		self.commands[ AUTHOR_SHELL ][ 'parent' ] = self.parent
		self.commands[ AUTHOR_SHELL ][ 'tag' ] = self.tag
		self.commands[ AUTHOR_SHELL ][ 'retag' ] = self.retag

		self.handled = self.stack.property( "handled" )
		self.token = self.stack.property( "token" )
		self.query = self.stack.property( "query" )
		self.query.attach( self )

		self.registration = self.player.register( self )
		self.commands[ PLAYER_SHELL ] = self.registration.commands

		self.next_op = []

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

	def stop_server( self ):
		if self.server is not None:
			self.server.running = False

	def transport( self ):
		if self.server is not None:
			if os.fork( ) == 0:
				os.execlp( 'amltransport', 'amltransport', 'localhost:%d' % self.server.port )
		else:
			raise Exception, "Server is not started..."

	def utransport( self ):
		if self.server is not None:
			if os.fork( ) == 0:
				os.execlp( 'amlutransport', 'amlutransport', 'localhost:%d' % self.server.port )
		else:
			raise Exception, "Server is not started..."

	def describe( self ):
		self.push( 'dup' )
		item = self.pop( )
		self.aml.connect( item, 0 )
		self.aml_write.set( 1 )

	def dot( self ):
		"""Override the . to force play of the top of stack."""

		item = self.pop( )
		if item.get_frames( ) > 0:
			self.player.set_speed( 0 )
			self.player.insert( 0, item )
			self.player.next( )
			self.player.set_speed( 1 )
		else:
			self.aml.connect( item, 0 )
			self.aml_write.set( 1 )

	def seek_dot( self ):
		self.player.seek_pos = string.atoi( self.pop( ).get_uri( ) )

	def clone_node( self, node ):
		"""Temporary - will migrate to C++ class."""

		self.io.collect( True )
		self.aml.connect( node, 0 )
		self.aml_write.set( 1 )
		for line in self.io.collect( False ):
			for item in line.split( '\n' ):
				if item != '':
					self.define( item )

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

	def next_operation( self, method ):
		if len( self.next_op ) == 0 or self.next_op[ -1 ] != method:
			self.next_op.append( method )
			return False
		else:
			self.next_op.pop( )
			return True

	def include_( self ):
		if self.next_operation( self.include_ ):
			self.include( self.pop( ).get_uri( ) )

	def save_print( self, message ):
		self.save_print_fd.write( message )

	def save( self ):
		if self.next_operation( self.save ):
			filename = self.pop( ).get_uri( )
			printer = self.printer
			try:
				self.printer = self.save_print
				self.save_print_fd = open( filename, 'w' )
				self.push( 'words' )
			finally:
				self.printer = printer

	def has_property( self, input, name ):
		matches = [ ]
		if input is not None:
			if input.property( name ).valid( ):
				matches += [ input ]
			i = 0
			while i < input.slots( ):
				matches += self.has_property( input.fetch_slot( i ), name )
				i += 1
		return matches

	def contains_dot( self ):
		name = str( self.pop( ).get_uri( ) )
		self.push( 'dup' )
		input = self.pop( )
		result = self.has_property( input, name )
		for match in result:
			self.push( match )
		self.push( len( result ) )

	def contains( self ):
		if self.next_operation( self.contains ):
			self.contains_dot( )

	def match_pattern( self, input, name, value ):
		matches = [ ]
		if input is not None:
			if input.property( name ).valid( ) and re.match( value + '$', input.property( name ).value_as_wstring( ) ) is not None:
				matches += [ input ]
			i = 0
			while i < input.slots( ):
				matches += self.match_pattern( input.fetch_slot( i ), name, value )
				i += 1
		return matches
	
	def find_all_dot( self ):
		label = str( self.pop( ).get_uri( ) )
		self.push( 'dup' )
		input = self.pop( )
		result = self.match_pattern( input, "@tag", label )
		for match in result:
			self.push( match )
		self.push( len( result ) )

	def find_all( self ):
		if self.next_operation( self.find_all ):
			self.find_all_dot( )

	def locate( self, input, name, value ):
		matches = [ ]
		if input is not None:
			if input.property( name ).valid( ) and input.property( name ).value_as_wstring( ) == value:
				matches += [ input ]
			i = 0
			while i < input.slots( ):
				matches += self.locate( input.fetch_slot( i ), name, value )
				i += 1
		return matches

	def find_dot( self ):
		label = str( self.pop( ).get_uri( ) )
		self.push( 'dup' )
		input = self.pop( )
		result = self.locate( input, "@tag", label )
		if len( result ):
			self.push( result[ 0 ] )
			self.push( 1 )
		else:
			self.push( 0 )
		
	def find( self ):
		if self.next_operation( self.find ):
			self.find_dot( )

	def locate_parent( self, haystack, needle ):
		parent = None
		slot = -1
		if haystack is not None:
			i = 0
			while i < haystack.slots( ):
				if needle.property( "@tag" ).value_as_wstring( ) == haystack.fetch_slot( i ).property( "@tag" ).value_as_wstring( ):
					parent = haystack
					slot = i
					break
				i += 1

			if slot == -1:
				i = 0
				while i < haystack.slots( ):
					parent, slot = self.locate_parent( haystack.fetch_slot( i ), needle )
					if slot != -1:
						break
					i += 1

		return parent, slot

	def parent( self ):
		self.push( 'over' )
		self.push( 'over' )
		needle = self.pop( )
		haystack = self.pop( )
		if not ml.equals( needle, haystack ):
			parent, slot = self.locate_parent( haystack, needle )
			if slot >= 0 and parent is not None:
				self.push( parent )
				self.push( slot )
			else:
				self.push( -1 )
		else:
			threader = self.player.get_instance( "threader" )
			if ml.equals( threader.fetch_slot( 0 ), haystack ):
				self.push( threader )
				self.push( 'describe' )
				self.push( 0 )
			else:
				self.push( -1 )

	def tags( self, input, tags, reuse = True ):
		untagged = []
		if input is not None:
			i = 0
			while i < input.slots( ):
				untagged += self.tags( input.fetch_slot( i ), tags, reuse )
				i += 1
			if reuse and input.property( "@tag" ).valid( ):
				tag = input.property( "@tag" ).value_as_wstring( )
				if tag in tags:
					untagged += [ input ]
				else:
					tags[ tag ] = input
			else:
				untagged += [ input ]
		return untagged

	def tag( self, reuse = True ):
		self.push( 'dup' )
		input = self.pop( )
		tags = {}
		untagged = self.tags( input, tags, reuse )
		for node in untagged:
			key = 0
			label = node.get_uri( )
			if node.slots( ) == 0:
				if label.endswith( ':' ):
					label = label[ 0:-1 ]
				else:
					label = 'input'
			full = "%s%d" % ( label, key )
			while full in tags.keys( ):
				key += 1
				full = "%s%d" % ( label, key )
			tags[ full ] = node
			self.push( node )
			self.push( '@tag="%s"' % full )
			self.push( 'drop' )

	def retag( self ):
		self.tag( False )

	def updated( self, subject ):
		"""Callback for query property - if the value of the query is found
		in the stack commands, then execute that and set the handled value to
		1 to indicate to the base implementation not to process this word."""

		self.handled.set( 0 )
		command = self.query.value_as_wstring( )
		if len( self.next_op ):
			method = self.next_op[ -1 ]
			method( )
			if len( self.next_op ):
				self.handled.set( 2 )
			else:
				self.handled.set( 1 )
		elif command == 'parse-token':
			if len( self.tokens ):
				self.token.set( unicode( self.tokens.pop( 0 ) ) )
			else:
				self.token.set( unicode( '""' ) )
		else:
			for vocab in self.commands.keys( ):
				if str( command ) in self.commands[ vocab ].keys( ):
					cmd = self.commands[ vocab ][ command ]
					cmd( )
					if len( self.next_op ):
						self.handled.set( 2 )
					else:
						self.handled.set( 1 )
					break

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


