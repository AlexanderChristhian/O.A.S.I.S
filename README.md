# O.A.S.I.S.

**Optimized Autonomous System for Impure Sources** — proyek capstone Kelompok 11, Departemen Teknik Elektro, Fakultas Teknik, Universitas Indonesia.

O.A.S.I.S. dirancang sebagai sistem purifikasi air portabel untuk kebutuhan MCK dan air minum. Repository ini berisi milestone **simulasi kontrol ESP32**, menggunakan PlatformIO (Arduino framework) dan rangkaian Wokwi.

## Anggota kelompok

| Nama | NPM |
| --- | --- |
| Alexander Christhian | 2306267025 |
| Fathan Yazid Satriani | 2306250560 |
| Innezahra Aurellia Titani | 2306168965 |
| Kevin Imanuel Hutagaol | 2306156763 |
| Natano Juditya Sihan | 2306206742 |

Nama dan NPM mengacu pada proposal Desain Proyek 1 Kelompok 11. Ketua kelompok: Innezahra Aurellia Titani.

## Fitur milestone saat ini

- Dua potensiometer sebagai input simulasi pH dan kekeruhan; TDS belum digunakan.
- Pilihan **MCK / OFF / DRINK** yang tersimpan setelah tombol dilepas. Mode awal OFF; tidak ada mode AUTO.
- Pemeriksaan kualitas air sebagai interlock sebelum mengaktifkan jalur.
- LCD I2C 20×4 untuk mode, pH, NTU, dan kategori air.
- Empat relay untuk pompa utama, katup MCK, pompa RO, dan katup minum.
- Pembacaan sensor setiap 250 ms dan ketika mode berubah; log serial setiap 1 detik pada 115200 baud.

## Menjalankan simulasi

Siapkan VS Code dengan ekstensi **PlatformIO IDE** dan **Wokwi Simulator**. Aktifkan lisensi Wokwi untuk VS Code yang sesuai. Panduan: [PlatformIO di VS Code](https://docs.platformio.org/en/latest/integration/ide/vscode.html) dan [Wokwi untuk VS Code](https://docs.wokwi.com/vscode/getting-started).

```sh
git clone https://github.com/AlexanderChristhian/O.A.S.I.S.git
cd O.A.S.I.S/O.A.S.I.S
pio run -e esp32dev
code .
```

**Buka folder dalam `O.A.S.I.S/` yang berisi `platformio.ini`**, bukan hanya root repository. Jika `pio` tidak tersedia di terminal biasa, gunakan terminal PlatformIO atau tombol Build di VS Code.

Setelah build berhasil, buka `diagram.json` lalu klik Play, atau jalankan **Wokwi: Start Simulator** dari Command Palette. File `wokwi.toml` sudah menunjuk ke `.pio/build/esp32dev/firmware.bin` dan `firmware.elf`. Simulasi tidak membutuhkan board USB.

## Pengoperasian

| Tombol / shortcut | Hasil |
| --- | --- |
| Biru / `1` — MCK | Memilih jalur MCK jika kualitas memenuhi kategori MCK atau DRINKABLE |
| Kuning / `2` — OFF | Mematikan seluruh keluaran pompa dan katup; LCD dan sensor tetap berjalan |
| Hijau / `3` — DRINK | Memilih jalur minum hanya jika kategori DRINKABLE |

Klik sekali lalu lepaskan. Mode tetap aktif sampai dipilih mode lain; tidak perlu menahan tombol atau menekan Start lagi. Klik area simulator jika shortcut belum merespons. Ketika mode tetap dipilih, output mengikuti perubahan kualitas sensor: pembacaan tidak memenuhi akan mematikan jalur, dan pembacaan yang kembali memenuhi akan mengaktifkannya lagi.

OFF mendapat prioritas; penekanan MCK dan DRINK bersamaan memilih OFF. Tombol simulasi menggunakan `bounce: 0`. Selector tiga posisi fisik nantinya memerlukan pembacaan kontak dan debounce yang sesuai wiring perangkat.

## Kriteria simulasi dan keluaran

| Kategori LCD | Kriteria |
| --- | --- |
| DRINKABLE | pH 6,5–8,5 dan NTU ≤ 5 |
| MCK ONLY | pH 6,5–8,5 dan 5 < NTU < 10 |
| UNSAFE | pH di luar rentang tersebut atau NTU ≥ 10 |

Target kekeruhan yang diinginkan kelompok adalah **< 0,3 NTU**; batas penerimaan simulasi tetap **≤ 5 NTU**. Kriteria ini adalah konfigurasi milestone kelompok, bukan sertifikasi kelayakan air minum.

| Mode | DRINKABLE | MCK ONLY | UNSAFE |
| --- | --- | --- | --- |
| OFF | Semua mati | Semua mati | Semua mati |
| MCK | Pompa utama + katup MCK | Pompa utama + katup MCK | Semua mati |
| DRINK | Pompa utama + pompa RO + katup minum | Semua mati | Semua mati |

## Urutan demo singkat

1. Mulai simulasi: LCD menunjukkan OFF, seluruh indikator beban mati.
2. Atur pH sekitar 7 dan NTU sekitar 7. Klik MCK: pompa utama dan katup MCK aktif.
3. Klik DRINK dengan pembacaan yang sama: semua beban mati karena belum memenuhi kriteria minum.
4. Turunkan NTU menjadi sekitar 2, dengan DRINK tetap dipilih: jalur minum aktif tanpa menekan tombol lagi.
5. Klik MCK: jalur beralih ke MCK. Klik OFF: seluruh beban mati.
6. Atur pH sekitar 4 atau NTU di atas 10, lalu pilih MCK/DRINK: keluaran tetap mati.

Gunakan angka pada LCD untuk mengatur knob. Empat LED beban di kanan mewakili pompa/katup; jangan menyamakannya dengan LED kecil pada modul relay.

## Batas simulasi

Simulasi menjalankan logika kontrol dan rangkaian I/O; **belum memodelkan penyaringan atau proses RO**. Menyalakan pompa tidak menurunkan NTU atau mengubah pH. Pembacaan hanya berubah melalui potensiometer. Belum ada model sensor masuk/keluar terpisah, waktu treatment, pengujian pascafiltrasi, atau deteksi kegagalan filter.

Mode DRINK saat ini baru mengaktifkan pompa RO jika pembacaan sudah memenuhi kategori DRINKABLE. Pompa utama, pompa RO, dan katup minum diaktifkan bersama; belum ada urutan treatment. Dashboard IoT, pengelolaan baterai, dan kalibrasi sensor fisik belum diimplementasikan pada milestone ini.

## Struktur repository

```text
README.md                 Ringkasan proyek, anggota, dan quick start
O.A.S.I.S/
  platformio.ini          Konfigurasi ESP32 dan library
  wokwi.toml              Lokasi firmware untuk Wokwi
  diagram.json            Komponen serta koneksi simulasi
  src/main.cpp            Firmware kontrol
  README.md               Detail pin, alur program, dan relay
  include/ lib/ test/     Folder standar PlatformIO
```

Untuk pemetaan GPIO dan diagram alur, lihat [dokumentasi teknis](O.A.S.I.S/README.md).

## Pemecahan masalah

- **Firmware tidak ditemukan:** build dari folder yang berisi `platformio.ini`, lalu mulai ulang Wokwi.
- **License expired:** aktifkan kembali lisensi melalui alur resmi Wokwi untuk VS Code.
- **Label diagram masih lama:** tutup tab simulator dan tab diagram, lalu buka kembali `diagram.json` setelah menyimpan perubahan.
- **Mode aktif tetapi beban mati:** cek pH dan NTU terhadap tabel kriteria; mode terpilih tidak menjamin keluaran diizinkan.
- **Upload tidak menemukan port:** simulasi cukup menggunakan Play Wokwi. Upload ke board fisik membutuhkan perangkat USB dan verifikasi wiring tersendiri.
