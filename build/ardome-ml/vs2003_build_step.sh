echo "Starting Visual Studio .NET 2003 to build release version... "
echo -n > aml.vc71.sln.release.build.log

"${PROGRAMFILES}/Microsoft Visual Studio .NET 2003/Common7/IDE/devenv.exe" bcomp/ardome-ml/vs2003/ardome_media_library.sln /rebuild release /out aml.vc71.sln.release.build.log

if [ ! $? == 0 ]; then
  cat aml.vc71.sln.release.build.log
  echo "Release build failed. Terminating."
  exit 1
fi

echo "Starting Visual Studio .NET 2003 to build debug version... "
echo -n > aml.vc71.sln.debug.build.log
"${PROGRAMFILES}/Microsoft Visual Studio .NET 2003/Common7/IDE/devenv.exe" bcomp/ardome-ml/vs2003/ardome_media_library.sln /rebuild debug /out aml.vc71.sln.debug.build.log

if [ ! $? == 0 ]; then
  cat aml.vc71.sln.debug.build.log
  echo "Debug build failed. Terminating."
  exit 1
fi	

echo "Building succeeded!"







