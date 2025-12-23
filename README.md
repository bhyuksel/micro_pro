# 📚 RGB LED Kontrol Sistemi - Kapsamlı Teknik Döküman

Bu döküman, RGB LED kontrol projesindeki **her satır kodun nasıl çalıştığını**, **neden öyle yazıldığını** ve **C++ temellerini** detaylıca açıklar.

---

## 📖 İçindekiler

1. [C++ Temelleri](#1-c-temelleri)
2. [Veri Tipleri ve Bellek Yönetimi](#2-veri-tipleri-ve-bellek-yönetimi)
3. [PROGMEM ve Flash Bellek](#3-progmem-ve-flash-bellek)
4. [Serial Haberleşme](#4-serial-haberleşme)
5. [PWM ve LED Kontrolü](#5-pwm-ve-led-kontrolü)
6. [Fonksiyon Açıklamaları](#6-fonksiyon-açıklamaları)
7. [Web Sunucusu](#7-web-sunucusu)

---

## 1. C++ Temelleri

### 1.1 Değişken Tanımlama

```cpp
int animationMode = 0;           // Tam sayı (integer)
float hue = 0;                   // Ondalıklı sayı (floating point)
String currentColorName = "Text"; // Metin (string)
bool isActive = true;            // Doğru/Yanlış (boolean)
```

| Tür      | Açıklama                    | Örnek                  |
| -------- | --------------------------- | ---------------------- |
| `int`    | Tam sayı (-32768 ile 32767) | `int count = 5;`       |
| `float`  | Ondalıklı sayı              | `float pi = 3.14;`     |
| `String` | Metin                       | `String name = "LED";` |
| `bool`   | Mantıksal (true/false)      | `bool isOn = false;`   |

### 1.2 Koşullar (if-else)

```cpp
if (brightness > 200) {
  // Parlaklık 200'den büyükse bu çalışır
  Serial.println("Çok parlak!");
} else if (brightness > 100) {
  // Parlaklık 100-200 arasındaysa
  Serial.println("Orta parlak");
} else {
  // Diğer durumlarda
  Serial.println("Düşük parlak");
}
```

### 1.3 Karşılaştırma Operatörleri

```cpp
if (a == b)  // a, b'ye EŞİT mi?
if (a != b)  // a, b'ye EŞİT DEĞİL mi?
if (a > b)   // a, b'den BÜYÜK mü?
if (a < b)   // a, b'den KÜÇÜK mü?
if (a >= b)  // a, b'den BÜYÜK VEYA EŞİT mi?
if (a <= b)  // a, b'den KÜÇÜK VEYA EŞİT mi?
```

**Örnek:**

```cpp
if (animationMode == 2) {
  // Rainbow modu aktif
}

if (brightness != 0) {
  // Parlaklık sıfır değilse LED açık
}
```

### 1.4 Mantıksal Operatörler

```cpp
if (brightness > 0 && animationMode == 0)
  // VE (&&): Her ikisi de doğruysa

if (brightness == 0 || animationMode == 4)
  // VEYA (||): Biri doğruysa

if (!isActive)
  // DEĞİL (!): Tersini al
```

### 1.5 Döngüler (Loops)

#### for döngüsü

```cpp
for (int i = 0; i < 10; i++) {
  // i, 0'dan 9'a kadar (10 kez çalışır)
  Serial.println(i);
}
```

**Açıklama:**

- `int i = 0;` → Başlangıç değeri
- `i < 10;` → Devam koşulu
- `i++` → Her adımda i'yi 1 artır

#### while döngüsü

```cpp
while (brightness < 255) {
  brightness += 10;  // Her seferinde 10 artır
  delay(100);
}
```

### 1.6 switch-case

```cpp
switch (animationMode) {
  case 0:
    // animationMode 0 ise
    turnOffLED();
    break;  // Çık

  case 2:
    // animationMode 2 ise
    rainbowAnimation();
    break;

  default:
    // Hiçbiri değilse
    break;
}
```

### 1.7 Fonksiyonlar

```cpp
// Fonksiyon tanımlama
void setColor(int r, int g, int b) {
  // void: Geriye değer döndürmez
  // r, g, b: Parametreler (girdi)
  writeRGB(r, g, b);
}

// Geriye değer döndüren fonksiyon
int calculate(int a, int b) {
  return a + b;  // Sonucu döndür
}

// Kullanım
setColor(255, 0, 0);       // Kırmızı
int sum = calculate(5, 3); // sum = 8
```

---

## 2. Veri Tipleri ve Bellek Yönetimi

### 2.1 Neden uint8_t, uint16_t Kullanıyoruz?

ESP32'de bellek **çok önemli**! Her byte değerli.

| Tip        | Boyut      | Aralık                     | Kullanım                   |
| ---------- | ---------- | -------------------------- | -------------------------- |
| `uint8_t`  | **1 byte** | 0-255                      | RGB değerleri, PWM (0-255) |
| `uint16_t` | **2 byte** | 0-65535                    | Ara hesaplamalar           |
| `int`      | **4 byte** | -2147483648 ile 2147483647 | Büyük sayılar              |
| `float`    | **4 byte** | Ondalıklı                  | HSV hesaplamaları          |

### 2.2 Pratik Örnekler

#### ❌ Kötü Kullanım (Bellek israfı)

```cpp
int red = 255;      // 4 byte kullanır ama sadece 0-255 lazım
int green = 128;    // 4 byte
int blue = 64;      // 4 byte
// TOPLAM: 12 byte
```

#### ✅ İyi Kullanım (Optimize)

```cpp
uint8_t red = 255;    // 1 byte
uint8_t green = 128;  // 1 byte
uint8_t blue = 64;    // 1 byte
// TOPLAM: 3 byte (75% tasarruf!)
```

### 2.3 Kodumuzdaki Örnekler

```cpp
// RGB değerleri her zaman 0-255, uint8_t yeterli
uint8_t baseR = 102, baseG = 102, baseB = 102;

// Parlaklık hesaplamalarında taşma olmasın diye uint16_t
static inline uint8_t applyBrightness(uint8_t v) {
  uint16_t out = (uint16_t)v * (uint16_t)brightness / 255;
  //           ↑ 255 * 255 = 65025 → uint8_t'ye sığmaz!
  return (uint8_t)out;
}
```

### 2.4 Neden Casting Yapıyoruz?

```cpp
uint8_t v = 200;
uint8_t brightness = 128;

// ❌ YANLIŞ: Taşma riski
uint8_t result = v * brightness / 255;
// 200 * 128 = 25600 → uint8_t max 255, TAŞAR!

// ✅ DOĞRU: Önce uint16_t'ye çevir
uint16_t temp = (uint16_t)v * (uint16_t)brightness;
// 25600 → uint16_t'ye sığar
uint8_t result = temp / 255;  // = 100
```

---

## 3. PROGMEM ve Flash Bellek

### 3.1 ESP32 Bellek Yapısı

```
┌─────────────────────────────────────┐
│         ESP32 BELLEK                │
├─────────────────────────────────────┤
│  RAM (SRAM)                         │
│  └─ 520 KB                          │
│  └─ Hızlı, ama az                   │
│  └─ Değişkenler burada              │
├─────────────────────────────────────┤
│  Flash (ROM)                        │
│  └─ 4 MB - 16 MB                    │
│  └─ Yavaş, ama çok                  │
│  └─ Program kodu burada             │
└─────────────────────────────────────┘
```

### 3.2 PROGMEM Nedir?

`PROGMEM`, sabit verileri **RAM yerine Flash'e** koyar.

#### ❌ Normal Kullanım (RAM'de)

```cpp
const uint8_t gammaTable[256] = {
  0, 0, 0, ... 255
};
// 256 byte RAM kullanır
```

#### ✅ PROGMEM Kullanımı (Flash'te)

```cpp
const uint8_t PROGMEM gammaTable[256] = {
  0, 0, 0, ... 255
};
// 0 byte RAM kullanır! (256 byte Flash'te)
```

### 3.3 pgm_read_byte() Neden Gerekli?

Flash bellekteki verilere **direkt erişemezsiniz**. Özel fonksiyon gerekir.

```cpp
// ❌ ÇALIŞMAZ: Flash'ten direkt okuma
uint8_t value = gammaTable[128];

// ✅ ÇALIŞIR: pgm_read_byte ile Flash'ten oku
uint8_t value = pgm_read_byte(&gammaTable[128]);
```

### 3.4 Gamma Tablosu Neden Flash'te?

```cpp
const uint8_t PROGMEM gammaTable[256] = {
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    // ... 256 değer
  215,218,220,223,225,228,231,233,236,239,241,244,247,249,252,255
};

static inline uint8_t applyGamma(uint8_t v) {
  return pgm_read_byte(&gammaTable[v]);
  //     └─ Flash'ten oku
}
```

**Neden Flash'te tutuyoruz?**

1. **256 byte** sürekli RAM'de durması gereksiz
2. Tablo **hiç değişmiyor** (const)
3. Saniyede 1000 kez okunsa bile yeterince hızlı

**Kullanım:**

```cpp
uint8_t corrected = applyGamma(128);
// gammaTable[128] Flash'ten okunur
// Gamma düzeltmesi uygulanır
```

---

## 4. Serial Haberleşme

### 4.1 Serial.begin(115200) Nedir?

```cpp
Serial.begin(115200);
```

**Baud Rate (Baud Hızı):** Saniyede kaç bit veri gönderilir.

| Baud Rate  | bit/s      | Açıklama                  |
| ---------- | ---------- | ------------------------- |
| 9600       | 9600       | Eski standart, yavaş      |
| 57600      | 57600      | Orta hız                  |
| **115200** | **115200** | **Modern standart** ✅    |
| 230400     | 230400     | Çok hızlı (bazen sorunlu) |

### 4.2 Neden 115200?

```
Veri Gönderme Hızı Karşılaştırması:

"Hello World" (11 byte = 88 bit) göndermek için:

9600 baud:    88/9600  = 9.2 ms
115200 baud:  88/115200 = 0.76 ms  ← 12x DAHA HIZLI!
```

**Avantajları:**

- ✅ ESP32'nin native hızı
- ✅ Gerçek zamanlı debug için yeterince hızlı
- ✅ Modern Arduino IDE varsayılanı
- ✅ Güvenilir (hata riski düşük)

### 4.3 Serial Kullanımı

```cpp
void setup() {
  Serial.begin(115200);

  Serial.print("Merhaba");      // Yeni satıra geçmeden yaz
  Serial.println(" Dünya!");    // Yeni satıra geç

  int value = 42;
  Serial.print("Değer: ");
  Serial.println(value);
  // Çıktı: "Değer: 42"
}

void loop() {
  Serial.print("Parlaklık: ");
  Serial.print(brightness);
  Serial.println("/255");
  delay(1000);
}
```

**Serial Monitor'de göreceğiniz:**

```
Merhaba Dünya!
Değer: 42
Parlaklık: 128/255
Parlaklık: 128/255
Parlaklık: 128/255
...
```

---

## 5. PWM ve LED Kontrolü

### 5.1 PWM (Pulse Width Modulation) Nedir?

PWM, **dijital pini hızlıca açıp kapatarak** analog bir sinyal gibi davranmasını sağlar.

```
Duty Cycle: %0 (Kapalı)
─────────────────────────────────────

Duty Cycle: %25 (Düşük Parlaklık)
█───█───█───█───█───█───█───█───

Duty Cycle: %50 (Orta Parlaklık)
██──██──██──██──██──██──██──██──

Duty Cycle: %75 (Yüksek Parlaklık)
███─███─███─███─███─███─███─███─

Duty Cycle: %100 (Tam Açık)
████████████████████████████████
```

### 5.2 Frekans Neden Önemli?

```cpp
static const int PWM_FREQ = 5000;  // 5kHz = Saniyede 5000 döngü
```

**Görsel Algı:**

```
100 Hz  → İnsan gözü titreşimi görebilir ❌
500 Hz  → Hafif titreşim hissedilebilir ⚠️
5000 Hz → Tamamen sabit görünür ✅
```

**Saniyede 5000 kez açılıp kapanıyor:**

```
1 saniye = 1000 ms
1 döngü = 1000/5000 = 0.2 ms

Her 0.2 ms'de bir LED açılıp kapanıyor
→ İnsan gözü algılayamayacak kadar hızlı!
```

### 5.3 PWM Resolution (Çözünürlük)

```cpp
static const int PWM_RES = 8;  // 8-bit = 2^8 = 256 seviye
```

| Bit | Seviye | Açıklama                 |
| --- | ------ | ------------------------ |
| 8   | 256    | 0-255 arası parlaklık    |
| 10  | 1024   | Daha hassas ama gereksiz |
| 12  | 4096   | Profesyonel amaçlar için |

**Neden 8-bit?**

- ✅ İnsan gözü 256 seviye arasındaki farkı zaten ayırt edemez
- ✅ CPU yükü düşük
- ✅ RGB değerleri de 0-255 (uyumlu)

### 5.4 PWM Nasıl Kullanılır?

```cpp
void setup() {
  // Kanal ayarla
  ledcSetup(CH_R, PWM_FREQ, PWM_RES);
  //         │      │         │
  //         │      │         └─ Çözünürlük (8-bit)
  //         │      └─ Frekans (5kHz)
  //         └─ Kanal numarası (0)

  // Pini kanala bağla
  ledcAttachPin(PIN_R, CH_R);
  //             │      │
  //             │      └─ Kanal 0
  //             └─ D0 pini
}

void loop() {
  // Parlaklık yaz (0-255)
  ledcWrite(CH_R, 128);  // %50 parlaklık
  //        │      │
  //        │      └─ Değer (0-255)
  //        └─ Kanal 0
}
```

### 5.5 İnsan Gözü Parlaklığı Nasıl Algılar?

#### Problem: Lineer vs Logaritmik Algı

```
PWM Değeri → LED Işığı → İnsan Algısı
──────────────────────────────────────
0           → %0        → Kapalı
64          → %25       → %10 gibi görünür ❌
128         → %50       → %25 gibi görünür ❌
192         → %75       → %50 gibi görünür ❌
255         → %100      → Tam parlak
```

**Grafik:**

```
Lineer LED        İnsan Algısı
    │                  │
255 ├──────────        │        ┌──────
    │                  │      ┌─┘
128 ├──────────        │    ┌─┘
    │                  │  ┌─┘
 64 ├──────────        │┌─┘
    │                  ├┘
  0 └──────────        └──────────
    Düz çizgi         Eğri (logaritmik)
```

### 5.6 Gamma Düzeltme ile Çözüm

```cpp
// Gamma = 2.2 formülü
output = 255 * (input/255)^2.2

// Örnek:
input = 128 → output = 55   (değil 128)
input = 192 → output = 138  (değil 192)
```

**Gamma Düzeltme Tablosu:**

```cpp
const uint8_t PROGMEM gammaTable[256] = {
  0, 0, 0, ... 55, ... 138, ... 255
  //         ↑128    ↑192
};
```

**Kullanımı:**

```cpp
void writeRGB(uint8_t r, uint8_t g, uint8_t b) {
  // 1. Gamma düzeltme uygula
  r = applyGamma(r);  // 128 → 55
  g = applyGamma(g);
  b = applyGamma(b);

  // 2. Parlaklık uygula
  r = applyBrightness(r);

  // 3. LED'e yaz
  ledcWrite(CH_R, r);
}
```

**Sonuç:**

```
Girdi → Gamma → PWM → LED → Göz Algısı
─────────────────────────────────────
128   →  55   → %22 → Orta → %50 algı ✅
192   → 138   → %54 → Parlak → %75 algı ✅
```

---

## 6. Fonksiyon Açıklamaları

### 6.1 applyBrightness() - Parlaklık Ölçekleme

```cpp
static inline uint8_t applyBrightness(uint8_t v) {
  uint16_t out = (uint16_t)v * (uint16_t)brightness / 255;
  return (uint8_t)out;
}
```

**Ne Yapar?**
Renk değerini parlaklık ile ölçekler.

**Örnek:**

```
v = 200 (Renk değeri)
brightness = 128 (Yarım parlaklık)

out = 200 * 128 / 255 = 100

Sonuç: 200 → 100 (yarıya düştü)
```

**Neden uint16_t?**

```
200 * 128 = 25600
→ uint8_t max 255, TAŞAR!
→ uint16_t max 65535, GÜVENLİ ✅
```

### 6.2 maybeInvert() - Common Anode Düzeltme

```cpp
static inline uint8_t maybeInvert(uint8_t v) {
  return INVERT ? (uint8_t)(255 - v) : v;
}
```

**LED Türleri:**

**Common Cathode (-):**

```
     LED
      │
Pin ──┤├── GND
      │

HIGH = LED AÇIK ✅
LOW  = LED KAPALI
```

**Common Anode (+):**

```
     LED
      │
Pin ──┤├── +3.3V
      │

HIGH = LED KAPALI
LOW  = LED AÇIK ✅  (ters!)
```

**Kod:**

```cpp
static const bool INVERT = true;  // Common Anode

// 255 yazmak istiyoruz (tam parlak)
uint8_t value = 255;
value = maybeInvert(value);  // → 0
ledcWrite(CH_R, 0);  // 0 = Common Anode'da tam parlak!

// 0 yazmak istiyoruz (kapalı)
value = 0;
value = maybeInvert(value);  // → 255
ledcWrite(CH_R, 255);  // 255 = Common Anode'da kapalı!
```

### 6.3 hsvToRgb() - Renk Uzayı Dönüşümü

```cpp
void hsvToRgb(float h, float s, float v, int &r, int &g, int &b)
```

**HSV Nedir?**

```
H (Hue)        → Renk tonu (0-360°)
S (Saturation) → Doygunluk (0-1)
V (Value)      → Parlaklık (0-1)
```

**Renk Çemberi:**

```
    0° Kırmızı
   360° ↑
        │
270° ←──┼──→ 90°
  Mavi  │  Sarı
        ↓
      180° Cyan
```

**Örnekler:**

```cpp
hsvToRgb(0,   1.0, 1.0, r, g, b);  // Kırmızı (255, 0, 0)
hsvToRgb(120, 1.0, 1.0, r, g, b);  // Yeşil   (0, 255, 0)
hsvToRgb(240, 1.0, 1.0, r, g, b);  // Mavi    (0, 0, 255)
hsvToRgb(180, 0.5, 1.0, r, g, b);  // Açık Cyan
```

**Rainbow Animasyonunda Kullanımı:**

```cpp
static float hue = 0;

hsvToRgb(hue, 1.0, 1.0, r, g, b);
writeRGB(r, g, b);

hue += 4;  // Her seferinde 4° artır
if (hue >= 360) hue = 0;  // 360°'ye ulaşınca başa dön

// Sonuç: Kırmızı → Sarı → Yeşil → Cyan → Mavi → Magenta → Kırmızı
```

### 6.4 getParam() - URL Parametresi Çekme

```cpp
String getParam(const String &req, const String &key)
```

**HTTP İsteği Örneği:**

```
GET /api/set?r=255&g=128&b=64&bri=200 HTTP/1.1
         │    └─────────────────────┘
         │         Query String
      URL Path
```

**Nasıl Çalışır?**

```cpp
String request = "GET /api/set?r=255&g=128&b=64 HTTP/1.1";

String r = getParam(request, "r");  // "255"
String g = getParam(request, "g");  // "128"
String b = getParam(request, "b");  // "64"

int red = r.toInt();    // 255
int green = g.toInt();  // 128
int blue = b.toInt();   // 64
```

**Adım Adım:**

```
1. "?" karakterini bul
   └─ Query string başlangıcı

2. " " (boşluk) karakterini bul
   └─ Query string bitişi

3. "r=" aranan key'i bul

4. "&" veya sonuna kadar oku
   └─ Value'yu al

5. String olarak döndür
```

### 6.5 writeRGB() - Ana LED Yazma Fonksiyonu

```cpp
void writeRGB(uint8_t r, uint8_t g, uint8_t b) {
  // 1. Gamma düzeltme
  r = applyGamma(r);
  g = applyGamma(g);
  b = applyGamma(b);

  // 2. Parlaklık uygula
  r = applyBrightness(r);
  g = applyBrightness(g);
  b = applyBrightness(b);

  // 3. Invert (Common Anode)
  r = maybeInvert(r);
  g = maybeInvert(g);
  b = maybeInvert(b);

  // 4. PWM'e yaz
  ledcWrite(CH_R, r);
  ledcWrite(CH_G, g);
  ledcWrite(CH_B, b);
}
```

**Veri Akışı:**

```
Girdi      Gamma   Parlaklık  Invert   PWM    LED
(255,0,0) → (255,0,0) → (200,0,0) → (55,255,255) → Kırmızı
          ↑ Düzelt  ↑ %78     ↑ C.Anode
```

---

## 7. Web Sunucusu

### 7.1 HTTP Protokolü

**İstek (Request):**

```
GET /api/set?r=255&g=0&b=0 HTTP/1.1
Host: 192.168.4.1
User-Agent: Mozilla/5.0
```

**Cevap (Response):**

```
HTTP/1.1 200 OK
Content-type: text/html; charset=utf-8
Connection: close

<!DOCTYPE html>
<html>
...
</html>
```

### 7.2 Kod Akışı

```cpp
void loop() {
  // 1. Bağlantı bekle
  WiFiClient client = server.available();
  if (!client) return;  // Bağlantı yoksa devam et

  // 2. İsteği oku
  String request = client.readStringUntil('\r');
  // "GET /api/set?r=255&g=0&b=0 HTTP/1.1"

  // 3. İsteği işle
  if (request.indexOf("GET /api/set") != -1) {
    // API isteği
    String r = getParam(request, "r");
    String g = getParam(request, "g");
    String b = getParam(request, "b");

    baseR = r.toInt();
    baseG = g.toInt();
    baseB = b.toInt();

    writeRGB(baseR, baseG, baseB);

    // Basit cevap
    sendPlainOK(client, "OK set");
  } else {
    // Web sayfası gönder
    sendControlPage(client);
  }

  // 4. Bağlantıyı kapat
  client.stop();
}
```

### 7.3 String İşlemleri

```cpp
String text = "Hello World";

// Karakter sayısı
int len = text.length();  // 11

// Alt string
String sub = text.substring(0, 5);  // "Hello"

// Arama
int pos = text.indexOf("World");  // 6
int pos2 = text.indexOf("XYZ");   // -1 (bulunamadı)

// Değiştirme
text.replace("World", "ESP32");  // "Hello ESP32"

// Birleştirme
String full = "IP: " + String(192) + "." + String(168);
// "IP: 192.168"
```

### 7.4 JSON Alternatifi - Query String

**Neden Query String?**

- ✅ Basit, küçük
- ✅ GET isteği ile uyumlu
- ✅ JSON kütüphanesi gerektirmez
- ✅ Bellek tasarrufu

```cpp
// Query String
GET /api/set?r=255&g=0&b=0

// JSON (daha karmaşık)
POST /api/set
Content-Type: application/json

{"r":255,"g":0,"b":0}
```

---

## 8. Zamanlama ve Delay

### 8.1 millis() Fonksiyonu

```cpp
unsigned long now = millis();
// ESP32 açıldığından beri geçen milisaniye
```

**Örnek:**

```
0 ms    → millis() = 0
1000 ms → millis() = 1000
5000 ms → millis() = 5000
```

### 8.2 Animasyon Zamanlaması

```cpp
unsigned long previousMillis = 0;

void loop() {
  unsigned long now = millis();

  if (now - previousMillis >= 1000) {
    // 1 saniye geçtiyse
    previousMillis = now;

    // Animasyon adımı
    changeColor();
  }
}
```

**Neden delay() Kullanmıyoruz?**

```cpp
// ❌ KÖTÜ: Tüm kodu bloklar
void loop() {
  changeColor();
  delay(1000);  // 1 saniye bekle (hiçbir şey yapma)
  // Bu süre içinde buton basışı algılanamaz!
}

// ✅ İYİ: Non-blocking
void loop() {
  if (millis() - previousMillis >= 1000) {
    // Zaman kontrolü
  }

  handleButton();  // Her loop'ta kontrol edilir
  handleWeb();
  // Hiçbir şey bloke edilmez!
}
```

---

## 9. Bellek Optimizasyonu Özetı

### 9.1 Kullandığımız Teknikler

| Teknik                 | Tasarruf                | Açıklama                     |
| ---------------------- | ----------------------- | ---------------------------- |
| `uint8_t` yerine `int` | %75                     | 4 byte → 1 byte              |
| `PROGMEM`              | 256 byte RAM            | Flash'e taşındı              |
| `static inline`        | CPU                     | Fonksiyon çağrı maliyeti yok |
| `const`                | Derleyici optimizasyonu | Sabitler optimize edilir     |

### 9.2 Toplam Bellek Kullanımı

```
Değişkenler:
- baseR, baseG, baseB      : 3 byte
- brightness               : 4 byte
- animationMode            : 4 byte
- previousMillis           : 4 byte
- Strings                  : ~100 byte
- Diğer                    : ~50 byte
─────────────────────────────────────
TOPLAM RAM:                 ~165 byte

Flash (Program):            ~50 KB
Flash (Gamma Tablosu):      256 byte
─────────────────────────────────────
```

---

## 10. Geliştirme İpuçları

### 10.1 Debug Nasıl Yapılır?

```cpp
void setup() {
  Serial.begin(115200);
}

void loop() {
  // Değişken değerlerini yazdır
  Serial.print("R: "); Serial.print(baseR);
  Serial.print(" G: "); Serial.print(baseG);
  Serial.print(" B: "); Serial.println(baseB);

  // Koşul kontrolü
  if (animationMode == 2) {
    Serial.println("Rainbow aktif!");
  }

  delay(500);  // Spam önlemek için
}
```

### 10.2 Ortak Hatalar

#### ❌ Taşma (Overflow)

```cpp
uint8_t a = 200;
uint8_t b = 100;
uint8_t c = a + b;  // 300 → TAŞAR! c = 44
```

#### ✅ Çözüm

```cpp
uint16_t c = (uint16_t)a + (uint16_t)b;  // 300 ✅
```

#### ❌ Bölme Hatası

```cpp
uint8_t result = 200 * 128 / 255;  // TAŞAR!
```

#### ✅ Çözüm

```cpp
uint16_t temp = (uint16_t)200 * 128;
uint8_t result = temp / 255;  // Güvenli
```

---

## 11. Sık Sorulan Sorular

### S1: Neden float yerine uint8_t kullanıyoruz?

**C:** `float` 4 byte ve yavaş. `uint8_t` 1 byte ve hızlı. RGB değerleri tam sayı (0-255) olduğu için `uint8_t` yeterli ve verimli.

### S2: INVERT neden gerekli?

**C:** Common Anode LED'lerde lojik tersine çalışır. HIGH = kapalı, LOW = açık. `maybeInvert()` bunu otomatik düzeltir.

### S3: Gamma düzeltme olmazsa ne olur?

**C:** LED'ler lineer çalışır ama göz logaritmik algılar. Orta tonlar çok parlak, düşük tonlar çok karanlık görünür. Gamma düzeltme bunu dengeler.

### S4: Neden 5000 Hz PWM?

**C:** İnsan gözü 100 Hz üzerindeki titreşimleri göremez. 5000 Hz hem yeterince hızlı hem de CPU'yu çok yormaz.

### S5: millis() 49 gün sonra ne olur?

**C:** `unsigned long` taşar ve 0'a döner. Ama fark hesaplaması yine çalışır (`now - previousMillis`).

---

## 12. Kaynaklar ve İleri Okuma

### ESP32 Dokümantasyonu

- [ESP32 Datasheet](https://www.espressif.com/sites/default/files/documentation/esp32_datasheet_en.pdf)
- [Arduino ESP32 Core](https://github.com/espressif/arduino-esp32)

### PWM ve LED

- [PWM Tutorial](https://learn.sparkfun.com/tutorials/pulse-width-modulation)
- [Gamma Correction](https://learn.adafruit.com/led-tricks-gamma-correction)

### C++ Temelleri

- [Arduino Language Reference](https://www.arduino.cc/reference/en/)
- [C++ Tutorial](https://www.cplusplus.com/doc/tutorial/)

---

**📝 Son Güncelleme:** 23 Aralık 2025  
**✍️ Hazırlayan:** RGB LED Kontrol Projesi  
**📄 Lisans:** MIT

_Bu dokümantasyon, projedeki her satır kodun nasıl ve neden öyle yazıldığını açıklar. Herhangi bir sorunuz varsa lütfen sorun!_
