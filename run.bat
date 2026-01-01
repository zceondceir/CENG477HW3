@echo off
echo [1/4] Temizlik yapiliyor (Build klasoru siliniyor)...
if exist build rmdir /s /q build

echo.
echo [2/4] CMake ayarlari yapiliyor...
cmake -B build -G "MinGW Makefiles" .
if %errorlevel% neq 0 goto :error

echo.
echo [3/4] Derleme baslatiliyor (Make)...
cmake --build build
if %errorlevel% neq 0 goto :error

echo.
echo [4/4] Program baslatiliyor...
echo ---------------------------------------------------
cd working_dir
PlanetRenderer.exe
cd ..
echo ---------------------------------------------------
echo Islem tamamlandi.
goto :eof

:error
echo.
echo !!! BIR HATA OLUSTU !!!
echo Derleme veya hazirlik asamasinda sorun var.
pause