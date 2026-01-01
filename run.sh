#!/bin/bash
set -euo pipefail

echo "[1/4] Temizlik yapılıyor (build klasörü siliniyor)..."
if [ -d build ]; then
  rm -rf build
fi

echo
echo "[2/4] CMake ayarları yapılıyor..."
# Windows'ta MinGW için -G "MinGW Makefiles" vardı, Linux'ta gerek yok
if ! cmake -B build .; then
  echo
  echo "!!! BIR HATA OLUSTU !!!"
  echo "Derleme veya hazırlık aşamasında sorun var."
  exit 1
fi

echo
echo "[3/4] Derleme başlatılıyor (make)..."
if ! cmake --build build; then
  echo
  echo "!!! BIR HATA OLUSTU !!!"
  echo "Derleme veya hazırlık aşamasında sorun var."
  exit 1
fi

echo
echo "[4/4] Program başlatılıyor..."
echo "---------------------------------------------------"
cd working_dir

# Eğer projeyi Linux için derliyorsan büyük ihtimalle çıktı:
#   PlanetRenderer   (uzantısız)
# Eğer hâlâ .exe derliyorsan (cross-compile vs.), onu da altta yorumdan çıkarabilirsin.

if [ -x ./PlanetRenderer ]; then
  ./PlanetRenderer
elif [ -x ./PlanetRenderer.exe ]; then
  ./PlanetRenderer.exe   # (veya wine ./PlanetRenderer.exe)
else
  echo "Çalıştırılabilir dosya bulunamadı: PlanetRenderer veya PlanetRenderer.exe"
  exit 1
fi

cd ..
echo "---------------------------------------------------"
echo "İşlem tamamlandı."
