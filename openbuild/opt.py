import SCons.Options

def create_options( file, args ):
	opts = SCons.Options.Options( file, args )

	opts.file = file

	opts.Add( 'prefix', "Directory of architecture independant files.", "/usr/local" )
	opts.Add( 'distdir', "Directory to actually install to.  Prefix will be used inside this.", "" )
	opts.Add( 'debug', "Debug or release - 1 or 0 resp.", '0' )

	return opts
