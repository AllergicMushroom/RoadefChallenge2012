pushd ..
    .\Scripts\premake5.exe vs2017
    msbuild RoadefGoogleChallenge.sln /p:Configuration=Release
popd