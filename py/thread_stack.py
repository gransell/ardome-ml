from aml import ml, pl, stack, thread_player, server

import string
import os

class thread_stack( stack, pl.observer ):
	"""Combined stack and threaded player."""

	def __init__( self, player = None, printer = None ):
		"""Constructor - creates the threaded player and sets up the extended AML
		command set (to allow more comprehensive tranport controls and player
		aware functionality)."""

		stack.__init__( self )
		pl.observer.__init__( self )

		if player is None:
			self.thread = thread_player( )
		else:
			self.thread = player

		self.printer = printer
		self.server = None

		self.commands = { }
		self.commands[ '.' ] = self.dot
		self.commands[ 'add' ] = self.add
		self.commands[ 'clone' ] = self.clone
		self.commands[ 'exit' ] = self.exit
		self.commands[ 'grab' ] = self.grab
		self.commands[ 'help' ] = self.help
		self.commands[ 'insert' ] = self.insert
		self.commands[ 'log_level' ] = self.log_level
		self.commands[ 'next' ] = self.next
		self.commands[ 'playing' ] = self.playing
		self.commands[ 'playlist' ] = self.playlist
		self.commands[ 'position?' ] = self.position
		self.commands[ 'prev' ] = self.prev
		self.commands[ 'previous' ] = self.previous
		self.commands[ 'props' ] = self.props
		self.commands[ 'render' ] = self.render
		self.commands[ 'seek' ] = self.seek
		self.commands[ 'speed' ] = self.speed
		self.commands[ 'speed?' ] = self.speed_query
		self.commands[ 'server' ] = self.start_server
		self.commands[ 'system' ] = self.system

		self.commands[ 'pitch_filter' ] = self.pitch
		self.commands[ 'volume_filter' ] = self.volume
		self.commands[ 'deinterlace_filter' ] = self.deinterlace

		prop = self.stack.property( "query" )
		prop.attach( self )

	def output( self, string ):
		if self.printer is None:
			print string
		else:
			self.printer( string )

	def start_server( self ):
		if self.server is None:
			self.push( 'dot' )
			port = string.atoi( self.stack.fetch_slot( 0 ).get_uri( ) )
			self.server = server( self.thread, port )
			self.server.start( )
		else:
			raise Exception, "Server is already started..."

	def start( self ):
		"""Start the shell as a thread. On OSX, GUI oriented stores (such as 
		SDL) cannot be run as a background thread."""

		self.thread.start( )

	def render( self ):
		self.push( 'dup' )
		self.push( 'filter:aml', 'filename=@' )
		self.push( 'dot' )
		item = self.stack.fetch_slot( 0 )
		text = item.property( "text" ).value_as_string( )
		self.output( text )

	def dot( self ):
		"""Override the . to force play of the top of stack."""

		self.push( 'length?' )
		self.push( 'dot' )
		length = int( self.stack.fetch_slot( 0 ).get_uri( ) )
		if length > 0:
			self.push( 'dot' )
			self.thread.set( self.stack.fetch_slot( 0 ) )
		else:
			self.push( 'filter:aml', 'filename=@' )
			self.push( 'dot' )
			item = self.stack.fetch_slot( 0 )
			text = item.property( "text" ).value_as_string( )
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

		self.push( 'dot' )
		tos = self.stack.fetch_slot( 0 )
		self.stack.connect( tos, 0 )
		self.push( 'recover' )
		self.clone_node( tos )

	def log_level( self ):

		self.push( 'dot' )
		pl.set_log_level( string.atoi( self.stack.fetch_slot( 0 ).get_uri( ) ) )

	def pitch( self ):
		"""Place the pitch filter on the stack (temporary - these 'helper'
		filters don't need specific words to obtain them)."""

		self.stack.connect( self.thread.get_instance( 'pitch' ), 0 )
		self.push( 'recover' )

	def volume( self ):
		"""Place the volume filter on the stack (temporary - these 'helper'
		filters don't need specific words to obtain them)."""

		self.stack.connect( self.thread.get_instance( 'volume' ), 0 )
		self.push( 'recover' )

	def deinterlace( self ):
		"""Place the deinterlace filter on the stack (temporary - these 'helper'
		filters don't need specific words to obtain them)."""

		self.stack.connect( self.thread.get_instance( 'deinterlace' ), 0 )
		self.push( 'recover' )

	def speed( self ):
		"""Set the speed to the number at the top of the stack."""

		self.push( 'dot' )
		self.thread.set_speed( string.atoi( self.stack.fetch_slot( 0 ).get_uri( ) ) )

	def speed_query( self ):
		""" Put the current playout speed on top of the stack."""

		self.push( self.thread.get_speed( ) )

	def add( self ):
		"""Add top of stack to playlist."""

		self.push( 'dot' )
		self.thread.add( self.stack.fetch_slot( 0 ) )

	def next( self ):
		"""Move to next in playlist."""

		self.thread.next( )

	def prev( self ):
		"""Move to prev in playlist."""

		self.thread.prev( )

	def seek( self ):
		"""Seek to the position at the top of the stack."""

		self.push( 'dot' )
		self.thread.seek( string.atoi( self.stack.fetch_slot( 0 ).get_uri( ) ) )

	def position( self ):
		"""Place current position on the top of the stack."""

		self.push( str( self.thread.position( ) ) )

	def props( self ):
		"""Display properties of the top of stack object. Temporary, will migrate
		to C++ primitives."""

		self.push( 'dup', 'dot' )
		input = self.stack.fetch_slot( 0 )
		if input is not None:
			properties = input.properties( )
			result = ''
			for key in properties.get_keys( ):
				result += str( key ) + ' '
			self.output( result )

	def help( self ):
		"""Nasty help output."""

		self.push( "dict" )

		self.output( '' )
		self.output( 'Shell Extensions:' )
		self.output( '' )

		output = ''
		for cmd in sorted( self.commands ):
			if output == '':
				output += cmd 
			else:
				output += ', ' + cmd 

		self.output( output )

	def playlist( self ):
		"""Dump the playlist to stdout."""

		self.thread.cond.acquire( )
		for input in self.thread.input:
			render = ml.create_filter( 'aml' )
			render.property( 'filename' ).set( unicode( '-' ) )
			render.connect( input, 0 )
			self.output( '' )
		self.thread.cond.release( )

	def previous( self ):
		"""Dump the previously played list to stdout."""

		self.thread.cond.acquire( )
		for input in self.thread.prev_inputs:
			render = ml.create_filter( 'aml' )
			render.property( 'filename' ).set( unicode( '-' ) )
			render.connect( input, 0 )
			self.output( '' )
		self.thread.cond.release( )

	def playing( self ):
		"""Dump the current playing item to stdout."""

		self.thread.cond.acquire( )
		if self.thread.current is not None:
			render = ml.create_filter( 'aml' )
			render.property( 'filename' ).set( unicode( '-' ) )
			render.connect( self.thread.current, 0 )
		self.thread.cond.release( )

	def exit( self ):
		"""Exit the thread."""

		self.thread.exit( )

	def active( self ):
		"""Indicates if thread is still active or not."""

		return self.thread.active( )

	def system( self ):
		"""Fork off a shell."""

		os.system( "bash" )

	def grab( self ):
		"""Grab the input at the specified playlist index (<index> grab -- <input>)."""

		self.push( 'dot' )
		input = self.thread.grab( int( self.stack.fetch_slot( 0 ).get_uri( ) ) )
		if input is not None:
			self.push( input )

	def insert( self ):
		"""Insert the input at the specified playlist index (<input> <index> 
		insert --)."""

		self.push( 'dot' )
		index = int( self.stack.fetch_slot( 0 ).get_uri( ) )
		self.push( 'dot' )
		self.thread.insert( index, self.stack.fetch_slot( 0 ) )

	def updated( self, subject ):
		"""Callback for query property - if the value of the query is found
		in the stack commands, then execute that and set the handled value to
		1 to indicate to the base implementation not to process this word."""

		command = self.stack.property( "query" ).value_as_wstring( )
		if str( command ) in self.commands.keys( ):
			cmd = self.commands[ command ]
			if isinstance( cmd, list ):
				for token in cmd:
					self.push( token )
			else:
				cmd( )
			self.stack.property( "handled" ).set( 1 )

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


