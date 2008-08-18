import amlx
from Tkinter import *

class tkplay( Frame ):
	def __init__( self, master, client ):
		Frame.__init__( self, master )

		self.client = client

		master.title( 'AML TK Player' )
		master.wm_minsize( 440, 300 )

		frame = Frame( master )
		frame.pack( fill = BOTH, expand = True )

		surface = Frame( frame, bg = "", colormap = "new" )
		surface.pack( fill = BOTH, expand = True )
		surface.bind( "<Configure>", self.resize )

		self.command( 'store @video.winid=%s sync=1 .' % str( surface.winfo_id( ) ) )

		transport = amlx.transport.transport( master, client )
		transport.pack( expand = False, fill = Y )

	def command( self, cmd ):
		"""Run a command and return the result as an array of strings."""

		try:
			self.client.send( cmd )
			return self.client.receive( )
		except Exception, e:
			print 'Caught', e 
			sys.exit( 1 )

	def resize( self, event ):
		self.command( 'store @video.width=%d @video.height=%d sync=1 .' % ( event.width, event.height ) )

def run( server = 'localhost', port = 55378 ):
	client = amlx.client( server, port )
	root = Tk( )
	app = tkplay( root, client )
	root.mainloop( )
