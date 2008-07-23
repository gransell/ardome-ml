import aml
import sys
import readline

def main( args = sys.argv[ 1: ] ):
	readline.parse_and_bind("tab: complete")
	
	thread = aml.thread_stack( )

	if len( args ):
		thread.thread.set_store( args )

	thread.include( 'shell.aml' )

	thread.start( )
	
	try:
		while thread.active( ):
			try:
				input = raw_input( 'aml> ' ).strip( )
	
				# Gack! drag and drop from nautilus...
				input = input.replace( "\'", "\"" ).replace( "\"\\\"\"", "\'" )
	
				if input != '':
					try:
						thread.define( input )
					except Exception, e:
						print e
			except KeyboardInterrupt:
				try:
					thread.push( "reset!" )
				except Exception, e:
					print '\n' + str( e )

	except EOFError:
		print
	
	if thread.active( ):
		thread.exit( )

if __name__ == "__main__":
	main( )
