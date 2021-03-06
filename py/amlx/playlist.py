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
		tokens = [ ]
		for line in playlist:
			if line != "":
				for token in line.split( ' ' ):
					if token.startswith( '#' ): break
					if len( tokens ) != 0 and ( tokens[ -1 ].count( '\"' ) % 2 != 0 or tokens[ -1 ].endswith( '\\' ) ):
						tokens[ -1 ] += ' ' + token
					elif token != '':
						tokens.append( token )
			else:
				tokens.append( '' )

		result = [ '' ]
		for item in tokens:
			if item != '':
				if item[ 0 ] == '\"' and item[ -1 ] == '\"':
					item = item[ 1:-1 ]
				elif item[ 0 ] == '\'' and item[ -1 ] == '\'':
					item = item[ 1:-1 ]

				if not item.startswith( 'filter:' ) and item.find( '=' ) == -1:
					if item.find( '/' ) != -1:
						name = item[ item.rfind( '/' ) + 1 : ]
						result[ -1 ] += urllib.unquote( name ) + ' '
					else:
						result[ -1 ] += urllib.unquote( item ) + ' '
			else:
				result += [ '' ]
		return result

	def split( self, item ):
		return item.split( )
