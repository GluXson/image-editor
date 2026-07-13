# run-gimp2.ps1
# Uruchamiaj z lokalizacji, gdzie znajduje się struktura projektu (np. image-editor-main)

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $ScriptDir

Write-Host "=== Sprawdzanie wymaganych komponentow ===" -ForegroundColor Cyan

function Test-Command($cmdName) {
    return [bool](Get-Command $cmdName -ErrorAction SilentlyContinue)
}

function Stop-WithPause($message) {
    Write-Host $message -ForegroundColor Red
    Read-Host "`nWystapil blad. Nacisnij Enter, aby zamknac okno"
    exit 1
}

# --- 1. Sprawdzenie CMake ---
if (-not (Test-Command "cmake")) {
    Write-Host "CMake nie znaleziony. Instaluje..." -ForegroundColor Yellow
    if (Test-Command "winget") {
        winget install --id Kitware.CMake -e --accept-source-agreements --accept-package-agreements
    } else {
        Stop-WithPause "Winget niedostepny. Zainstaluj CMake recznie: https://cmake.org/download/"
    }
} else {
    Write-Host "CMake OK: $(cmake --version | Select-Object -First 1)"
}

# --- 2. Sprawdzenie Qt (sciezka do Qt Tools / MinGW) ---
$qtRoot = "C:\Qt"
if (-not (Test-Path $qtRoot)) {
    Write-Host "Qt nie znaleziony w C:\Qt. Instaluje przez winget (moze wymagac recznej konfiguracji)..." -ForegroundColor Yellow
    if (Test-Command "winget") {
        winget install --id TheQtCompany.Qt.Online -e --accept-source-agreements --accept-package-agreements
    } else {
        Stop-WithPause "Zainstaluj Qt (z komponentem MinGW 13.1.0 64-bit oraz CMake 64bit) z https://www.qt.io/download-qt-installer"
    }
} else {
    Write-Host "Qt OK: $qtRoot"
}

# --- 3. Znajdz folder z CMakeLists.txt ---
$cmakeListsFile = Get-ChildItem -Path $ScriptDir -Filter "CMakeLists.txt" -Recurse -Depth 2 -ErrorAction SilentlyContinue | Select-Object -First 1

if (-not $cmakeListsFile) {
    Stop-WithPause "Nie znaleziono CMakeLists.txt w $ScriptDir ani w jego podfolderach."
}

$ProjectDir = $cmakeListsFile.DirectoryName
Write-Host "Znaleziono projekt w: $ProjectDir" -ForegroundColor Green

# --- 4. Znajdz konkretne pliki binarne MinGW (g++, gcc, mingw32-make) ---
$mingwBinDir = Get-ChildItem -Path $qtRoot -Directory -Filter "mingw*_64" -Recurse -ErrorAction SilentlyContinue |
    Where-Object { Test-Path (Join-Path $_.FullName "bin\g++.exe") } |
    Select-Object -First 1

if (-not $mingwBinDir) {
    Stop-WithPause "Nie znaleziono katalogu MinGW z g++.exe pod $qtRoot. Sprawdz czy Qt Maintenance Tool zainstalowal komponent MinGW 13.1.0 64-bit."
}

$mingwBin = Join-Path $mingwBinDir.FullName "bin"
$gppExe = Join-Path $mingwBin "g++.exe"
$gccExe = Join-Path $mingwBin "gcc.exe"
$makeExe = Join-Path $mingwBin "mingw32-make.exe"

if (-not (Test-Path $makeExe)) {
    Stop-WithPause "Nie znaleziono mingw32-make.exe w $mingwBin"
}

$env:PATH = "$mingwBin;$env:PATH"
Write-Host "MinGW bin: $mingwBin" -ForegroundColor Green

# --- 5. Znajdz katalog Qt zawierajacy Qt6Config.cmake ---
$qtConfigDir = Get-ChildItem -Path $qtRoot -Directory -Filter "mingw_64" -Recurse -ErrorAction SilentlyContinue |
    Where-Object { Test-Path (Join-Path $_.FullName "lib\cmake\Qt6\Qt6Config.cmake") } |
    Select-Object -First 1

if (-not $qtConfigDir) {
    Stop-WithPause "Nie znaleziono Qt6Config.cmake pod $qtRoot. Sprawdz czy zainstalowany jest komponent Qt 6.11.0 MinGW 64-bit."
}

$qtPrefixPath = $qtConfigDir.FullName
Write-Host "Qt prefix path: $qtPrefixPath" -ForegroundColor Green

# --- 6. Konfiguracja i budowa projektu ---
Write-Host "=== Budowanie projektu (CMake + MinGW) ===" -ForegroundColor Cyan

$buildDir = Join-Path $ProjectDir "build\Desktop_Qt_6_11_0_MinGW_64_bit-Release"
if (-not (Test-Path $buildDir)) {
    New-Item -ItemType Directory -Path $buildDir | Out-Null
}

cmake -S $ProjectDir -B $buildDir `
    -G "MinGW Makefiles" `
    -DCMAKE_BUILD_TYPE=Release `
    -DCMAKE_MAKE_PROGRAM="$makeExe" `
    -DCMAKE_C_COMPILER="$gccExe" `
    -DCMAKE_CXX_COMPILER="$gppExe" `
    -DCMAKE_PREFIX_PATH="$qtPrefixPath"

if ($LASTEXITCODE -ne 0) { Stop-WithPause "Konfiguracja CMake nie powiodla sie." }

cmake --build $buildDir --target all -j
if ($LASTEXITCODE -ne 0) { Stop-WithPause "Budowanie nie powiodlo sie." }

# --- 7. Znajdz .exe ---
$exePath = Get-ChildItem -Path $buildDir -Filter "gimp2.exe" -Recurse -ErrorAction SilentlyContinue | Select-Object -First 1

if (-not $exePath) {
    Stop-WithPause "Nie znaleziono gimp2.exe po kompilacji."
}

# --- 8. Deploy zaleznosci Qt (DLL-e) ---
Write-Host "=== Kopiowanie bibliotek Qt (windeployqt) ===" -ForegroundColor Cyan

$windeployqtExe = Join-Path $qtPrefixPath "bin\windeployqt.exe"

if (Test-Path $windeployqtExe) {
    & $windeployqtExe $exePath.FullName
    if ($LASTEXITCODE -ne 0) {
        Write-Host "windeployqt zwrocil ostrzezenie, ale kontynuuje." -ForegroundColor Yellow
    }
} else {
    Stop-WithPause "Nie znaleziono windeployqt.exe w $qtPrefixPath\bin"
}

Copy-Item "$mingwBin\libgcc_s_seh-1.dll" -Destination $exePath.DirectoryName -Force -ErrorAction SilentlyContinue
Copy-Item "$mingwBin\libstdc++-6.dll" -Destination $exePath.DirectoryName -Force -ErrorAction SilentlyContinue
Copy-Item "$mingwBin\libwinpthread-1.dll" -Destination $exePath.DirectoryName -Force -ErrorAction SilentlyContinue

# --- 9. Utworz skrot ---
Write-Host "=== Tworzenie skrotu ===" -ForegroundColor Cyan
$shortcutPath = Join-Path $ScriptDir "gimp2.lnk"
$WScriptShell = New-Object -ComObject WScript.Shell
$Shortcut = $WScriptShell.CreateShortcut($shortcutPath)
$Shortcut.TargetPath = $exePath.FullName
$Shortcut.WorkingDirectory = $exePath.DirectoryName
$Shortcut.IconLocation = $exePath.FullName
$Shortcut.Save()

Write-Host "Skrot utworzony: $shortcutPath" -ForegroundColor Green

Read-Host "`nGotowe. Nacisnij Enter dwukrotnie, aby zamknac okno"