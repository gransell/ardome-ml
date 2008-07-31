import aml
import threading
import readline

class thread_shell( threading.Thread ):
	def __init__( self, player ):
		threading.Thread.__init__( self )
		self.player = player
		self.stack = aml.thread_stack( player )

	def run( self ):
		stack = self.stack
		stack.include( 'shell.aml' )
		readline.parse_and_bind("tab: complete")
		try:
			while stack.active( ):
				try:
					input = raw_input( 'aml> ' ).strip( )
					# Gack! drag and drop from nautilus...
					input = input.replace( "\'", "\"" ).replace( "\"\\\"\"", "\'" )
					if input != '':
						try:
							stack.define( input )
						except Exception, e:
							print e
				except KeyboardInterrupt:
					try:
						stack.push( "reset!" )
					except Exception, e:
						print '\n' + str( e )
		except EOFError:
			print

		if stack.active( ):
			stack.push( "play" )
			stack.push( "exit" )

