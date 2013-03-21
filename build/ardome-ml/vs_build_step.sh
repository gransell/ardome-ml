if [ $# -ne 1 ]; then
	echo "Usage: vs_build_step.sh target (vs2008 or vs2010)"
	exit 1
fi

if [ $1 != "vs2008" -a $1 != "vs2010" ]; then
	echo "Unknown target: $1"
	exit 1
fi

TARGET=$1

echo "Starting SCons to build release and debug versions using Visual Studio $TARGET... "

scons.bat -j4 target=$TARGET

if [ ! $? == 0 ]; then
  echo "Build failed. Terminating."
  exit 1
fi

echo "Running doxygen..."
bcomp/$TARGET/doxygen.exe doc/aml_windows.dox

if [ ! $? == 0 ]; then
    echo "Building of ducumentation failed. Terminating."
    exit 1
fi

echo "Building succeeded!"

