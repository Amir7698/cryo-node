# Build and run the PC unit tests for core/.
# Usage, from the repository root:
#   powershell -ExecutionPolicy Bypass -File tests/run_tests.ps1

$ErrorActionPreference = "Stop"
Set-Location (Join-Path $PSScriptRoot "..")   # works from any folder
New-Item -ItemType Directory -Force build | Out-Null

gcc -std=c99 -Wall -Wextra -Werror -Icore/include `
    core/src/alarm.c tests/test_alarm.c -o build/test_alarm.exe
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

./build/test_alarm.exe
exit $LASTEXITCODE
