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

		speed_, position_, to_ = self.status( )

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
			print 'Caught', e 
			sys.exit( 1 )

	def status( self ):
		to_ = 0
		speed_ = 0
		position_ = 0

		result = self.command( 'status?' )
		if len( result ) == 3:
			speed_ = int( result[ 0 ] )
			position_ = int( result[ 1 ] )
			to_ = int( result[ 2 ] )
		else:
			raise Exception, "Invalid status response from server"

		return speed_, position_, to_

	def button( self, frame, text, cmd, keys = [ ], **kw ):
		def press_cb( self = self, cmd = cmd ): self.command( cmd )
		def event_cb( event, self = self, cmd = cmd ): self.command( cmd )
		button = Button( frame, text = text, command = press_cb, takefocus = 0, **kw )
		for key in keys: self.bind_all( key, event_cb )
		button.pack( side = LEFT )

	def scrub( self, value ):
		if not self.ignore:
			self.command( "pause %d seek" % int( value ) )
		self.ignore = False

	def timer( self ):
		speed_, position_, to_ = self.status( )
		#self.state.set( [ '||', '>' ][ int( speed_ == 0 ) ]  )
		current = self.scrub.cget( 'to' )
		if not self.ignore:
			self.ignore = current != to_
			self.scrub.config( to_ = to_ )
		self.position.set( position_ )
		self.after( 100, self.timer )

def run( server = 'localhost', port = 55378 ):
	client = amlx.client( server, port )
	root = Tk( )
	root.resizable( 0, 0 )
	app = transport( root, client )
	audio = amlx.audio.audio( root, client )
	root.mainloop( )

