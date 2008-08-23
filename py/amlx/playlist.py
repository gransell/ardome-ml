import amlx
import urllib
from Tkinter import *

class playlist( Frame ):

	"""Provides a basic playlist widget which is fed by a transport instance."""

	def __init__( self, master, transport ):

		"""Constructor sets up the widget."""

		Frame.__init__( self, master )

		self.transport = transport
		self.transport.display = self.display

		frame = Frame( master )
		frame.pack( expand = True, fill = BOTH )

		scrollbar = Scrollbar( frame, takefocus = False )
		scrollbar.pack( side = RIGHT, fill = Y )

		self.list = Listbox( frame, yscrollcommand = scrollbar.set )
		self.list.pack( side = LEFT, expand = True, fill = BOTH )
		scrollbar.config( command = self.list.yview )

	def display( self, playlist ):
		self.list.delete( 0, END )
		for item in self.extract( playlist ):
			if item != "":
				self.list.insert( END, item )

	def extract( self, playlist ):
		result = [ '' ]
		for item in playlist:
			if item != '':
				tokens = self.split( item )
				if not tokens[ 0 ].startswith( 'filter:' ) and tokens[ 0 ].find( '=' ) == -1:
					if tokens[ 0 ].find( '/' ) != -1:
						name = tokens[ 0 ][ tokens[ 0 ].rfind( '/' ) + 1 : ]
						result[ -1 ] += urllib.unquote( name ) + ' '
					else:
						result[ -1 ] += urllib.unquote( tokens[ 0 ] ) + ' '

			else:
				result += [ '' ]
		return result

	def split( self, item ):
		return item.split( )
