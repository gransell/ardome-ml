import SCons.Util
import msvc

def exists(env):
	return msvc.exists(env)
	
def generate(env):
	msvc.run_vcvars_bat( env, msvc.current_vc_version( env ) )
	
	env['AR']		  = 'lib'
	env['ARFLAGS']	 = SCons.Util.CLVar('/nologo')
	env['ARCOM']	   = "${TEMPFILE('$AR $ARFLAGS /OUT:$TARGET $SOURCES')}"
	env['LIBPREFIX']   = ''
	env['LIBSUFFIX']   = '.lib'
	
	