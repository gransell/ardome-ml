import amlx
from Tkinter import *

class audio( Frame ):

	"""Provides a basic audio widget which connects to a remote server."""

	def __init__( self, master, client ):

		"""Constructor sets up the widget, keyboard handling and timer event
		handling for the scrub widget."""

		Frame.__init__( self, master )

		self.client = client

		self.bind_all( '<KeyPress-Escape>', self.exit )

		pitch_, volume_ = self.status( )

		frame = Frame( master )
		frame.pack( fill = X )

		self.pitch = DoubleVar( )
		self.pitch.set( pitch_ )

		Button( frame, text = 'Pitch', width = 10, command = lambda self = self: self.command( '1.0 pitch' ) ).pack( side = LEFT )
		self.scrub = Scale( frame, command = self.scrub, orient = HORIZONTAL, showvalue = False, from_ = 0.05, to_ = 5.0, variable = self.pitch, digits = 3, resolution = 0.05, takefocus = 0 )
		self.scrub.pack( fill = X )

		frame = Frame( master )
		frame.pack( fill = X )

		self.volume = DoubleVar( )
		self.volume.set( volume_ )

		Button( frame, text = 'Volume', width = 10, command = lambda self = self: self.command( '1.0 volume' ) ).pack( side = LEFT )
		self.volume_scrub = Scale( frame, command = self.vscrub, orient = HORIZONTAL, showvalue = False, from_ = 0.0, to_ = 5.0, variable = self.volume, digits = 3, resolution = 0.05, takefocus = 0 )
		self.volume_scrub.pack( fill = X )

		self.after( 500, self.timer )

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
		pitch_ = 1.0
		volume_ = 1.0

		result = self.command( 'pitch? . volume? .' )
		if len( result ) == 2:
			pitch_ = float( result[ 0 ] )
			volume_ = float( result[ 1 ] )
		else:
			raise Exception, "Invalid status response from server"

		return pitch_, volume_

	def scrub( self, value ):
		self.command( "%f pitch" % float( value ) )

	def vscrub( self, value ):
		self.command( "%f volume" % float( value ) )

	def timer( self ):
		pitch_, volume_ = self.status( )
		self.pitch.set( pitch_ )
		self.volume.set( volume_ )
		self.after( 500, self.timer )

def run( server = 'localhost', port = 55378 ):
	client = amlx.client( server, port )
	root = Tk( )
	root.resizable( 0, 0 )
	app = audio( root, client )
	root.mainloop( )

