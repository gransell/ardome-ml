import aml
from Tkinter import *

class transport( aml.client ):

	def __init__( self, master, server = 'localhost', port = 55378 ):

		aml.client.__init__( self, server, port )

		frame = Frame( master )
		frame.pack( )

		self.button_prev = Button( frame, text= '|<<<', command=self.prev )
		self.button_prev.pack( side = LEFT )

		self.button_start = Button( frame, text= '|<<', command=self.start )
		self.button_start.pack( side = LEFT )

		self.button_rew = Button( frame, text= '<<', command=self.rew )
		self.button_rew.pack( side = LEFT )

		self.button_step_back = Button( frame, text= '|<', command=self.step_back )
		self.button_step_back.pack( side = LEFT )

		self.button_reverse = Button( frame, text= '<', command=self.reverse )
		self.button_reverse.pack( side = LEFT )

		self.button_toggle = Button( frame, text= '||>', command=self.toggle )
		self.button_toggle.pack( side = LEFT )

		self.button_play = Button( frame, text= '>', command=self.play )
		self.button_play.pack( side = LEFT )

		self.button_step_fwd = Button( frame, text= '>|', command=self.step_fwd )
		self.button_step_fwd.pack( side = LEFT )

		self.button_ffwd = Button( frame, text= '>>', command=self.ffwd )
		self.button_ffwd.pack( side = LEFT )

		self.button_next = Button( frame, text= '>>>|', command=self.next )
		self.button_next.pack( side = LEFT )

	def command( self, cmd ):
		self.send( cmd )
		self.receive( )

	def prev( self ):
		self.command( 'prev' )

	def start( self ):
		self.command( '0 seek' )

	def rew( self ):
		self.command( 'rew' )

	def step_back( self ):
		self.command( 'pause -1 step' )

	def reverse( self ):
		self.command( 'reverse' )

	def toggle( self ):
		self.command( 'toggle' )

	def play( self ):
		self.command( 'play' )

	def step_fwd( self ):
		self.command( 'pause 1 step' )

	def ffwd( self ):
		self.command( 'ffwd' )

	def next( self ):
		self.command( 'next' )

def run( server = 'localhost', port = 55378 ):
	root = Tk( )
	app = transport( root, server, port )
	root.mainloop( )

