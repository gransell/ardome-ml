from aml import ml, pl, stack, thread_player, server

import string
import os
import math

class thread_stack( stack, pl.observer ):
	"""Combined stack and threaded player."""

	def __init__( self, player = None, printer = None ):
		"""Constructor - creates the threaded player and sets up the extended AML
		command set (to allow more comprehensive tranport controls and player
		aware functionality)."""

		stack.__init__( self, printer = printer )
		pl.observer.__init__( self )

		if player is None:
			self.player = thread_player( )
		else:
			self.player = player

		self.printer = printer
		self.server = None

		self.commands = { }
		self.commands[ '.' ] = self.dot
		self.commands[ '**' ] = self.pow
		self.commands[ 'sin' ] = self.sin
		self.commands[ 'cos' ] = self.cos
		self.commands[ 'tan' ] = self.tan
		self.commands[ 'add' ] = self.add
		self.commands[ 'clone' ] = self.clone
		self.commands[ 'exit' ] = self.exit
		self.commands[ 'filters' ] = self.filters
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
		self.commands[ 'status?' ] = self.status
		self.commands[ 'system' ] = self.system
		self.commands[ 'transport' ] = self.transport
		self.commands[ 'store' ] = self.store

		self.commands[ 'pitch_filter' ] = self.pitch
		self.commands[ 'volume_filter' ] = self.volume
		self.commands[ 'deinterlace_filter' ] = self.deinterlace

		self.next_command = None
		self.incoming = ''

		self.aml = ml.create_filter( 'aml' )
		self.aml.property( 'filename' ).set( u'@' )
		self.aml_stdout = self.aml.property( 'stdout' )
		self.aml_write = self.aml.property( 'write' )

		self.handled = self.stack.property( "handled" )
		self.query = self.stack.property( "query" )
		self.query.attach( self )

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
		if self.printer is None:
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

	def start( self ):
		"""Start the shell as a thread. On OSX, GUI oriented stores (such as 
		SDL) cannot be run as a background thread."""

		self.player.start( )

	def render( self ):
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

	def pop( self ):
		self.push( 'dot' )
		return self.stack.fetch_slot( 0 )

	def pow( self ):
		y = float( self.pop( ).get_uri( ) )
		x = float( self.pop( ).get_uri( ) )
		self.push( math.pow( x, y ) )

	def sin( self ):
		x = float( self.pop( ).get_uri( ) )
		self.push( math.sin( x ) )

	def cos( self ):
		x = float( self.pop( ).get_uri( ) )
		self.push( math.cos( x ) )

	def tan( self ):
		x = float( self.pop( ).get_uri( ) )
		self.push( math.tan( x ) )

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

		tos = self.pop( )
		self.stack.connect( tos, 0 )
		self.push( 'recover' )
		self.clone_node( tos )

	def log_level( self ):

		pl.set_log_level( string.atoi( self.pop( ).get_uri( ) ) )

	def pitch( self ):
		"""Place the pitch filter on the stack (temporary - these 'helper'
		filters don't need specific words to obtain them)."""

		self.stack.connect( self.player.get_instance( 'pitch' ), 0 )
		self.push( 'recover' )

	def volume( self ):
		"""Place the volume filter on the stack (temporary - these 'helper'
		filters don't need specific words to obtain them)."""

		self.stack.connect( self.player.get_instance( 'volume' ), 0 )
		self.push( 'recover' )

	def deinterlace( self ):
		"""Place the deinterlace filter on the stack (temporary - these 'helper'
		filters don't need specific words to obtain them)."""

		self.stack.connect( self.player.get_instance( 'deinterlace' ), 0 )
		self.push( 'recover' )

	def speed( self ):
		"""Set the speed to the number at the top of the stack."""

		self.player.set_speed( string.atoi( self.pop( ).get_uri( ) ) )

	def speed_query( self ):
		""" Put the current playout speed on top of the stack."""

		self.push( self.player.get_speed( ) )

	def add( self ):
		"""Add top of stack to playlist."""

		self.player.add( self.pop( ) )

	def next( self ):
		"""Move to next in playlist."""

		self.player.next( )

	def prev( self ):
		"""Move to prev in playlist."""

		self.player.prev( )

	def seek( self ):
		"""Seek to the position at the top of the stack."""

		self.player.seek( string.atoi( self.pop( ).get_uri( ) ) )

	def position( self ):
		"""Place current position on the top of the stack."""

		self.push( str( self.player.position( ) ) )

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

		self.player.cond.acquire( )
		try:
			for input in self.player.input:
				render = ml.create_filter( 'aml' )
				render.property( 'filename' ).set( unicode( '@' ) )
				render.connect( input, 0 )
				self.output( render.property( 'stdout' ).value_as_string( ) )
				self.output( '' )
		finally:
			self.player.cond.release( )

	def previous( self ):
		"""Dump the previously played list to stdout."""

		self.player.cond.acquire( )
		try:
			for input in self.player.prev_inputs:
				render = ml.create_filter( 'aml' )
				render.property( 'filename' ).set( unicode( '@' ) )
				render.connect( input, 0 )
				self.output( render.property( 'stdout' ).value_as_string( ) )
				self.output( '' )
		finally:
			self.player.cond.release( )

	def playing( self ):
		"""Dump the current playing item to stdout."""

		self.player.cond.acquire( )
		try:
			if self.player.current is not None:
				render = ml.create_filter( 'aml' )
				render.property( 'filename' ).set( unicode( '@' ) )
				render.connect( self.player.current, 0 )
				self.output( render.property( 'stdout' ).value_as_string( ) )
				self.output( '' )
		finally:
			self.player.cond.release( )

	def status( self ):
		self.player.cond.acquire( )
		try:
			if self.player.current is not None:
				self.push( self.player.current.get_frames( ) )
				self.push( self.player.position( ) )
				self.push( self.player.get_speed( ) )
			else:
				self.push( 0 )
				self.push( 0 )
				self.push( 0 )
			self.dot( )
			self.dot( )
			self.dot( )
		finally:
			self.player.cond.release( )
		
	def exit( self ):
		"""Exit the thread."""

		self.player.exit( )

	def active( self ):
		"""Indicates if thread is still active or not."""

		return self.player.active( )

	def system( self ):
		"""Fork off a shell."""

		os.system( "bash" )

	def grab( self ):
		"""Grab the input at the specified playlist index (<index> grab -- 
		<input>)."""

		input = self.player.grab( int( self.pop( ).get_uri( ) ) )
		if input is not None:
			self.push( input )

	def insert( self ):
		"""Insert the input at the specified playlist index (<input> <index> 
		insert --)."""

		index = int( self.pop( ).get_uri( ) )
		self.player.insert( index, self.pop( ) )

	def store( self ):
		if self.next_command is None:
			self.next_command = self.store
			self.store_props = [ ]
		elif self.incoming != '.':
			self.store_props += [ str( self.incoming ) ]
		else:
			self.player.cond.acquire( )
			try:
				self.next_command = None
				if isinstance( self.player.stores, list ) and len( self.player.stores ) and isinstance( self.player.stores[0], str ):
					self.player.store += self.store_props
				else:
					result = self.player.stores
					for pair in self.store_props:
						name, value = pair.split( '=', 1 )
						prop = result[ 0 ].property( name )
						if prop.valid( ):
							prop.set_from_string( value )
						elif name.startswith( '@' ):
							key = pl.create_key_from_string( name )
							prop = pl.property( key )
							prop.set( unicode( value ) )
							result[ 0 ].properties( ).append( prop )
						else:
							print 'Requested property %s not found found - ignoring.' % name
			finally:
				self.player.cond.release( )

	def updated( self, subject ):
		"""Callback for query property - if the value of the query is found
		in the stack commands, then execute that and set the handled value to
		1 to indicate to the base implementation not to process this word."""

		command = self.query.value_as_wstring( )
		if self.next_command is not None:
			self.incoming = command
			self.next_command( )
			self.handled.set( 1 )
		elif str( command ) in self.commands.keys( ):
			cmd = self.commands[ command ]
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


