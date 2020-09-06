set binFolder="..\bin\Release-x86_64-windows\RoadefGoogleChallenge\"

call Win-Compile.bat

pushd %binFolder%
    pushd Assets\Instances
        for %%x in (*.txt) do ..\..\RoadefGoogleChallenge.exe check %%x ..\Solutions\%%x ..\Solutions\%%x
    popd
popd

pause