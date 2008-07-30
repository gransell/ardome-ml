from aml import ml, stack

class player:
	def __init__( self ):
		"""Constructor."""

		self.stack = stack( )
		self.stack.include( 'core.aml' )

		self.filters = []
		self.filters.append( [ 'thread_safe', 'threader', 'active=1' ] )
		self.filters.append( [ 'always', 'deinterlace' ] )
		self.filters.append( [ 'always', 'pitch', 'speed=1' ] )
		self.filters.append( [ 'always', 'volume', 'volume=1' ] )
		self.filters.append( [ 'has_no_image', 'visualise' ] )
		self.filters.append( [ 'has_no_audio', 'conform' ] )

		self.instances = {}

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

	def get_instance( self, key ):
		"""Get an instance filter associated to the playing input."""

		if key in self.instances.keys( ):
			return self.instances[ key ]
		return None

	def register_rule( self, rule ):
		"""Register a filter rule."""

		self.stack.push( 'filter:' + rule[ 1 ] )
		self.stack.push( rule[ 2: ] )
		self.stack.push( 'dup' )
		self.instances[ rule[ 1 ] ] = self.stack.pop( )

	def check_input( self, input ):
		"""Internal method: converts input to aml input object and associates
		minimalist filters to ensure we have audio/image and active threaders."""

		if isinstance( input, str ) or isinstance( input, unicode ):
			input = ml.create_input( input )
		elif isinstance( input, list ):
			stack.push( input )
			input = self.stack.pop( )

		if input is not None:

			self.stack.push( input )
			frame = input.fetch( )
			self.instances = {}

			for rule in self.filters:
				if rule[ 0 ] == 'thread_safe':
					if input.is_thread_safe( ):
						self.register_rule( rule )
				elif rule[ 0 ] == 'has_no_image':
					if frame.get_image( ) is None:
						self.register_rule( rule )
				elif rule[ 0 ] == 'has_no_audio':
					if frame.get_audio( ) is None:
						self.register_rule( rule )
				elif rule[ 0 ] == 'always':
					self.register_rule( rule )
				else:
					print "Unknown rule", str( rule )

			input = self.stack.pop( )

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

