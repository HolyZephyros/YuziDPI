# YuziDPI

**YuziDPI**, Windows için geliştirilmiş, çekirdek sürücüsü gerektirmeyen, tek dosya (`.exe`), ultra hafif (**~1.2 MB RAM**) **Akıllı Yerel Tünel & Ağ Paket Optimizasyon** aracıdır.

> **[⬇ Son Sürümü İndir (YuziDPI.exe — v1.0.1)](https://github.com/HolyZephyros/YuziDPI/releases/latest/download/YuziDPI.exe)**

---

## ✨ Özellikler

- **Sürücüsüz & Sıfır Gecikme:** Çekirdek (kernel) sürücüsü kurmaz; sistem düzeyinde ağ filtresi eklemediği için diğer masaüstü uygulamalarıyla çakışmaz ve **0 ms ek gecikme** sağlar.
- **3 Çalışma Kapsamı:**
  1. **Sadece Liste:** Yalnızca eklediğiniz siteleri (`discord.com` vb.), uygulamaları (`.exe`) ve klasörleri tüneller.
  2. **Hariç Tutulanlar:** Belirttiğiniz liste dışındaki tüm bağlantıları yönlendirir.
  3. **Tüm Sistem:** Bilgisayardaki tüm trafiği seçili kademe üzerinden geçirir.
- **6 Canlı Kademe (`1..6`):** Kapatıp açmaya gerek kalmadan tek tıkla kademeler arasında anında canlı geçiş yapar.
- **Tam Discord Uyumluluğu:** Discord'un güncelleyicisi (`Update.exe` / `updater.node`), ses sunucuları ve medya CDN'leri ile kutudan çıktığı gibi çalışır; Windows açılışındaki güncelleme takılmalarını otomatik kurtarır.
- **Sessiz OTA Otomatik Güncelleme:** GitHub Releases üzerinden yeni sürüm yayınlandığında yalnızca ilk açılışta kontrol ederek kendini günceller.
- **19 Dil Desteği:** Türkçe, İngilizce, Almanca, Fransızca, İspanyolca, Portekizce, İtalyanca, Rusça, Japonca, Korece, Çince, Arapça, Felemenkçe, Lehçe, Hintçe, Endonezce, Tayca, Vietnamca ve Ukraynaca.
- **Catppuccin Mocha Arayüz:** Göz yormayan mat Catppuccin Mocha karanlık tema.

---

## 🚀 Kurulum & Kullanım

1. [**Releases**](https://github.com/HolyZephyros/YuziDPI/releases/latest) sayfasından `YuziDPI.exe` dosyasını indirin.
2. Herhangi bir kurulum gerektirmez; doğrudan çalıştırın ve **Başlat** düğmesine basın.
3. İsterseniz alt bardan **Oto Bağlan**, **Windows ile Aç** ve **Gizli Başlat** seçeneklerini aktif edebilirsiniz.

---

## 🛠️ Derleme (Kaynaktan)

MinGW-w64 (`g++` ve `windres`) ile tek komutla derlenir:

```bat
build.bat
```
