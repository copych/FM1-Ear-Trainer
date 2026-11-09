param([int]$Port = 8010)
$ErrorActionPreference = 'Stop'
$taskRoot = Split-Path -Parent $PSScriptRoot
if (-not (Test-Path -LiteralPath (Join-Path $taskRoot 'build/site/emu/melodee.wasm'))) {
    throw 'Build the site first using tools/build-melodee.sh site in WSL.'
}
if (Get-NetTCPConnection -LocalPort $Port -State Listen -ErrorAction SilentlyContinue) {
    throw "Port $Port is in use; choose another with -Port."
}
$nodePath = (Get-Command node.exe).Source
$serverPath = Join-Path $PSScriptRoot 'serve-site.cjs'
$server = Start-Process -FilePath $nodePath -ArgumentList @(('"' + $serverPath + '"'), "$Port") -WorkingDirectory $taskRoot -WindowStyle Hidden -PassThru
Write-Output "Ear Trainer: http://127.0.0.1:$Port/ (PID $($server.Id))"
