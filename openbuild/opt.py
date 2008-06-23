# (C) Ardendo SE 2008
#
# Released under the LGPL

import SCons.Options
import utils
import os

valid_vs_targets =['vs2003', 'vs2005', 'vs2008', '' ]
valid_mac_targets = ['osx']
valid_32bit_unix_targets = ['ubuntu32']
valid_64bit_unix_targets = ['ubuntu64']

def check_value( key, value, env, correct_values) :
	if value in correct_values : return
	del env._dict[key]
	raise SCons.Errors.UserError, 'Invalid value for "%s" : "%s", must be one of the following: %s' % ( key, value, str(correct_values) )

def validate_vs_value( key, value, environment) :
	check_value( key, value, environment, valid_vs_targets )
		
def validate_target( key, value, environment ) :
	if os.name == 'win32' or os.name == 'nt' : check_value( key, value, environment, valid_vs_targets)
	elif os.name == 'posix':
		if os.uname( )[ 0 ] == 'Darwin': check_value( key, value, environment, valid_mac_targets)
		elif os.uname( )[ 0 ] == 'Linux' and utils.arch( ) == 'x86_64': check_value( key, value, environment, valid_64bit_unix_targets)
		elif os.uname( )[ 0 ] == 'Linux' and utils.arch( ) == 'i686': check_value( key, value, environment, valid_32bit_unix_targets)
	else :
		raise SCons.Errors.InternalError, "Unsupported operating system: %s" % (os.name) 
	
def create_options( file, args ):
	opts = SCons.Options.Options( file, args )

	opts.file = file
	target = determine_target( )

	opts.Add( 'target', 'Target operating system and build environment.', target, validate_target )
	opts.Add( 'prefix', 'Directory for installation.', '/usr/local' )
	opts.Add( 'distdir', 'Directory to actually install to.  Prefix will be used inside this.', '' )
	if os.name == 'win32' or os.name == 'nt' :
		desc = 'Will create Visual Studio vcproj and sln-files instead of building. Do not specify target. Valid values: %s' % ( str(valid_vs_targets) )
		opts.Add( 'vs', desc, '', validate_vs_value )

	if target == 'osx':
		opts.Add( 'install_name', 'Bundle install name', '@loader_path/../lib/' )

	return opts

def determine_target( ):
	target = 'unknown'
	if os.name == 'posix':
		if os.uname( )[ 0 ] == 'Darwin': target = 'osx'
		elif os.uname( )[ 0 ] == 'Linux' and utils.arch( ) == 'x86_64': target = 'ubuntu64'
		elif os.uname( )[ 0 ] == 'Linux' and utils.arch( ) == 'i686': target = 'ubuntu32'
		else: target = 'vs2003'
	elif os.name == 'win32':
		if utils.vs() is not None : target = utils.vs()
		else: target = 'vs2003'
	elif os.name == 'nt':
		target = 'vs2003'
	return target
