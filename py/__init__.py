import platform
import sys
import os
import time
import exceptions
import string

# Necessary hacks for linux - ensures characters are read correctly and that the global
# c++ namespace is used when loading plugins
if platform.system( ) == 'Linux':
	import locale
	locale.setlocale( locale.LC_CTYPE, '' )
	try:
		import dl
		sys.setdlopenflags( dl.RTLD_NOW | dl.RTLD_GLOBAL )
	except:
		sys.setdlopenflags( 257 )
elif platform.system( ) == 'Darwin':
	from AppKit import NSApplicationLoad
	NSApplicationLoad( )

# Get the full path to the running instance of this file (note that if you happen to be 
# in the site-packages dir, __file__ is reported as relative - hence the join here - however
# if the __file__ is absolute, the getcwd is ignored)
full_path = os.path.join( os.getcwd( ), __file__ )

# Determine the directory path to the plugins directory
if platform.system() == 'Windows' or platform.system() == 'Microsoft':
	dir_path = os.path.join( full_path.rsplit( os.sep, 4 )[ 0 ], 'bin', 'aml-plugins' )
elif platform.system( ) == 'Darwin':
	dir_path = os.path.join( full_path.rsplit( os.sep, 4 )[ 0 ] )
else:
	dir_path = os.path.join( full_path.rsplit( os.sep, 4 )[ 0 ], 'ardome-ml', 'openmedialib', 'plugins' )

# Initialisate openpluginlib
import openpluginlib as pl
import openimagelib as il
import openmedialib as ml

pl.init( dir_path )
pl.set_log_level( -1 )

import threading

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
			elif isinstance( cmd, openmedialib.input ):
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

class player:
	def __init__( self ):
		"""Constructor."""

		self.normalise = None

	def ante( self, input, stores, frame ):
		"""Method called before push to stores. Returns false if no error/
		termination detected."""

		return False

	def post( self, input, stores, frame, speed = 1, eof = True ):
		"""Method called after push to stores. Returns false if no error/
		termination detected."""

		if input.get_position( ) + speed < input.get_frames( ):
			input.seek( speed, True )
			return False
		return eof

	def check_input( self, input ):
		"""Internal method: converts input to aml input object and associates
		minimalist filters to ensure we have audio/image and active threaders."""

		if isinstance( input, str ) or isinstance( input, unicode ):
			input = ml.create_input( input )
		elif isinstance( input, list ):
			input = stack( input ).pop( )

		if self.normalise is None:
			frame = input.fetch( )

			if frame.get_image( ) is None:
				filter = ml.create_filter( 'visualise' )
				filter.connect( input, 0 )
				input = filter

			if frame.get_audio( ) is None:
				filter = ml.create_filter( 'conform' )
				filter.connect( input, 0 )
				input = filter

			if input.is_thread_safe( ):
				filter = ml.create_filter( 'threader' )
				filter.property( 'active' ).set( 1 )
				filter.connect( input, 0 )
				input = filter
		else:
			input = self.normalise( input )
		
		return input

	def walk_and_assign( self, input, name, value ):
		"""Walk the input, check for the existence of the property name and 
		assign it the value on each matching node."""

		if input.property( name ).valid( ):
			input.property( name ).set( value )
		i = input.slots( ) - 1
		while i >= 0:
			self.walk_and_assign( input.fetch_slot( i ), name, value )
			i -= 1

	def check_stores( self, input, stores ):
		"""Internal method: initialises stores with aml input object."""

		if stores is None or len( stores ) == 0:
			stores = [ ml.create_store( "preview:", input.fetch( ) ) ]
		elif isinstance( stores, list ) and isinstance( stores[ 0 ], str ):
			result = [ ml.create_store( stores[ 0 ], input.fetch( ) ) ]
			for prop in stores[ 1: ]:
				name, value = prop.split( '=', 1 )
				result[ 0 ].property( name ).set_from_string( value )
			stores = result
		elif not isinstance( stores, list ):
			stores = [ stores ]

		for store in stores:
			if store is None or not store.init( ):
				raise Exception( "Unable to initialise stores" )

		deferrable = True
		for store in stores:
			deferrable = store.property( "deferrable" ).valid( ) and store.property( "deferrable" ).value_as_int( ) == 1
			if not deferrable: break

		self.walk_and_assign( input, "deferred", int( deferrable ) )

		return stores

	def play( self, input, stores = None ):
		"""Passes frames from input to stores in a loop."""

		input = self.check_input( input )
		stores = self.check_stores( input, stores )

		error = False
	
		try:
			while not error and input.get_position( ) < input.get_frames( ):
				frame = input.fetch( )
				error = self.ante( input, stores, frame )
				if not error:
					for store in stores:
						if not store.push( frame ): 
							error = True
				yield frame
				error = self.post( input, stores, frame ) or error
	
		except KeyboardInterrupt:
			print "Aborted"

		if not error:
			for store in stores:
				store.complete( )

		yield None

class stats_player( player ):
	"""Subclass of player which reports position on stdout."""

	def __init__( self ):
		"""Constructor."""

		player.__init__( self )

	def ante( self, input, stores, frame ):
		"""Report position/duration."""

		print "%d/%d\r" %( input.get_position( ), input.get_frames( ) ), 
		sys.stdout.flush( )
		return player.ante( self, input, stores, frame )

def play( input, stores = None, renderer = stats_player ):
	"""Convenience method for simple, non-threaded, position reporting playout."""

	render = renderer( )
	iter = render.play( input, stores )
	while iter.next( ) is not None:
		pass
	print

class thread_player( player, threading.Thread ):
	"""Advanced playlist oriented threaded player."""

	def __init__( self ):
		"""Constructor."""

		player.__init__( self )
		threading.Thread.__init__( self )
		self.lock = threading.RLock( )
		self.cond = threading.Condition( self.lock )
		self.next_input = False
		self.current = None
		self.prev_inputs = [ ]
		self.prev_count = 10
		self.prev_input = False
		self.input = [ ]
		self.store = None
		self.speed = 1
		self.position_ = -1
		self.current_position = -1
		self.exiting = False

	def set_store( self, store ):
		"""Allows the specification of a custome store."""

		self.store = store

	def set( self, input ):
		"""Request immediate playout of input."""

		self.cond.acquire( )
		self.input = [ input ]
		self.speed = 1
		self.next_input = True
		self.cond.notifyAll( )
		self.cond.release( )

	def add( self, input ):
		"""Append to playout list."""

		self.cond.acquire( )
		self.input += [ input ]
		self.cond.notifyAll( )
		self.cond.release( )

	def next( self ):
		"""Play next in list."""

		self.cond.acquire( )
		if len( self.input ):
			self.next_input = True
			self.cond.notifyAll( )
		self.cond.release( )

	def prev( self ):
		"""Play previous item."""

		self.cond.acquire( )
		if len( self.prev_inputs ):
			self.prev_input = True
			self.cond.notifyAll( )
		self.cond.release( )

	def set_speed( self, speed ):
		"""Set the speed of the playout."""

		self.cond.acquire( )
		self.speed = speed
		self.cond.notifyAll( )
		self.cond.release( )

	def get_speed( self ):
		"""Return the speed of playout."""

		return self.speed

	def seek( self, position ):
		"""Seek to the requested frame position."""

		self.cond.acquire( )
		if self.current is not None:
			self.position_ = position
			self.cond.notifyAll( )
			self.cond.wait( )
		self.cond.release( )

	def position( self ):
		"""Return the current position in the playing input."""

		self.cond.acquire( )
		result = self.current_position
		self.cond.release( )
		return result

	def exit( self ):
		"""Exit the thread."""

		self.cond.acquire( )
		self.exiting = True
		self.cond.notifyAll( )
		self.cond.release( )

	def active( self ):
		"""Indicates if still active."""

		self.cond.acquire( )
		is_active = not self.exiting
		self.cond.release( )
		return is_active

	def post( self, input, stores, frame, eof = False ):
		"""Method called after push to stores. Returns false if no error/
		termination detected."""

		self.cond.acquire( )
		if self.position_ >= 0:
			input.seek( self.position_, False )
			self.position_ = -1
		elif input.get_position( ) + self.speed < input.get_frames( ):
			input.seek( self.speed, True )
		elif len( self.input ) > 0:
			eof = True
		self.current_position = input.get_position( )
		self.cond.release( )
		return eof

	def run( self ):
		"""Thread execution loop."""

		while self.active( ):
			self.cond.acquire( )

			while self.active( ) and len( self.input ) == 0 and not self.prev_input and not self.next_input:
				self.cond.wait( )

			if not self.active( ): 
				self.cond.release( )
				break

			# Honour prev/next requests
			if self.prev_input and self.current is not None:
				self.input = [ self.current ] + self.input
				if len( self.prev_inputs ) != 0:
					self.input = [ self.prev_inputs[ -1 ] ] + self.input
					self.prev_inputs.pop( -1 )
			elif self.next_input and self.current is not None:
				self.prev_inputs += [ self.current ]
			elif self.current is not None:
				self.prev_inputs += [ self.current ]

			# Previous input maintenance
			while len( self.prev_inputs ) > self.prev_count:
				self.prev_inputs.pop( 0 )

			# Schdule the first input for playout
			self.current = self.input[ 0 ]
			self.current.seek( 0, False )
			self.store = self.check_stores( self.current, self.store )
			iter = self.play( self.current, self.store )
			self.input.pop( 0 )
			self.next_input = False
			self.prev_input = False
			self.cond.release( )

			# Loop while playing current input
			running = True
			while running and self.active( ):
				self.cond.acquire( )
				running = not self.next_input and not self.prev_input and iter.next( ) is not None
				self.cond.notifyAll( )
				if self.speed == 0:
					self.cond.wait( 0.1 )
				self.cond.release( )
				time.sleep( 0.0001 )

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

class thread_stack( stack, pl.observer ):
	"""Combined stack and threaded player."""

	def __init__( self ):
		"""Constructor - creates the threaded player and sets up the extended AML
		command set (to allow more comprehensive tranport controls and player
		aware functionality)."""

		stack.__init__( self )
		pl.observer.__init__( self )

		self.thread = thread_player( )
		self.thread.normalise = self.normalise

		self.colon_count = 0
		self.definition = [ ]

		self.pitch_ = None
		self.volume_ = None
		self.deinterlace_ = None

		self.commands = { }
		self.commands[ '.' ] = self.dot
		self.commands[ 'add' ] = self.add
		self.commands[ 'clone' ] = self.clone
		self.commands[ 'exit' ] = self.exit
		self.commands[ 'help' ] = self.help
		self.commands[ 'next' ] = self.next
		self.commands[ 'playing' ] = self.playing
		self.commands[ 'playlist' ] = self.playlist
		self.commands[ 'position?' ] = self.position
		self.commands[ 'prev' ] = self.prev
		self.commands[ 'previous' ] = self.previous
		self.commands[ 'props' ] = self.props
		self.commands[ 'seek' ] = self.seek
		self.commands[ 'speed' ] = self.speed
		self.commands[ 'speed?' ] = self.speed_query
		self.commands[ 'system' ] = self.system

		self.commands[ 'pitch_filter' ] = self.pitch
		self.commands[ 'volume_filter' ] = self.volume
		self.commands[ 'deinterlace_filter' ] = self.deinterlace

		prop = self.stack.property( "query" )
		prop.attach( self )

	def start( self ):
		self.thread.start( )

	def dot( self ):
		"""Override the . to force play of the top of stack."""

		self.push( 'length?' )
		self.push( 'dot' )
		length = int( self.stack.fetch_slot( 0 ).get_uri( ) )
		if length > 0:
			self.push( 'dot' )
			self.thread.set( self.stack.fetch_slot( 0 ) )
		else:
			self.push( 'render' )
			self.push( 'drop' )

	def clone_node( self, node ):
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
		self.push( 'dot' )
		tos = self.stack.fetch_slot( 0 )
		self.stack.connect( tos, 0 )
		self.push( 'recover' )
		self.clone_node( tos )

	def pitch( self ):
		if self.pitch_ is not None:
			self.stack.connect( self.pitch_, 0 )
			self.push( 'recover' )

	def volume( self ):
		if self.volume_ is not None:
			self.stack.connect( self.volume_, 0 )
			self.push( 'recover' )

	def deinterlace( self ):
		if self.deinterlace_ is not None:
			self.stack.connect( self.deinterlace_, 0 )
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
		self.push( 'dup', 'dot' )
		input = self.stack.fetch_slot( 0 )
		if input is not None:
			properties = input.properties( )
			for key in properties.get_keys( ):
				print key, 
			print

	def help( self ):
		"""Nasty help output."""

		self.push( "dict" )

		print
		print "Shell Extensions:"
		print

		output = ''
		for cmd in sorted( self.commands ):
			if output == '':
				output += cmd 
			else:
				output += ', ' + cmd 

		print output

	def playlist( self ):
		"""Dump the playlist to stdout."""

		self.thread.cond.acquire( )
		for input in self.thread.input:
			render = ml.create_filter( 'aml' )
			render.property( 'filename' ).set( unicode( '-' ) )
			render.connect( input, 0 )
			print
		self.thread.cond.release( )

	def previous( self ):
		self.thread.cond.acquire( )
		for input in self.thread.prev_inputs:
			render = ml.create_filter( 'aml' )
			render.property( 'filename' ).set( unicode( '-' ) )
			render.connect( input, 0 )
			print
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

	def normalise( self, input ):
		self.thread.cond.acquire( )

		self.stack.connect( input, 0 )
		self.push( 'recover' )

		frame = input.fetch( )

		if input.is_thread_safe( ):
			self.push( 'filter:threader', 'active=1' )

		self.push( 'filter:deinterlace' )
		self.push( 'dot', 'recover' )
		self.deinterlace_ = self.stack.fetch_slot( 0 )

		self.push( 'filter:pitch', 'speed=1' )
		self.push( 'dot', 'recover' )
		self.pitch_ = self.stack.fetch_slot( 0 )

		self.push( 'filter:volume' )
		self.push( 'dot', 'recover' )
		self.volume_ = self.stack.fetch_slot( 0 )

		if frame.get_image( ) is None:
			self.push( 'visualise' )

		if frame.get_audio( ) is None:
			self.push( 'filter:conform' )

		self.push( 'dot' )

		result = self.stack.fetch_slot( 0 )

		self.thread.cond.release( )

		return result

