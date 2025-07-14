<h2> Campus Monitoring System Capstone Project </h2>

Repository github ini adalah repository lampiran kode dari Tugas Akhir Calvin Institute of Technology dengan judul "PERANCANGAN SISTEM MONITORING KAMPUS TERPUSAT BERBASIS IOT" yang dikerjakan oleh Martin Emmanuel Chang (212100199) dan Wesley Hakim (212100211) dengan dosen pembimbing pertama Erwin Anggadjaja, Ph.D dan dosen pembimbing kedua Aditya Heru Prathama, Ph.D.

Berikut panduan  program dan sistem secara ringkas:
- Folder Dashboard app berisikan program untuk menjalankan aplikasi web dashboard django
- Folder Kode pengujian adalah kode python yang digunakan untuk melakukan pengujian pada Bab V atau F500
- Folder Kode station adalah file .ino yang digunakan untuk setiap station


Untuk menjalankan dashboard:
1. Jalankan environment python dengan library yang tertulis pada requirements.txt
2. Jalankan program python manage.py mqtt_listener pada folder CampusMonitoring dalam folder Dashboard app. Program ini untuk menjalankan klien MQTT yang menerima data


Untuk menjalankan station:
1. Upload kode station kepada mikrokontroler
2. Pasang catu daya station setelah mqtt_listener telah dijalankan


Untuk melihat dokumen prosedur penggunaan sistem dapat dilihat selengkapnya pada Dokumen Tugas Akhir PERANCANGAN SISTEM MONITORING KAMPUS TERPUSAT BERBASIS IOT yang tersedia pada kampus Calvin Institute of Technology
