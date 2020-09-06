set binFolder="..\bin\Release-x86_64-windows\RoadefGoogleChallenge\"

call Win-Compile.bat

pushd %binFolder%
    pushd Assets\Instances
        for %%x in (*.txt) do ..\..\RoadefGoogleChallenge.exe solve %%x ..\Solutions\%%x %%x.sol
    popd
popd

pause