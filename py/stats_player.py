from aml import player

import sys

class stats_player( player ):
	"""Subclass of player which reports position on stdout."""

	def __init__( self ):
		"""Constructor."""

		player.__init__( self )

	def ante( self, input, stores, frame ):
		"""Report position/duration."""

		print "%d/%d\r" %( input.get_position( ), input.get_frames( ) ), 
		sys.stdout.flush( )
		return player.ante( self, input, stores, frame )

def play( input, stores = None, renderer = stats_player ):
	"""Convenience method for simple, non-threaded, position reporting playout."""

	render = renderer( )
	iter = render.play( input, stores )
	while iter.next( ) is not None:
		pass
	print


