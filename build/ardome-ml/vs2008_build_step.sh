echo "Starting SCons to build release and debug versions using Visual Studio 2008... "

scons.bat -j4 target=vs2008

if [ ! $? == 0 ]; then
  echo "Build failed. Terminating."
  exit 1
fi

echo "Building succeeded!"


