import SCons.Options
import utils
import os

def create_options( file, args ):
	opts = SCons.Options.Options( file, args )

	opts.file = file
	target = determine_target( )

	opts.Add( 'target', 'Target operating system and build environment.', target )
	opts.Add( 'prefix', 'Directory for installation.', '/usr/local' )
	opts.Add( 'distdir', 'Directory to actually install to.  Prefix will be used inside this.', '' )

	if target == 'osx':
		opts.Add( 'install_name', 'Bundle install name', '@loader_path' )

	return opts

def determine_target( ):
	target = 'unknown'
	if os.name == 'posix':
		if os.uname( )[ 0 ] == 'Darwin': target = 'osx'
		elif os.uname( )[ 0 ] == 'Linux' and utils.arch( ) == 'x86_64': target = 'ubuntu64'
		elif os.uname( )[ 0 ] == 'Linux' and utils.arch( ) == 'i686': target = 'ubuntu32'
		else: target = 'vs2003'
	elif os.name == 'win32':
		target = 'vs2003'
	elif os.name == 'nt':
		target = 'vs2003'
	return target
