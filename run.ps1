# run.ps1 — сборка и запуск Phase Portrait (MinGW UCRT64, CMake).
#
# По умолчанию: отладочная сборка в build\Debug (динамический SFML + DLL), затем запуск.
#   .\run.ps1
#   .\run.ps1 -Configure     # пересоздать CMake-кэш
#   .\run.ps1 -RunOnly       # только запустить уже собранный Debug exe
#
# Релиз (портативный exe, SFML статически) в build\Release:
#   .\run.ps1 -Release
#   .\run.ps1 -Release -Configure
#   .\run.ps1 -Release -Run           # после сборки запустить
#   .\run.ps1 -Release -RunOnly       # только запустить Release exe
param(
    [switch]$Release,
    [switch]$RunOnly,
    [switch]$Configure,
    [switch]$Run
)

$ErrorActionPreference = "Stop"
$Root     = $PSScriptRoot
$BuildDir = if ($Release) { Join-Path $Root "build\Release" } else { Join-Path $Root "build\Debug" }
$Exe      = Join-Path $BuildDir "PhasePortrait.exe"

$env:PATH = "C:\msys64\ucrt64\bin;" + $env:PATH

if ($RunOnly) {
    if (-not (Test-Path $Exe)) {
        Write-Host "Не найден: $Exe (соберите проект без -RunOnly)." -ForegroundColor Red
        exit 1
    }
    Set-Location $BuildDir
    & .\PhasePortrait.exe
    Set-Location $Root
    exit $LASTEXITCODE
}

if (-not (Test-Path $BuildDir)) {
    New-Item -ItemType Directory -Path $BuildDir -Force | Out-Null
}

Set-Location $BuildDir

$cache = Join-Path $BuildDir "CMakeCache.txt"
if ($Configure -or -not (Test-Path $cache)) {
    $static = if ($Release) { "ON" } else { "OFF" }
    cmake $Root -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release `
        "-DPORTABLE_STATIC_SFML=$static" `
        -DCMAKE_C_COMPILER="C:/msys64/ucrt64/bin/gcc.exe" `
        -DCMAKE_CXX_COMPILER="C:/msys64/ucrt64/bin/g++.exe"
    if ($LASTEXITCODE -ne 0) { Set-Location $Root; exit $LASTEXITCODE }
}

mingw32-make -j $env:NUMBER_OF_PROCESSORS

if ($LASTEXITCODE -ne 0) {
    Write-Host "Ошибка сборки!" -ForegroundColor Red
    Set-Location $Root
    exit $LASTEXITCODE
}

if (-not $Release) {
    Copy-Item (Join-Path $Root "libs\SFML\bin\*.dll") . -Force -ErrorAction SilentlyContinue
}

Set-Location $Root

if ($Release) {
    if (Test-Path $Exe) {
        $size = (Get-Item $Exe).Length / 1MB
        Write-Host ""
        Write-Host "Готово: $Exe" -ForegroundColor Green
        Write-Host ("Размер: {0:N2} MB" -f $size)
        Write-Host "Переносите только этот exe (Windows x64, SFML вшит)." -ForegroundColor DarkGray
    } else {
        Write-Host "Предупреждение: $Exe не найден." -ForegroundColor Yellow
    }
    if ($Run) {
        & $Exe
        exit $LASTEXITCODE
    }
    exit 0
}

Write-Host "Сборка успешна, запускаем..." -ForegroundColor Green
Set-Location $BuildDir
& .\PhasePortrait.exe
$code = $LASTEXITCODE
Set-Location $Root
exit $code
