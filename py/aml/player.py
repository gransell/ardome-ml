from aml import ml, pl, stack

video_filters = []
video_filters.append( [ 'thread_safe', 'threader', 'active=1' ] )
video_filters.append( [ 'always', 'deinterlace' ] )
video_filters.append( [ 'always', 'pitch', 'speed=1' ] )
video_filters.append( [ 'always', 'volume', 'volume=1' ] )
video_filters.append( [ 'always', 'resampler' ] )
video_filters.append( [ 'always', 'visualise' ] )
video_filters.append( [ 'has_no_audio', 'conform' ] )
video_filters.append( [ 'always', 'correction', 'enable=0' ] )

audio_filters = []
audio_filters.append( [ 'thread_safe', 'threader', 'active=1' ] )
audio_filters.append( [ 'always', 'pitch', 'speed=1' ] )
audio_filters.append( [ 'always', 'volume', 'volume=1' ] )
audio_filters.append( [ 'always', 'resampler' ] )
audio_filters.append( [ 'has_no_audio', 'conform' ] )

class player:
	def __init__( self, filters = video_filters ):
		"""Constructor."""

		self.stack = stack( )
		self.stack.include( 'core.aml' )

		self.filters = filters

		self.instances = {}

		self.stack.push( 'test:' )
		for filter in self.filters:
			if filter[ 0 ] == 'always':
				self.stack.push( 'filter:' + filter[ 1 ], filter[ 2: ], 'dup' )
				self.instances[ filter[ 1 ] ] = self.stack.pop( )
		self.stack.push( 'clear' )

		self.stores = []
		self.initted = False
		self.deferrable = False

	def create( self, args = [ ] ):
		"""Create the stores requested or the default if none are specified."""

		if len( args ) == 0: args = [ 'preview:' ]
		dummy = ml.create_input( 'colour:' )
		self.stores = []
		self.initted = False
		for arg in args:
			if arg.find( '=' ) < 0:
				self.stores.append( ml.create_store( arg, dummy.fetch( ) ) )
				if self.stores[ -1 ] is None:
					raise Exception, "Unable to create store %s" % arg
			else:
				name, value = arg.split( '=', 1 )
				prop = self.stores[ -1 ].property( name )
				if prop.valid( ):
					prop.set_from_string( value )
				elif name.startswith( '@' ):
					key = pl.create_key_from_string( name )
					prop = pl.property( key )
					prop.set( unicode( value ) )
					self.stores[ -1 ].properties( ).append( prop )
				else:
					print 'Requested property %s not found found - ignoring.' % name

	def init( self ):
		"""Init the stores for use - note that we can invoke create first, 
		then set additional properties and finally invoke init - this is to
		allow the use of write once properties like the winid of the video
		preview stores."""

		if len( self.stores ) == 0: self.create( )
		if not self.initted:
			self.deferrable = True
			self.initted = True
			for store in self.stores:
				self.initted = self.initted and store.init( )
				self.deferrable = self.deferrable and \
								  store.property( "deferrable" ).valid( ) and \
								  store.property( "deferrable" ).value_as_int( ) == 1
		return self.initted

	def ante( self, input, stores, frame ):
		"""Method called before push to stores. Returns true if termination 
		required."""

		return False

	def post( self, input, stores, frame, speed = 1, eof = True ):
		"""Method called after push to stores. Returns true if termination 
		required."""

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

		if rule[ 1 ] not in self.instances:
			self.stack.push( 'filter:' + rule[ 1 ] )
			self.stack.push( rule[ 2: ] )
			self.stack.push( 'dup' )
			self.instances[ rule[ 1 ] ] = self.stack.pop( )
		else:
			filter = self.instances[ rule[ 1 ] ]
			filter.connect( ml.create_input( 'nothing:' ), 0 )
			self.stack.push( filter )

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

		self.walk_and_assign( input, "deferred", int( self.deferrable ) )

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

	def play( self, input ):
		"""Passes frames from input to stores in a loop."""

		self.init( )
		input = self.check_input( input )

		error = False
		frame = None
	
		try:
			while not error and input.get_position( ) < input.get_frames( ):
				if frame is None or frame.get_position( ) != input.get_position( ):
					frame = input.fetch( )
				error = self.ante( input, self.stores, frame )
				if not error:
					for store in self.stores:
						if not store.push( frame ): 
							error = True
				yield frame
				error = self.post( input, self.stores, frame ) or error
	
		except KeyboardInterrupt:
			print "Aborted"

		if not error:
			for store in self.stores:
				store.complete( )

		yield None

