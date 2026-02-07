@echo off
setlocal enabledelayedexpansion

if "%~3"=="" (
    echo Error: Missing required parameters
    echo Usage: %~nx0 ^<input_file^> ^<output_file^> ^<scetool_bin_dir^>
    echo Example: %~nx0 input.prx output.sprx "C:\tools\scetool\bin"
    exit /b 1
)

set "INPUT_FILE=%~1"
set "OUTPUT_FILE=%~2"
set "SCETOOL_DIR=%~3"

if not exist "%INPUT_FILE%" (
    echo Error: Input file not found: %INPUT_FILE%
    exit /b 1
)

if not exist "%SCETOOL_DIR%" (
    echo Error: scetool directory not found: %SCETOOL_DIR%
    exit /b 1
)

if not exist "%SCETOOL_DIR%\scetool.exe" (
    echo Error: scetool.exe not found in: %SCETOOL_DIR%
    exit /b 1
)

cd /d "%SCETOOL_DIR%"

echo Encrypting %INPUT_FILE% to %OUTPUT_FILE%...
scetool.exe -v --sce-type=SELF --compress-data=TRUE --skip-sections=TRUE --key-revision=1C --self-auth-id=1010000001000003 --self-vendor-id=01000002 --self-type=APP --self-app-version=0001000000000000 --self-fw-version=0004002000000000 --encrypt "%INPUT_FILE%" "%OUTPUT_FILE%" 2>&1

if errorlevel 1 (
    echo Error: scetool encryption failed
    exit /b 1
)

if not "%PS3_HOST%"=="" (
    echo Uploading to PS3 at %PS3_HOST%...
    for %%F in ("%OUTPUT_FILE%") do set "FILENAME=%%~nxF"
    curl -T "%OUTPUT_FILE%" "ftp://%PS3_HOST%/dev_hdd0/!FILENAME!"
    
    if errorlevel 1 (
        echo Warning: FTP upload failed
        exit /b 1
    )
    echo Upload complete
) else (
    echo PS3_HOST environment variable not set, skipping FTP upload
)

exit /b 0
