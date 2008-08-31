import amlx
from Tkinter import *

class transport( Frame ):

	"""Provides a basic transport widget which connects to a remote server."""

	def __init__( self, master, client ):

		"""Constructor sets up the widget, keyboard handling and timer event
		handling for the scrub widget."""

		Frame.__init__( self, master )

		self.client = client

		self.bind_all( '<KeyPress-Escape>', self.exit )

		speed_, position_, to_, generation_ = self.status( )

		self.generation = -1

		frame = Frame( master )
		frame.pack( )

		self.button( frame, '|<<<', 'prev play' )
		self.button( frame, '|<<', '0 seek' )
		self.button( frame, '<<', 'rew' )
		self.button( frame, '|<', 'pause -1 step', [ '<KeyPress-Left>' ] )
		self.button( frame, '<', 'reverse', [ '<KeyPress-j>' ] )
		self.state = StringVar( )
		self.state.set( '||' )
		self.button( frame, '||', 'toggle', [ '<KeyPress-k>' ], textvariable = self.state )
		self.button( frame, '>', 'play', [ '<KeyPress-l>' ] )
		self.button( frame, '>|', 'pause 1 step', [ '<KeyPress-Right>' ] )
		self.button( frame, '>>', 'ffwd' )
		self.button( frame, '>>>|', 'next play' )

		frame = Frame( master )
		frame.pack( fill = X )

		self.ignore = True
		self.position = IntVar( )
		self.position.set( position_ )

		self.scrub = Scale( frame, command = self.scrub, orient = HORIZONTAL, showvalue = False, from_ = 0, to_ = to_, variable = self.position, takefocus = 0 )
		self.scrub.pack( fill = X )
		self.scrub.bind( '<Button>', lambda event, self = self: self.scrub_down( event ) )
		self.scrub.bind( '<ButtonRelease>', lambda event, self = self: self.scrub_up( event ) )

		self.after( 100, self.timer )

	def exit( self, event ):
		"""Exit the application."""

		sys.exit( 0 )

	def command( self, cmd ):
		"""Run a command and return the result as an array of strings."""

		try:
			self.client.send( cmd )
			return self.client.receive( )
		except Exception, e:
			sys.exit( 1 )

	def status( self ):
		to_ = 0
		speed_ = 0
		position_ = 0
		generation_ = 0

		result = self.command( 'status?' )
		if len( result ) == 4:
			speed_ = int( result[ 0 ] )
			position_ = int( result[ 1 ] )
			to_ = int( result[ 2 ] )
			generation_ = int( result[ 3 ] )
		else:
			raise Exception, "Invalid status response from server"

		return speed_, position_, to_, generation_

	def button( self, frame, text, cmd, keys = [ ], **kw ):
		def press_cb( self = self, cmd = cmd ): self.command( cmd )
		def event_cb( event, self = self, cmd = cmd ): self.command( cmd )
		button = Button( frame, text = text, command = press_cb, takefocus = 0, **kw )
		for key in keys: self.bind_all( key, event_cb )
		button.pack( side = LEFT )

	def scrub_down( self, event ):
		result = self.command( 'speed? .' )
		if len( result ) == 1:
			self.previous_speed = int( result[ 0 ] )
		else:
			raise Exception, "Invalid status response from server"

	def scrub( self, value ):
		if not self.ignore:
			self.command( "pause %d seek" % int( value ) )
		self.ignore = False

	def scrub_up( self, event ):
		result = self.command( '%d speed' % self.previous_speed )
		if len( result ) != 0:
			raise Exception, "Invalid status response from server"

	def display( self, playlist ):
		pass

	def timer( self ):
		speed_, position_, to_, generation_ = self.status( )
		#self.state.set( [ '||', '>' ][ int( speed_ == 0 ) ]  )
		current = self.scrub.cget( 'to' )
		if current != to_:
			self.ignore = True
			self.scrub.config( to_ = to_ )
		self.position.set( position_ )
		if generation_ != self.generation:
			self.generation = generation_
			result = self.command( 'playing playlist' )
			self.display( result )
		self.after( 100, self.timer )

def run( server = 'localhost', port = 55378 ):
	client = amlx.client( server, port )
	root = Tk( )
	root.resizable( 0, 1 )
	app = transport( root, client )
	root.wm_minsize( root.winfo_reqwidth( ), root.winfo_reqheight( ) )
	audio = amlx.audio.audio( root, client )
	playlist = amlx.playlist.playlist( root, app )
	root.mainloop( )

