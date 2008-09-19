import threading
import time
import os
import string

from aml.player import player, ml, pl

class thread_player( player ):
	"""Advanced playlist oriented threaded player."""

	def __init__( self ):
		"""Constructor."""

		player.__init__( self )
		self.lock = threading.RLock( )
		self.cond = threading.Condition( self.lock )
		self.next_input = False
		self.current = None
		self.prev_inputs = [ ]
		self.prev_count = 10
		self.prev_input = False
		self.input = [ ]
		self.speed = 1
		self.position_ = -1
		self.current_position = -1
		self.exiting = False
		self.generation = 0
		self.client_count = 0

	def register( self, stack ):
		return player_stack( stack, self )

	def control_start( self ):
		self.cond.acquire( )
		self.client_count += 1
		self.cond.release( )

	def control_end( self ):
		self.cond.acquire( )
		self.client_count -= 1
		self.cond.notifyAll( )
		self.cond.release( )

	def set( self, input ):
		"""Request immediate playout of input."""

		self.cond.acquire( )
		try:
			self.input = [ input ]
			self.speed = 1
			self.next_input = True
			self.generation += 1
			self.cond.notifyAll( )
		finally:
			self.cond.release( )

	def add( self, input ):
		"""Append to playout list."""

		self.cond.acquire( )
		try:
			self.input += [ input ]
			self.generation += 1
			self.cond.notifyAll( )
		finally:
			self.cond.release( )

	def next( self ):
		"""Play next in list."""

		self.cond.acquire( )
		try:
			if len( self.input ):
				self.next_input = True
				self.cond.notifyAll( )
		finally:
			self.cond.release( )

	def prev( self ):
		"""Play previous item."""

		self.cond.acquire( )
		try:
			if len( self.prev_inputs ):
				self.prev_input = True
				self.cond.notifyAll( )
		finally:
			self.cond.release( )

	def set_speed( self, speed ):
		"""Set the speed of the playout."""

		self.cond.acquire( )
		try:
			self.speed = speed
			self.cond.notifyAll( )
		finally:
			self.cond.release( )

	def get_speed( self ):
		"""Return the speed of playout."""

		return self.speed

	def seek( self, position ):
		"""Seek to the requested frame position."""

		self.cond.acquire( )
		try:
			if self.current is not None:
				if position >= 0:
					self.position_ = position
				else:
					self.position_ = self.current.get_frames( ) + position
				if self.position_ < 0:
					self.position_ = 0
				elif self.position_ >= self.current.get_frames( ):
					self.position_ = self.current.get_frames( ) - 1
				self.cond.notifyAll( )
				self.cond.wait( 0.25 )
		finally:
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
		is_active = self.client_count > 0
		self.cond.release( )
		return is_active

	def grab( self, index ):
		"""Retrieve an input from the playlist (remove and return)."""

		self.cond.acquire( )
		result = None
		if index >= 0 and index < len( self.input ):
			result = self.input.pop( index )
			self.generation += 1
		elif index == -1 and self.current is not None:
			result = self.current
		elif index < -1 and - ( index + 2 ) < len( self.prev_inputs ):
			result = self.prev_inputs.pop( len( self.prev_inputs ) + index + 1 )
		self.cond.release( )
		return result

	def insert( self, index, input ):
		"""Insert an input into the playlist."""

		self.cond.acquire( )
		if index >= 0 and index < len( self.input ):
			self.input.insert( index, input )
		else:
			self.input.append( input )
		self.generation += 1
		self.cond.release( )

	def post( self, input, stores, frame, eof = False ):
		"""Method called after push to stores. Returns false if no error
		detected."""

		self.cond.acquire( )
		if not self.active( ):
			eof = True
		elif self.position_ >= 0:
			input.seek( self.position_, False )
			self.position_ = -1
		elif input.get_position( ) + self.speed < input.get_frames( ):
			input.seek( self.speed, True )
		elif len( self.input ) > 0:
			eof = True
		self.current_position = input.get_position( )
		self.cond.release( )
		return eof

	def get_instance( self, key ):
		"""Thread safe version - get the filter instance required."""

		self.cond.acquire( )
		try:
			result = player.get_instance( self, key )
		finally:
			self.cond.release( )
		return result

	def check_input( self, input ):
		"""Thread safe version - check the input."""

		self.cond.acquire( )
		try:
			result = player.check_input( self, input )
		finally:
			self.cond.release( )
		return result

	def run( self ):
		"""Thread execution loop."""

		while self.active( ):
			self.cond.acquire( )

			while self.active( ) and len( self.input ) == 0 and not self.next_input and not self.prev_input:
				self.cond.wait( 0.1 )

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

			# Schedule the first input for playout
			self.current = self.input[ 0 ]
			self.current.seek( 0, False )
			iter = self.play( self.current )
			self.input.pop( 0 )
			self.next_input = False
			self.prev_input = False
			self.generation += 1
			self.cond.release( )

			# Loop while playing current input
			running = True
			while running and self.active( ):
				self.cond.acquire( )
				running = not self.next_input and not self.prev_input and iter.next( ) is not None
				self.cond.notifyAll( )
				if running and self.speed == 0:
					self.cond.wait( 0.1 )
				self.cond.release( )
				time.sleep( 0.0001 )

		self.close( )

class player_stack:

	def __init__( self, stack, player ):

		self.stack = stack
		self.push = self.stack.push
		self.pop = self.stack.pop
		self.dot = self.stack.dot
		self.output = self.stack.output

		self.player = player

		self.commands = { }
		self.commands[ 'add' ] = self.add
		self.commands[ 'grab' ] = self.grab
		self.commands[ 'insert' ] = self.insert
		self.commands[ 'next' ] = self.next
		self.commands[ 'playing' ] = self.playing
		self.commands[ 'playlist' ] = self.playlist
		self.commands[ 'position?' ] = self.position
		self.commands[ 'prev' ] = self.prev
		self.commands[ 'previous' ] = self.previous
		self.commands[ 'seek' ] = self.seek
		self.commands[ 'speed' ] = self.speed
		self.commands[ 'speed?' ] = self.speed_query
		self.commands[ 'status?' ] = self.status
		self.commands[ 'store' ] = self.store
		self.commands[ 'play_filter' ] = self.play_filter

	def play_filter( self ):
		"""Obtains the specified play filter and places it on the stack."""

		if self.stack.next_command is None:
			self.stack.next_command = self.play_filter
		else:
			self.stack.next_command = None
			self.push( self.player.get_instance( self.stack.incoming ) )

	def speed( self ):
		"""Set the speed to the number at the top of the stack."""

		self.player.set_speed( string.atoi( self.pop( ).get_uri( ) ) )

	def speed_query( self ):
		""" Put the current playout speed on top of the stack."""

		self.push( self.player.get_speed( ) )

	def add( self ):
		"""Add top of stack to playlist."""

		tos = self.pop( )
		if tos.get_frames( ) > 0:
			self.player.add( tos )

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

	def playlist( self ):
		"""Dump the playlist to stdout."""

		self.player.cond.acquire( )
		try:
			for input in self.player.input:
				self.stack.aml.connect( input, 0 )
				text = self.stack.aml_stdout.value_as_string( )
				self.output( text )
				self.stack.aml_write.set( 1 )
				self.output( '' )
		finally:
			self.player.cond.release( )

	def previous( self ):
		"""Dump the previously played list to stdout."""

		self.player.cond.acquire( )
		try:
			for input in self.player.prev_inputs:
				self.stack.aml.connect( input, 0 )
				text = self.stack.aml_stdout.value_as_string( )
				self.output( text )
				self.stack.aml_write.set( 1 )
				self.output( '' )
		finally:
			self.player.cond.release( )

	def playing( self ):
		"""Dump the current playing item to stdout."""

		self.player.cond.acquire( )
		try:
			if self.player.current is not None:
				self.stack.aml.connect( self.player.current, 0 )
				text = self.stack.aml_stdout.value_as_string( )
				self.output( text )
				self.stack.aml_write.set( 1 )
				self.output( '' )
		finally:
			self.player.cond.release( )

	def status( self ):
		"""Unsure about this one... used by the transport client to obtain
		playlist generation, frames in current played item, position and
		speed."""

		self.player.cond.acquire( )
		try:
			self.push( self.player.generation )
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
			self.dot( )
		finally:
			self.player.cond.release( )

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
		if self.stack.next_command is None:
			self.stack.next_command = self.store
			self.store_props = [ ]
		elif self.stack.incoming != '.':
			self.store_props += [ str( self.stack.incoming ) ]
		else:
			self.player.cond.acquire( )
			try:
				self.stack.next_command = None
				if isinstance( self.player.stores, list ) and len( self.player.stores ) and isinstance( self.player.stores[0], str ):
					self.player.store += self.store_props
				else:
					result = self.player.stores
					for pair in self.store_props:
						name, value = pair.split( '=', 1 )
						prop = result[ 0 ].property( name )
						if value.find( '%s' ) != -1:
							value = value % str( self.stack.pop( ).get_uri( ) )
						if prop.valid( ):
							prop.set_from_string( value )
						elif name.startswith( '@' ):
							key = pl.create_key_from_string( name )
							prop = pl.property( key )
							prop.set( unicode( value ) )
							result[ 0 ].properties( ).append( prop )
						else:
							print 'Requested property %s not found found - ignoring.' % name

					prop = result[ 0 ].property( 'sync' )
					if prop.valid( ):
						prop.set( 1 )
			finally:
				self.player.cond.release( )

