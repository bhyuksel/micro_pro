vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv# 🎨 RGB LED Kontrol Sistemi - Detaylı Kod Açıklaması

Bu dokümantasyon, ESP32 tabanlı RGB LED kontrol sisteminin tüm bileşenlerini detaylıca açıklamaktadır.

---

## 📁 Dosya Yapısı

```
mikro_proje/
└── a.cpp          # Ana Arduino/ESP32 kodu
```

---

## 🔧 1. Kütüphaneler ve Başlangıç Ayarları

```cpp
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiAP.h>
```

| Kütüphane      | Açıklama                                     |
| -------------- | -------------------------------------------- |
| `WiFi.h`       | ESP32'nin Wi-Fi özelliklerini kullanmak için |
| `WiFiClient.h` | HTTP istemci bağlantıları için               |
| `WiFiAP.h`     | Access Point (Erişim Noktası) modu için      |

---

## 📶 2. Wi-Fi Erişim Noktası (SoftAP) Ayarları

```cpp
const char *ssid = "DeneyapKart";
const char *password = "dydkmpv10";
WiFiServer server(80);
```

| Değişken     | Değer         | Açıklama                               |
| ------------ | ------------- | -------------------------------------- |
| `ssid`       | "DeneyapKart" | Wi-Fi ağ adı (telefonunuzda görünecek) |
| `password`   | "dydkmpv10"   | Wi-Fi şifresi                          |
| `server(80)` | Port 80       | HTTP web sunucusu portu                |

### Nasıl Çalışır?

1. ESP32, kendi Wi-Fi ağını oluşturur
2. Telefonunuz bu ağa bağlanır
3. Tarayıcıda `192.168.4.1` adresine gidersiniz
4. Web arayüzü açılır

---

## 💡 3. RGB LED Pin Tanımları

```cpp
static const int PIN_R = D0;  // Kırmızı LED pini
static const int PIN_G = D1;  // Yeşil LED pini
static const int PIN_B = D4;  // Mavi LED pini

static const bool INVERT = true;  // Common Anode için true
```

### LED Türleri

| Tür                | INVERT  | Açıklama                                |
| ------------------ | ------- | --------------------------------------- |
| **Common Anode**   | `true`  | Ortak bacak (+), 255 = kapalı, 0 = açık |
| **Common Cathode** | `false` | Ortak bacak (-), 0 = kapalı, 255 = açık |

### Pin Bağlantı Şeması

```
ESP32          RGB LED (Common Anode)
─────          ─────────────────────
D0  ────────── R (Kırmızı)
D1  ────────── G (Yeşil)
D4  ────────── B (Mavi)
3.3V ───────── + (Ortak Anot)
```

---

## 🎛️ 4. PWM (Pulse Width Modulation) Ayarları

```cpp
static const int CH_R = 0;      // Kırmızı için PWM kanalı
static const int CH_G = 1;      // Yeşil için PWM kanalı
static const int CH_B = 2;      // Mavi için PWM kanalı
static const int PWM_FREQ = 5000;  // 5kHz frekans
static const int PWM_RES  = 8;     // 8-bit çözünürlük (0-255)
```

### PWM Nedir?

PWM, LED parlaklığını kontrol etmek için kullanılır. Dijital sinyali çok hızlı açıp kapatarak analog bir efekt yaratır.

```
255 (Tam Parlak): ████████████████████████████████
128 (Yarı):       ████████        ████████
64  (Düşük):      ████        ████        ████
0   (Kapalı):     ────────────────────────────────
```

---

## 📊 5. Durum Değişkenleri

```cpp
int animationMode = 0;          // Aktif animasyon modu
unsigned long previousMillis = 0; // Zamanlama için
int currentColor = 0;           // Mevcut renk indeksi
String currentColorName = "Kapalı";  // Gösterilecek renk adı
String currentColorHex  = "#666666"; // Web arayüzü için HEX renk

int brightness = 255;           // Parlaklık (0-255)

uint8_t baseR = 0, baseG = 0, baseB = 0;  // Seçilen RGB değerleri
```

### Animasyon Modları

| Mod | Açıklama                                  |
| --- | ----------------------------------------- |
| 0   | Kapalı / Sabit renk                       |
| 1   | Klasik renk geçişi (7 renk arası döngü)   |
| 2   | Rainbow (gökkuşağı - sürekli renk geçişi) |
| 3   | Pulse (nefes efekti - kararıp aydınlanma) |
| 4   | Alarm (kırmızı-mavi yanıp sönme)          |

---

## 🔢 6. Yardımcı Fonksiyonlar

### 6.1 Parlaklık Uygulama

```cpp
static inline uint8_t applyBrightness(uint8_t v) {
  uint16_t out = (uint16_t)v * (uint16_t)brightness / 255;
  return (uint8_t)out;
}
```

**Örnek:**

- Renk değeri: 200, Parlaklık: 128 (yarım)
- Sonuç: `200 * 128 / 255 = 100`

### 6.2 Invert (Ters Çevirme)

```cpp
static inline uint8_t maybeInvert(uint8_t v) {
  return INVERT ? (uint8_t)(255 - v) : v;
}
```

Common Anode LED'ler için değeri tersine çevirir.

### 6.3 HSV → RGB Dönüşümü

```cpp
void hsvToRgb(float h, float s, float v, int &r, int &g, int &b)
```

| Parametre        | Aralık | Açıklama                                         |
| ---------------- | ------ | ------------------------------------------------ |
| `h` (Hue)        | 0-360  | Renk tonu (kırmızı→sarı→yeşil→cyan→mavi→magenta) |
| `s` (Saturation) | 0-1    | Doygunluk (0=gri, 1=canlı renk)                  |
| `v` (Value)      | 0-1    | Parlaklık (0=siyah, 1=tam parlak)                |

**Rainbow animasyonunda kullanılır.**

---

## 🌈 7. Gamma Düzeltme Tablosu

```cpp
const uint8_t PROGMEM gammaTable[256] = {
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,
    ...
  215,218,220,223,225,228,231,233,236,239,241,244,247,249,252,255
};
```

### Neden Gamma Düzeltme?

İnsan gözü ışığı **logaritmik** olarak algılar, LED'ler ise **lineer** çalışır.

```
Gamma olmadan:          Gamma ile:
Girdi   LED   Algı      Girdi   LED   Algı
─────   ───   ────      ─────   ───   ────
128     128   Parlak    128     55    Orta
64      64    Hala      64      13    Düşük
                parlak
```

Gamma = 2.2 kullanarak renklerin daha doğru görünmesini sağlıyoruz.

---

## 💡 8. LED Kontrol Fonksiyonları

### 8.1 writeRGB - Ana LED Yazma

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

  // 3. Invert (Common Anode için)
  r = maybeInvert(r);
  g = maybeInvert(g);
  b = maybeInvert(b);

  // 4. PWM'e yaz
  ledcWrite(CH_R, r);
  ledcWrite(CH_G, g);
  ledcWrite(CH_B, b);
}
```

### İşlem Sırası

```
Girdi RGB → Gamma → Parlaklık → Invert → PWM Çıkış
(255,0,128) → (255,0,55) → (200,0,43) → (55,255,212) → LED
```

### 8.2 setBaseColor - Renk Ayarlama

```cpp
void setBaseColor(uint8_t r, uint8_t g, uint8_t b,
                  const String& name, const String& hexStr) {
  baseR = r; baseG = g; baseB = b;
  currentColorName = name;
  currentColorHex  = hexStr;
  animationMode = 0;  // Animasyonu durdur
  writeRGB(baseR, baseG, baseB);
}
```

### 8.3 turnOffLED - LED'i Kapat

```cpp
void turnOffLED() {
  animationMode = 0;
  baseR = baseG = baseB = 0;
  currentColorName = "Kapalı";
  currentColorHex = "#666666";
  writeRGB(0,0,0);
}
```

---

## 🎬 9. Animasyon Sistemi

```cpp
void handleAnimations() {
  unsigned long now = millis();

  switch (animationMode) {
    case 1: // Klasik renk geçişi
    case 2: // Rainbow
    case 3: // Pulse
    case 4: // Alarm
  }
}
```

### 9.1 Klasik Renk Geçişi (Mode 1)

Her 1 saniyede bir sonraki renge geçer:

```
Kırmızı → Yeşil → Mavi → Sarı → Cyan → Magenta → Beyaz → (tekrar)
```

### 9.2 Rainbow (Mode 2)

HSV renk uzayında Hue değerini sürekli artırarak gökkuşağı efekti:

```cpp
hue += 4;  // Her 40ms'de 4 derece artır
if (hue >= 360) hue = 0;
```

### 9.3 Pulse (Mode 3)

Mevcut rengi "nefes alır gibi" kararıp aydınlatır:

```cpp
pulseValue += pulseUp ? 5 : -5;
if (pulseValue >= 255) pulseUp = false;
if (pulseValue <= 0)   pulseUp = true;
```

### 9.4 Alarm (Mode 4)

Kırmızı ve mavi arasında hızlı geçiş:

```cpp
if (toggle) writeRGB(255,0,0);  // Kırmızı
else        writeRGB(0,0,255);  // Mavi
```

---

## 🔘 10. Fiziksel Buton Kontrolü

```cpp
void handleButton() {
  buttonState = digitalRead(BUTTON_PIN);

  if (pressDuration < 500) {
    // Kısa basış: Renk değiştir (R→G→B)
  } else {
    // Uzun basış: Animasyon modunu değiştir
  }
}
```

| Basış Süresi | Eylem                          |
| ------------ | ------------------------------ |
| < 500ms      | Kırmızı → Yeşil → Mavi döngüsü |
| ≥ 500ms      | Animasyon modunu değiştir      |

---

## 🌐 11. Web Arayüzü

### 11.1 HTML Yapısı

```
┌─────────────────────────────────────┐
│         ✨ RGB LED Kontrol          │
│         Akıllı LED Yönetim          │
├─────────────────────────────────────┤
│            ┌───────┐                │
│            │  LED  │  ← Önizleme    │
│            │ ⚪    │                │
│            └───────┘                │
│          [Özel Renk]                │
├─────────────────────────────────────┤
│  ○ RENK SEÇİCİ                      │
│     ┌─────────────┐                 │
│     │ 🎨 Renk     │                 │
│     │   Tekerleği │                 │
│     └─────────────┘                 │
│  [🎨 Rengi Uygula]                  │
├─────────────────────────────────────┤
│  ☀ PARLAKLIK                   85%  │
│  ├────────────●───────────────┤     │
├─────────────────────────────────────┤
│  ▦ HIZLI RENKLER                    │
│  ┌──┐ ┌──┐ ┌──┐ ┌──┐               │
│  │🔴│ │🟢│ │🔵│ │🟡│               │
│  └──┘ └──┘ └──┘ └──┘               │
│  ┌──┐ ┌──┐ ┌──┐ ┌──┐               │
│  │🔵│ │🟣│ │⚪│ │⬛│               │
│  └──┘ └──┘ └──┘ └──┘               │
├─────────────────────────────────────┤
│  ▶ ANİMASYONLAR                     │
│  ┌────────┐ ┌────────┐              │
│  │   🌈   │ │   💓   │              │
│  │Rainbow │ │ Pulse  │              │
│  └────────┘ └────────┘              │
│  ┌────────┐ ┌────────┐              │
│  │   🚨   │ │   ⏹️   │              │
│  │ Alarm  │ │ Durdur │              │
│  └────────┘ └────────┘              │
├─────────────────────────────────────┤
│  📶 DeneyapKart                     │
└─────────────────────────────────────┘
```

### 11.2 CSS Özellikleri

| Özellik           | Açıklama                                     |
| ----------------- | -------------------------------------------- |
| Glassmorphism     | Bulanık cam efekti (`backdrop-filter: blur`) |
| Gradyan arka plan | Hareketli mor-pembe tonları                  |
| LED Glow          | Seçilen renge göre dinamik ışıma efekti      |
| Responsive        | Mobil uyumlu tasarım                         |

### 11.3 JavaScript Fonksiyonları

#### hexToRgb - HEX → RGB Dönüşümü

```javascript
function hexToRgb(hex) {
  const r = parseInt(hex.slice(1, 3), 16);
  const g = parseInt(hex.slice(3, 5), 16);
  const b = parseInt(hex.slice(5, 7), 16);
  return { r, g, b };
}
```

**Örnek:** `#ff5500` → `{r: 255, g: 85, b: 0}`

#### Anlık Renk Güncelleme (Debounce)

```javascript
picker.addEventListener("input", function () {
  clearTimeout(colorTimeout);
  colorTimeout = setTimeout(() => {
    fetch("/api/set?r=" + rgb.r + "&g=" + rgb.g + "&b=" + rgb.b);
  }, 150); // 150ms bekle
});
```

Kullanıcı renk seçerken sürekli istek göndermemek için **150ms debounce** kullanılır.

---

## 🔗 12. API Endpoint'leri

### 12.1 Renk Ayarlama

```
GET /api/set?r=255&g=128&b=64&bri=200
```

| Parametre | Aralık | Açıklama              |
| --------- | ------ | --------------------- |
| `r`       | 0-255  | Kırmızı değeri        |
| `g`       | 0-255  | Yeşil değeri          |
| `b`       | 0-255  | Mavi değeri           |
| `bri`     | 0-255  | Parlaklık (opsiyonel) |

### 12.2 Parlaklık Ayarlama

```
GET /api/brightness?value=180
```

### 12.3 Hızlı Renkler

```
GET /red      → Kırmızı (255,0,0)
GET /green    → Yeşil (0,255,0)
GET /blue     → Mavi (0,0,255)
GET /yellow   → Sarı (255,255,0)
GET /cyan     → Cyan (0,255,255)
GET /magenta  → Magenta (255,0,255)
GET /white    → Beyaz (255,255,255)
GET /off      → Kapalı (0,0,0)
```

### 12.4 Animasyonlar

```
GET /animation/rainbow  → Rainbow modu
GET /animation/pulse    → Pulse modu
GET /animation/alarm    → Alarm modu
GET /animation/0        → Animasyonu durdur
```

---

## ⚡ 13. Setup ve Loop

### setup() - Başlangıç Ayarları

```cpp
void setup() {
  Serial.begin(115200);           // Seri haberleşme
  pinMode(BUTTON_PIN, INPUT_PULLUP); // Buton

  // PWM kanallarını ayarla
  ledcSetup(CH_R, PWM_FREQ, PWM_RES);
  ledcSetup(CH_G, PWM_FREQ, PWM_RES);
  ledcSetup(CH_B, PWM_FREQ, PWM_RES);

  // Pinleri kanallara bağla
  ledcAttachPin(PIN_R, CH_R);
  ledcAttachPin(PIN_G, CH_G);
  ledcAttachPin(PIN_B, CH_B);

  turnOffLED();  // LED'i kapat

  // Wi-Fi Access Point başlat
  WiFi.softAP(ssid, password);
  server.begin();
}
```

### loop() - Ana Döngü

```cpp
void loop() {
  handleAnimations();  // Animasyonları işle
  handleButton();      // Butonu kontrol et

  WiFiClient client = server.available();
  if (!client) return;

  String request = client.readStringUntil('\r');

  // API isteklerini işle
  if (request.indexOf("GET /api/set") != -1) { ... }
  if (request.indexOf("GET /api/brightness") != -1) { ... }
  // ...

  sendControlPage(client);  // Web sayfasını gönder
}
```

---

## 🔄 14. Veri Akış Şeması

```
┌─────────────┐     HTTP Request      ┌─────────────┐
│             │ ───────────────────▶  │             │
│   Telefon   │                       │   ESP32     │
│  (Tarayıcı) │  ◀───────────────────│  (Server)   │
│             │     HTML Response     │             │
└─────────────┘                       └──────┬──────┘
                                             │
                                             ▼
                                      ┌─────────────┐
                                      │   writeRGB  │
                                      │  Fonksiyonu │
                                      └──────┬──────┘
                                             │
                    ┌────────────────────────┼────────────────────────┐
                    ▼                        ▼                        ▼
             ┌───────────┐            ┌───────────┐            ┌───────────┐
             │ Gamma     │            │ Parlaklık │            │  Invert   │
             │ Düzeltme  │            │  Uygula   │            │ (C.Anode) │
             └─────┬─────┘            └─────┬─────┘            └─────┬─────┘
                   │                        │                        │
                   └────────────────────────┴────────────────────────┘
                                             │
                                             ▼
                                      ┌─────────────┐
                                      │  PWM Çıkış  │
                                      │ (ledcWrite) │
                                      └──────┬──────┘
                                             │
                    ┌────────────────────────┼────────────────────────┐
                    ▼                        ▼                        ▼
               ┌─────────┐             ┌─────────┐             ┌─────────┐
               │  🔴 R   │             │  🟢 G   │             │  🔵 B   │
               │  (D0)   │             │  (D1)   │             │  (D4)   │
               └─────────┘             └─────────┘             └─────────┘
                                             │
                                             ▼
                                      ┌─────────────┐
                                      │   RGB LED   │
                                      │    💡       │
                                      └─────────────┘
```

---

## 📋 15. Sorun Giderme

### LED Ters Renk Gösteriyor

```cpp
static const bool INVERT = true;  // false deneyin
```

### Renk Seçici Çalışmıyor

1. Tarayıcı konsolunu kontrol edin (F12)
2. `/api/set` isteklerini inceleyin
3. ESP32 seri monitörü kontrol edin

### Wi-Fi Bağlantı Sorunu

1. Şifrenin doğru olduğundan emin olun
2. ESP32'yi yeniden başlatın
3. Telefonu ağa tekrar bağlayın

### LED Yanmıyor

1. Pin bağlantılarını kontrol edin
2. `INVERT` değerini değiştirin
3. LED'in sağlam olduğunu test edin

---

## 📚 16. Kullanılan Teknolojiler

| Kategori     | Teknoloji                 |
| ------------ | ------------------------- |
| **Donanım**  | ESP32 / Deneyap Kart      |
| **LED**      | RGB LED (Common Anode)    |
| **Protokol** | HTTP, Wi-Fi (802.11)      |
| **PWM**      | LEDC (LED Control)        |
| **Frontend** | HTML5, CSS3, JavaScript   |
| **Tasarım**  | Glassmorphism, Responsive |

---

## 🎓 17. Öğrenilen Kavramlar

1. **PWM ile LED Kontrolü** - Parlaklık ve renk karışımı
2. **Wi-Fi SoftAP** - ESP32'yi erişim noktası olarak kullanma
3. **HTTP Web Sunucusu** - Basit REST API oluşturma
4. **Gamma Correction** - Renk algısını düzeltme
5. **Debouncing** - Gereksiz istekleri önleme
6. **HSV Renk Uzayı** - Renk geçişleri için

---

_Bu döküman, RGB LED Kontrol Sistemi v2.0 için hazırlanmıştır._
