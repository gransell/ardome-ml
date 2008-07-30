import threading
import time

from aml.player import player

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

	def grab( self, index ):
		"""Retrieve an input from the playlist (remove and return)."""

		self.cond.acquire( )
		result = None
		if index >= 0 and index < len( self.input ):
			result = self.input.pop( index )
		self.cond.release( )
		return result

	def insert( self, index, input ):
		"""Insert an input into the playlist."""

		self.cond.acquire( )
		if index >= 0 and index < len( self.input ):
			self.input.insert( index, input )
		else:
			self.input.append( input )
		self.cond.release( )

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

	def get_instance( self, key ):
		"""Thread safe version - get the filter instance required."""

		self.cond.acquire( )
		result = player.get_instance( self, key )
		self.cond.release( )
		return result

	def check_input( self, input ):
		"""Thread safe version - check the input."""

		self.cond.acquire( )
		result = player.check_input( self, input )
		self.cond.release( )
		return result

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


