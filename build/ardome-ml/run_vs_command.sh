THIS_COMMAND=`basename $0`

if [ $# -le 1 ]; then
	echo "Usage: $THIS_COMMAND <vs_version> <command>"
	echo "<vs_version> may be one of vs2008, vs2010, vs2012"
	echo "For example, to run the cl.exe tool from VS 2010 on the file main.cpp, use:"
	echo "$THIS_COMMAND vs2010 cl.exe main.cpp"
	exit 1
fi

VS_VERSION="$1"
shift
VS_COMMAND_TO_RUN="$@"

if [ "$VS_VERSION" == "vs2008" ]; then
	VS_VERSION_NUMBER="9.0"
elif [ "$VS_VERSION" == "vs2010" ]; then
	VS_VERSION_NUMBER="10.0"
elif [ "$VS_VERSION" == "vs2012" ]; then
	VS_VERSION_NUMBER="11.0"
fi

if [ -z "$VS_VERSION_NUMBER" ]; then
	echo "ERROR: Unrecognized VS version string: \"$VS_VERSION\""
	echo "Allowed values are: vs2008, vs2010, vs2012"
	exit 1
fi


if [ ! -d "/proc/registry" ]; then
	echo "Could not locate /proc/registry. This script doesn't seem to be running under Cygwin."
	exit 1
fi

# Note that on 64-bit Windows machines, this key is actually located at:
# HKEY_LOCAL_MACHINE/SOFTWARE/Wow6432Node/Microsoft/VisualStudio/
# so that's where it can be found in regedit.
MS_VS_REG_KEY="/proc/registry/HKEY_LOCAL_MACHINE/SOFTWARE/Microsoft/VisualStudio/$VS_VERSION_NUMBER/Setup/VS/ProductDir"
if [ ! -f "$MS_VS_REG_KEY" ]; then
	echo "Could not locate installation registry key in filesystem. Visual Studio $VS_VERSION doesn't seem to be installed."
	exit 1
fi
MS_VS_DIR=`cat "$MS_VS_REG_KEY"`
echo "Found VS installation at: $MS_VS_DIR"


# Now, we set up a temporary batch file which first calls the
# vcvarsall.bat script from the Visual Studio installation
# to set up the environment correctly for the tool.
# After that, the tool itself is run.

function clean_temp_bat_file
{
	rm -f $TEMP_BAT_FILE
}
TEMP_BAT_FILE=`mktemp --suffix=.bat`
if [ $? -ne 0 ]; then
	echo "ERROR: Failed to create temporary batch file. TMP/TEMP not set?"
	exit 1
fi
trap "clean_temp_bat_file" EXIT

chmod u+x $TEMP_BAT_FILE
echo "@echo off" >> $TEMP_BAT_FILE
echo "call \"$MS_VS_DIR\\VC\\vcvarsall.bat\"" >> $TEMP_BAT_FILE
echo "$VS_COMMAND_TO_RUN" >> $TEMP_BAT_FILE

$TEMP_BAT_FILE
exit $?

