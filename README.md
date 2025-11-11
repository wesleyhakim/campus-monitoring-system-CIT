# Campus Monitoring System Capstone Project

## Repository Outline
```
    campus-monitoring-system-CIT
    |
    ├── Dashboard app/CampusMonitoring/
    │   ├── CampusMonitor/
    │   ├── CampusMonitoring/
    │   ├── ...
    │   ├── delete_entries.py
    │   └── manage.py
    ├── Dokumentasi/
    │   ├── Final TA Presentation_compressed.pdf
    │   ├── Katalog TA_compressed.pdf
    │   ├── LINK VIDEO PROMOSI.pdf
    │   ├── Makalah TA (1).pdf
    │   └── TA Poster (420 x 594 mm) (1)_compressed.pdf
    ├── Kode pengujian/
    │   ├── station_test1-2.py
    │   └── testing code1-1.py
    ├── Kode station/
    │   ├── indoor_station_final/
    │   │   └── indoor_station_final.ino
    │   ├── outdoor_station_final/
    │   │   └── outdoor_station_final.ino
    │   ├── power_station_final/
    │   │   └── power_station_final.ino
    │   ├── water_station_final/
    │   │   └── water_station_final.ino
    ├── README.md
    └── requirements.txt
```
- **campus-monitoring-system-CIT**  
    Repository github ini.

- **Dashboard app/CampusMonitoring**  
    Folder yang berisi mengenai dashboard framework django system monitoring.

- **Dokumentasi/**  
    Folder berisi file dokumentasi seperti file presentasi, katalog, link video, makalah, dan poster.

- **Kode pengujian/**  
    Folder berisi kode python untuk menguji uptime serta menerima data dari MQTT subscribe pada Bab V atau F500. 

- **Kode station/**  
    Folder berisi kode `.ino` untuk setiap station.

- **README.md**  
    File dokumentasi yang berisi mengenai overview dan penjelasan repository ini.

- **requirements.txt**  
    File txt yang berisi mengenai library python yang perlu diinstal untuk aplikasi dapat berjalan.

## Deskripsi proyek
Repository github ini adalah repository lampiran kode dari Tugas Akhir Calvin Institute of Technology dengan judul `"PERANCANGAN SISTEM MONITORING KAMPUS TERPUSAT BERBASIS IOT"` yang dikerjakan oleh `Martin Emmanuel Chang (212100199)` dan `Wesley Hakim (212100211)` dengan dosen pembimbing pertama Erwin Anggadjaja, Ph.D dan dosen pembimbing kedua Aditya Heru Prathama, Ph.D.

Proyek ini berupaya membuat sebuah sistem monitoring kampus terpusas berbasis IoT dengan station sebagai node yang bertindak untuk mengambil data lingkungan menggunakan berabgai sensor, kemudian mengirim data bacaan sensor tersebut kepada server Raspberry Pi untuk disimpan datanya dan ditampilkan pada web dashboard. Pengiriman data dilakukan menggunakan MQTT dan web dashboard menggunakan web framework Django yang dijalankan pada Raspberry Pi juga.

## Prosedur
  **Untuk menjalankan dashboard:**  
    1. Jalankan environment python dengan library yang tertulis pada requirements.txt  
    2. Jalankan program python manage.py mqtt_listener pada folder CampusMonitoring dalam folder Dashboard app. Program ini untuk menjalankan klien MQTT yang menerima data  

  **Untuk menjalankan station:**  
    1. Upload kode station kepada mikrokontroler  
    2. Pasang catu daya station setelah mqtt_listener telah dijalankan  

## Stacks
Project ini menggunakan bahasa pemrograman Python, library Django, dan paho-mqtt untuk membuat web dashboard dan menerima data. Juga digunakan bahasa pemrograman berbasis C++ dengan Arduino IDE untuk membaca serta mengirim data dari mikrokontroler station.

## Kontributor

| Nama       | LinkedIn | GitHub |
|------------|----------|--------|
| Martin Emmanuel Chang | [🔗 Connect](https://www.linkedin.com/in/martin-emmanuel-chang-666b1529a/) | [✅ Follow](https://github.com/mmmmartin24) |
| Wesley Hakim | [🔗 Connect](https://www.linkedin.com/in/wesley-hakim/) | [✅ Follow](https://github.com/wesleyhakim) |

## Reference
Untuk melihat dokumen prosedur penggunaan sistem dapat dilihat selengkapnya pada Dokumen Tugas Akhir PERANCANGAN SISTEM MONITORING KAMPUS TERPUSAT BERBASIS IOT yang tersedia pada kampus Calvin Institute of Technology.  
[🏛️ Situs kampus](https://calvin.ac.id/)  
[📍 Lokasi](https://maps.app.goo.gl/DjaNVvmcPE8rgyDR9)  
