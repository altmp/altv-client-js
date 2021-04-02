@echo off

:: Build the project
cmake . -BBUILD
cmake --build BUILD --config Release

:: Copy built binary to dist folder
IF NOT EXIST dist (
    mkdir dist
)
copy BUILD\Release\altv-client-js.dll dist
copy BUILD\Release\altv-client-js.lib dist
copy BUILD\Release\altv-client-js.pdb dist

:: The extra.bat can be used to run an additional step after compilation,
:: like copying the file to your client directory automatically
IF EXIST tools\extra.bat (
    tools\extra.bat
)
