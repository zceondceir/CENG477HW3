#!/bin/bash

# Eğer argüman girilmemişse uyarı ver ve çık
if [ -z "$1" ]
then
  echo "Hata: Bir commit mesajı girmelisiniz!"
  echo "Kullanım: ./git.sh 'mesajınız'"
  exit 1
fi

# Git komutlarını sırasıyla çalıştır
git add .
git commit -m "$1"
git push https://github.com/Osaka-San-TFT/CENG477HW3 master