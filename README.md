# Image Editor - gimp2

Prosty edytor obrazu napisany w C++ z wykorzystaniem Qt6 i CMake.

## Wymagania
- Qt 6.11.0 (komponent MinGW 13.1.0 64-bit)
- CMake (w wersji dostarczonej z Qt lub systemowej)
- System Windows 64-bit

## Instalacja i pierwsze uruchomienie
1. Sklonuj repozytorium do folderu, np. `D:\Projekty\gimp2`.
2. Uruchom skrypt `preinstall-gimp2.ps1` jako administrator (prawy klik &gt; Uruchom z PowerShell, lub w terminalu:)
   `powershell -ExecutionPolicy Bypass -File preinstall-gimp2.ps1`
3. Skrypt automatycznie:
   - sprawdzi obecnosc CMake i Qt, zainstaluje brakujace elementy przez winget
   - znajdzie CMakeLists.txt i skompiluje projekt w trybie Release
   - skopiuje wymagane biblioteki Qt i MinGW (windeployqt)
   - utworzy skrot `gimp2.lnk` w katalogu skryptu
4. Po zakonczeniu uruchom aplikacje klikajac `gimp2.lnk`.

## Ponowne budowanie
Aby przebudowac projekt po zmianach w kodzie, usun folder `build` i uruchom skrypt ponownie.

## Struktura projektu
- `CMakeLists.txt` - konfiguracja budowania
- `main.cpp`, `mainwindow.cpp/h/ui` - glowne okno aplikacji
- `*handler.cpp/h` - logika operacji na obrazie (punktowe, histogram, konwolucja, binaryzacja, Canny, Harris)
- `*page.cpp/h/ui` - widoki interfejsu dla poszczegolnych operacji
- `preinstall-gimp2.ps1` - skrypt instalacyjno-budujacy

## Technologie
- C++17, Qt6 (Widgets, Gui, Core)
- CMake + MinGW 13.1.0 64-bit
