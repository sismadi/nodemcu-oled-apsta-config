# NodeMCU-OLED-APSTA-Config

## Sistem IoT NodeMCU ESP8266 dengan Konfigurasi WiFi & API melalui Access Point (AP)

Proyek ini mengimplementasikan sistem Internet of Things ($\text{IoT}$) menggunakan mikrokontroler **$\text{NodeMCU ESP8266}$** yang dilengkapi dengan layar **$\text{OLED SSD1306}$**. Fitur utamanya adalah sistem konfigurasi yang fleksibel: kredensial $\text{WiFi}$ dan $\text{URL API}$ disimpan di $\text{EEPROM}$ dan dapat diatur ulang kapan saja melalui mode **Access Point (AP) *Setup***.

### Fitur Utama 🚀

  * **Mode Ganda ($\text{AP} + \text{STA}$):** Beroperasi sebagai klien ($\text{STA}$) untuk terhubung ke $\text{WiFi}$ dan sebagai *Access Point* ($\text{AP}$) untuk mode konfigurasi.
  * **Persistent Configuration:** Menyimpan $\text{SSID}$, *Password*, $\text{URL API GET}$, dan $\text{URL API POST}$ ke memori $\text{EEPROM}$.
  * **$\text{OLED}$ Display:** Menampilkan status koneksi, mode operasi, $\text{IP}$ lokal, dan data yang diambil dari $\text{API}$.
  * **Komunikasi $\text{HTTP}/\text{HTTPS}$:** Mendukung permintaan $\text{GET}$ dan $\text{POST}$ ke server eksternal, termasuk komunikasi aman ($\text{HTTPS}$) menggunakan `WiFiClientSecure`.
  * **$\text{JSON}$ Parsing:** Mampu mengambil dan memproses data $\text{JSON}$ dari $\text{API}$ menggunakan `ArduinoJson`.

-----

## Persyaratan Perangkat Keras (Hardware Requirements)

| Komponen | Deskripsi |
| :--- | :--- |
| **Microcontroller** | $\text{NodeMCU ESP8266}$ (versi $\text{ESP-12E}$ atau yang kompatibel) |
| **Display** | $\text{OLED}$ Display $\text{SSD1306}$ ($\text{128x64}$ piksel) |
| **Koneksi I2C** | $\text{SDA}$ ke $\text{D3}$ ($\text{GPIO0}$), $\text{SCL}$ ke $\text{D4}$ ($\text{GPIO2}$) *(Menggunakan pin yang dimodifikasi)* |
| **Power** | Kabel $\text{Micro-USB}$ atau sumber daya $\text{Vin}$ ($7-12\text{V}$) |

-----

## Diagram Rangkaian Sederhana 🔌

Rangkaian ini menggunakan protokol $\text{I2C}$ untuk komunikasi dengan $\text{OLED}$.

| Pin $\text{NodeMCU}$ | Pin $\text{OLED}$ $\text{SSD1306}$ | Keterangan |
| :---: | :---: | :--- |
| **$\text{D3}$** ($\text{GPIO0}$) | $\text{SDA}$ (Data) | Pin $\text{I2C}$ Data **(Perubahan dari default)** |
| **$\text{D4}$** ($\text{GPIO2}$) | $\text{SCL}$ (Clock) | Pin $\text{I2C}$ Clock **(Perubahan dari default)** |
| $\text{3.3V}$ | $\text{VCC}$ / $\text{VDD}$ | Catu daya $\text{OLED}$ |
| $\text{GND}$ | $\text{GND}$ | Ground |

-----

## Pustaka (Libraries) yang Dibutuhkan

Pastikan Anda telah menginstal pustaka berikut di $\text{Arduino IDE}$:

1.  **$\text{ESP8266}$ Boards Core** (melalui *Boards Manager*)
2.  **Adafruit $\text{GFX}$ Library**
3.  **Adafruit $\text{SSD1306}$ Library**
4.  **$\text{ArduinoJson}$** (Disarankan versi $\text{6}$ atau lebih tinggi)

Anda juga membutuhkan pustaka bawaan $\text{ESP8266}$: `ESP8266WiFi`, `ESP8266WebServer`, `ESP8266HTTPClient`, `WiFiClientSecure`, `Wire`, dan `EEPROM`.

-----

## Panduan Penggunaan dan Setup 🛠️

### 1\. Mode Konfigurasi (AP Setup)

Ketika $\text{NodeMCU}$ pertama kali dinyalakan atau gagal terhubung ke $\text{WiFi}$ yang tersimpan, ia akan secara otomatis masuk ke **Mode $\text{AP}$ *Setup***.

  * **$\text{SSID}$ AP:** `SetupNodeMCU`
  * **$\text{Password}$ AP:** `12345678`
  * **$\text{IP}$ Setup:** `192.168.4.1`

**Langkah Setup:**

1.  Sambungkan ponsel atau $\text{PC}$ Anda ke jaringan $\text{WiFi}$ **`SetupNodeMCU`**.
2.  Buka *browser* dan navigasikan ke $\mathbf{[http://192.168.4.1/](http://192.168.4.1/)}$.
3.  Masukkan $\text{SSID}$ dan *Password* $\text{WiFi}$ utama Anda, serta $\text{URL API GET}$ dan $\text{API POST}$ (opsional).
4.  Klik **Save**. $\text{NodeMCU}$ akan menyimpan konfigurasi dan me-restart.

### 2\. Mode Normal (STA Mode)

Setelah konfigurasi disimpan, $\text{NodeMCU}$ akan mencoba terhubung ke jaringan $\text{WiFi}$ yang telah ditentukan.

  * Jika koneksi berhasil, ia akan menampilkan $\text{IP}$ lokal dan mulai menjalankan logika utama ($\text{HTTP GET}$ dan $\text{POST}$) secara berkala.
  * Logika utama mengambil $\text{JSON}$ dari $\text{URL API GET}$ setiap **10 detik** dan mencoba mem-parsing data tersebut untuk menampilkan nilai kunci *name*/*nama*/*title* pada $\text{OLED}$.
  * Logika $\text{POST}$ (jika $\text{URL API POST}$ diisi) akan mengirim data status perangkat ($\text{RSSI}$, *uptime*) setiap **30 detik**.

-----

## Logika Pin $\text{Wire}$ (I2C)

Perhatikan bahwa inisialisasi $\text{I2C}$ (Wire) pada fungsi `setup()` telah dimodifikasi sesuai permintaan untuk menggunakan $\text{D3}$ sebagai $\text{SDA}$ dan $\text{D4}$ sebagai $\text{SCL}$.

```cpp
// Sebelum: Wire.begin(D2, D1);
// Setelah:
Wire.begin(D3, D4); // SDA (D3/GPIO0), SCL (D4/GPIO2)
```

**Peringatan:** Pin $\text{D3}$ ($\text{GPIO0}$) dan $\text{D4}$ ($\text{GPIO2}$) adalah pin *Boot* sensitif pada $\text{ESP8266}$. Memprogram pin ini sebagai *output* $\text{I2C}$ umumnya aman setelah *boot*, tetapi pastikan tidak ada komponen eksternal yang menarik pin-pin ini ke $\text{LOW}$ saat proses *power up*, karena dapat mencegah $\text{NodeMCU}$ melakukan *boot* normal.
