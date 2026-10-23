# YuziDPI

**YuziDPI**, Windows için geliştirilmiş, sürücü (WinDivert vb. çekirdek sürücüsü) gerektirmeyen, tek dosya (`.exe`), ultra hafif (**~1.2 MB RAM**) **Akıllı DPI Bypass & Uygulama/Alan Adı Tüneli** aracıdır.

> **[⬇ Son Sürümü İndir (YuziDPI.exe — v1.0.1)](https://github.com/HolyZephyros/YuziDPI/releases/latest/download/YuziDPI.exe)**

---

## ✨ Özellikler

- **Sürücüsüz & Anti-Cheat Dostu:** Çekirdek (kernel) sürücüsü kurmaz; çevrimiçi oyunların Anti-Cheat sistemleriyle (Vanguard, Ricochet, EAC, BattlEye) çakışmaz ve oyunlarda **0 ms ek gecikme** sağlar.
- **3 Çalışma Kapsamı:**
  1. **Sadece Liste:** Yalnızca eklediğiniz siteleri (`discord.com` vb.), uygulamaları (`.exe`) ve klasörleri tüneller.
  2. **Hariç Tutulanlar:** Belirttiğiniz liste dışındaki tüm bağlantıları açar.
  3. **Tüm Sistem:** Bilgisayardaki tüm trafiği seçili DPI kademesi üzerinden geçirir.
- **6 Canlı DPI Kademesi (`1..6`):** Kapatıp açmaya gerek kalmadan tek tıkla kademeler arasında anında canlı geçiş yapar.
- **Tam Discord Uyumluluğu:** Discord'un Rust tabanlı güncelleyicisi (`Update.exe` / `updater.node`), ses sunucuları ve medya CDN'leri ile kutudan çıktığı gibi çalışır; Windows açılışındaki güncelleme takılmalarını otomatik kurtarır.
- **Sessiz OTA Otomatik Güncelleme:** GitHub Releases üzerinden yeni sürüm yayınlandığında arka planda sormadan kendini günceller.
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

