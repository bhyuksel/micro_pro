#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiAP.h>

// ---------------- Wi-Fi SoftAP ----------------
const char *ssid = "DeneyapKart";
const char *password = "dydkmpv10";
WiFiServer server(80);

// ---------------- HARİCİ RGB LED PINLERİ ----------------
// Senin kullandığın pinler:
static const int PIN_R = D0;
static const int PIN_G = D1;
static const int PIN_B = D4;

// Eğer LED’in COMMON ANODE ise true yap (aktif düşük gibi çalışır)
static const bool INVERT = true;

// ---------------- PWM (LEDC) ----------------
static const int CH_R = 0;
static const int CH_G = 1;
static const int CH_B = 2;
static const int PWM_FREQ = 5000;  // 5kHz
static const int PWM_RES  = 8;     // 0..255

// ---------------- Fiziksel Buton ----------------
#define BUTTON_PIN 0

// ---------------- Durum değişkenleri ----------------
int animationMode = 0; // 0: kapalı, 1: sabit renk geçişi, 2: rainbow, 3: pulse, 4: alarm
unsigned long previousMillis = 0;
int currentColor = 0;
String currentColorName = "Kapalı";
String currentColorHex  = "#666666";

// Parlaklık (0..255) - ARTIK MOBİLDEN DEĞİŞİYOR
int brightness = 255;

// Son seçilen baz renk (HEX’ten gelen)
uint8_t baseR = 0, baseG = 0, baseB = 0;

// Buton durumu
bool buttonState = HIGH;
bool lastButtonState = HIGH;
unsigned long buttonPressTime = 0;

// ---------------- Yardımcılar ----------------
static inline uint8_t applyBrightness(uint8_t v) {
  // v: 0..255 -> parlaklık ölçeği
  uint16_t out = (uint16_t)v * (uint16_t)brightness / 255;
  return (uint8_t)out;
}

static inline uint8_t maybeInvert(uint8_t v) {
  return INVERT ? (uint8_t)(255 - v) : v;
}

// HSV -> RGB dönüşümü (0..255)
void hsvToRgb(float h, float s, float v, int &r, int &g, int &b) {
  float c = v * s;
  float x = c * (1 - abs(fmod(h / 60.0, 2) - 1));
  float m = v - c;
  float r_, g_, b_;

  if (h < 60) { r_ = c; g_ = x; b_ = 0; }
  else if (h < 120) { r_ = x; g_ = c; b_ = 0; }
  else if (h < 180) { r_ = 0; g_ = c; b_ = x; }
  else if (h < 240) { r_ = 0; g_ = x; b_ = c; }
  else if (h < 300) { r_ = x; g_ = 0; b_ = c; }
  else { r_ = c; g_ = 0; b_ = x; }

  r = (r_ + m) * 255;
  g = (g_ + m) * 255;
  b = (b_ + m) * 255;
}

bool parseHex(String s, uint8_t &r, uint8_t &g, uint8_t &b) {
  s.trim();
  s.replace("#", "");
  if (s.length() != 6) return false;

  auto nib = [](char c)->int{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
    if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
    return -1;
  };
  auto hex2 = [&](int i)->int{
    int a = nib(s[i]), b2 = nib(s[i+1]);
    if (a < 0 || b2 < 0) return -1;
    return (a << 4) | b2;
  };

  int rr = hex2(0), gg = hex2(2), bb = hex2(4);
  if (rr < 0 || gg < 0 || bb < 0) return false;

  r = (uint8_t)rr; g = (uint8_t)gg; b = (uint8_t)bb;
  return true;
}

// query string içinden parametre çek (çok basit parser)
String getParam(const String &req, const String &key) {
  int q = req.indexOf('?');
  if (q < 0) return "";

  int sp = req.indexOf(' ', q);
  if (sp < 0) sp = req.length();

  String qs = req.substring(q + 1, sp);

  String k = key + "=";   // ✅ kopya oluşturduk

  int p = qs.indexOf(k);
  if (p < 0) return "";

  int vstart = p + k.length();
  int vend = qs.indexOf('&', vstart);
  if (vend < 0) vend = qs.length();

  String val = qs.substring(vstart, vend);
  val.replace("%23", "#");
  return val;
}


// ---------------- LED (PWM) ----------------
void writeRGB(uint8_t r, uint8_t g, uint8_t b) {
  // parlaklık uygula
  r = applyBrightness(r);
  g = applyBrightness(g);
  b = applyBrightness(b);

  // invert (common anode / aktif düşük)
  r = maybeInvert(r);
  g = maybeInvert(g);
  b = maybeInvert(b);

  ledcWrite(CH_R, r);
  ledcWrite(CH_G, g);
  ledcWrite(CH_B, b);
}

void setBaseColor(uint8_t r, uint8_t g, uint8_t b, const String& name, const String& hexStr) {
  baseR = r; baseG = g; baseB = b;
  currentColorName = name;
  currentColorHex  = hexStr;
  animationMode = 0;
  writeRGB(baseR, baseG, baseB);
}

void turnOffLED() {
  animationMode = 0;
  baseR = baseG = baseB = 0;
  currentColorName = "Kapalı";
  currentColorHex = "#666666";
  writeRGB(0,0,0);
}

// ---------------- Animasyonlar ----------------
void handleAnimations() {
  unsigned long now = millis();
  static float hue = 0;
  static bool pulseUp = true;
  static int pulseValue = 0;

  switch (animationMode) {
    case 1: // klasik renk geçişi
      if (now - previousMillis >= 1000) {
        previousMillis = now;
        switch (currentColor) {
          case 0: setBaseColor(255,0,0,"Kırmızı","#ff4444"); break;
          case 1: setBaseColor(0,255,0,"Yeşil","#44ff44"); break;
          case 2: setBaseColor(0,0,255,"Mavi","#4444ff"); break;
          case 3: setBaseColor(255,255,0,"Sarı","#ffff44"); break;
          case 4: setBaseColor(0,255,255,"Cyan","#44ffff"); break;
          case 5: setBaseColor(255,0,255,"Magenta","#ff44ff"); break;
          case 6: setBaseColor(255,255,255,"Beyaz","#ffffff"); break;
        }
        currentColor = (currentColor + 1) % 7;
        animationMode = 1; // setBaseColor mode=0 yapıyor; geri alalım
      }
      break;

    case 2: // rainbow
      if (now - previousMillis >= 40) {
        previousMillis = now;
        int r,g,b;
        hsvToRgb(hue, 1.0, 1.0, r, g, b);
        writeRGB((uint8_t)r, (uint8_t)g, (uint8_t)b);
        hue += 4; if (hue >= 360) hue = 0;
        currentColorName = "Rainbow";
        currentColorHex = "#ff00ff";
      }
      break;

    case 3: // pulse (baz rengi “nefes” gibi)
      if (now - previousMillis >= 20) {
        previousMillis = now;
        pulseValue += pulseUp ? 5 : -5;
        if (pulseValue >= 255) pulseUp = false;
        if (pulseValue <= 0)   pulseUp = true;

        // pulseValue ile base rengi ölçekle
        uint8_t r = (uint16_t)baseR * pulseValue / 255;
        uint8_t g = (uint16_t)baseG * pulseValue / 255;
        uint8_t b = (uint16_t)baseB * pulseValue / 255;
        writeRGB(r,g,b);

        currentColorName = "Pulse";
      }
      break;

    case 4: // alarm
      if (now - previousMillis >= 200) {
        previousMillis = now;
        static bool toggle = false;
        toggle = !toggle;
        if (toggle) writeRGB(255,0,0);
        else        writeRGB(0,0,255);
        currentColorName = "Alarm";
        currentColorHex = "#ff0000";
      }
      break;
  }
}

// ---------------- Fiziksel Buton ----------------
void handleButton() {
  buttonState = digitalRead(BUTTON_PIN);

  if (buttonState != lastButtonState) {
    if (buttonState == LOW) {
      buttonPressTime = millis();
    } else {
      unsigned long pressDuration = millis() - buttonPressTime;
      if (pressDuration < 500) { // kısa basış: renk değiştir (RGB)
        currentColor = (currentColor + 1) % 3;
        if (currentColor == 0) setBaseColor(255,0,0,"Kırmızı","#ff4444");
        if (currentColor == 1) setBaseColor(0,255,0,"Yeşil","#44ff44");
        if (currentColor == 2) setBaseColor(0,0,255,"Mavi","#4444ff");
      } else { // uzun basış: animasyon
        animationMode = (animationMode + 1) % 5;
        if (animationMode == 1) animationMode = 2; // 1'i geç
      }
    }
    delay(50);
  }
  lastButtonState = buttonState;
}

// ---------------- Web ----------------
void sendControlPage(WiFiClient &client) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-type:text/html; charset=utf-8");
  client.println("Connection: close");
  client.println();

  client.println("<!DOCTYPE html><html lang='tr'><head><meta charset='UTF-8'>");
  client.println("<meta name='viewport' content='width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no'>");
  client.println("<title>RGB LED Kontrol</title>");
  client.println("<link href='https://fonts.googleapis.com/css2?family=Inter:wght@300;400;500;600;700&display=swap' rel='stylesheet'>");
  client.println("<style>");
  
  // Modern CSS Reset & Variables
  client.println("*{margin:0;padding:0;box-sizing:border-box}");
  client.println(":root{--bg:#0a0a0f;--card:rgba(20,20,30,0.85);--glass:rgba(255,255,255,0.05);--border:rgba(255,255,255,0.08);--text:#f8f8fc;--text-dim:rgba(255,255,255,0.6);--accent:#6366f1;--glow:rgba(99,102,241,0.4)}");
  
  // Body & Background
  client.println("body{font-family:'Inter',system-ui,-apple-system,sans-serif;background:var(--bg);color:var(--text);min-height:100vh;padding:16px;overflow-x:hidden}");
  client.println("body::before{content:'';position:fixed;top:-50%;left:-50%;width:200%;height:200%;background:radial-gradient(circle at 30% 20%,rgba(99,102,241,0.15) 0%,transparent 50%),radial-gradient(circle at 70% 80%,rgba(236,72,153,0.12) 0%,transparent 50%);z-index:-1;animation:bgMove 20s ease-in-out infinite}");
  client.println("@keyframes bgMove{0%,100%{transform:translate(0,0)}50%{transform:translate(-5%,-5%)}}");
  
  // Container
  client.println(".container{max-width:420px;margin:0 auto}");
  
  // Header
  client.println(".header{text-align:center;padding:20px 0 30px}");
  client.println(".header h1{font-size:24px;font-weight:600;background:linear-gradient(135deg,#fff 0%,rgba(255,255,255,0.7) 100%);-webkit-background-clip:text;-webkit-text-fill-color:transparent;margin-bottom:4px}");
  client.println(".header p{font-size:13px;color:var(--text-dim)}");
  
  // Main LED Preview
  client.println(".led-preview{position:relative;width:140px;height:140px;margin:0 auto 30px;border-radius:50%;background:"+currentColorHex+";box-shadow:0 0 60px "+currentColorHex+"80,0 0 120px "+currentColorHex+"40,inset 0 0 40px rgba(255,255,255,0.2);transition:all 0.5s ease}");
  client.println(".led-preview::before{content:'';position:absolute;top:10%;left:15%;width:35%;height:20%;background:linear-gradient(180deg,rgba(255,255,255,0.6) 0%,transparent 100%);border-radius:50%;filter:blur(8px)}");
  client.println(".status-badge{display:inline-flex;align-items:center;gap:8px;background:var(--glass);backdrop-filter:blur(10px);border:1px solid var(--border);padding:10px 20px;border-radius:30px;font-size:14px;font-weight:500;margin-bottom:24px}");
  client.println(".status-dot{width:8px;height:8px;border-radius:50%;background:"+currentColorHex+";box-shadow:0 0 10px "+currentColorHex+"}");
  
  // Glass Card
  client.println(".card{background:var(--card);backdrop-filter:blur(20px);border:1px solid var(--border);border-radius:24px;padding:24px;margin-bottom:16px}");
  client.println(".card-title{font-size:13px;font-weight:600;text-transform:uppercase;letter-spacing:1px;color:var(--text-dim);margin-bottom:16px;display:flex;align-items:center;gap:8px}");
  client.println(".card-title svg{width:16px;height:16px;opacity:0.7}");
  
  // Color Wheel Section
  client.println(".color-section{text-align:center}");
  client.println(".color-wheel-wrap{position:relative;width:200px;height:200px;margin:0 auto 20px}");
  client.println(".color-wheel{width:100%;height:100%;border-radius:50%;background:conic-gradient(red,yellow,lime,aqua,blue,magenta,red);cursor:pointer;position:relative}");
  client.println(".color-wheel::before{content:'';position:absolute;inset:30%;background:radial-gradient(circle,#fff,#ccc);border-radius:50%}");
  client.println(".color-picker-native{position:absolute;bottom:10px;right:10px;width:50px;height:50px;border:none;border-radius:50%;cursor:pointer;background:var(--glass);backdrop-filter:blur(10px);border:2px solid var(--border)}");
  client.println(".apply-btn{width:100%;padding:16px;border:none;border-radius:16px;background:linear-gradient(135deg,var(--accent) 0%,#8b5cf6 100%);color:#fff;font-size:15px;font-weight:600;cursor:pointer;transition:all 0.3s;box-shadow:0 4px 20px var(--glow)}");
  client.println(".apply-btn:active{transform:scale(0.98);opacity:0.9}");
  
  // Brightness Slider
  client.println(".brightness-section{margin-bottom:10px}");
  client.println(".brightness-header{display:flex;justify-content:space-between;align-items:center;margin-bottom:16px}");
  client.println(".brightness-value{font-size:28px;font-weight:700;background:linear-gradient(135deg,#fbbf24,#f59e0b);-webkit-background-clip:text;-webkit-text-fill-color:transparent}");
  client.println(".slider-track{position:relative;height:50px;background:linear-gradient(90deg,#1a1a2e 0%,#fbbf24 100%);border-radius:25px;padding:6px}");
  client.println(".slider{-webkit-appearance:none;width:100%;height:100%;background:transparent;border-radius:20px;cursor:pointer}");
  client.println(".slider::-webkit-slider-thumb{-webkit-appearance:none;width:38px;height:38px;background:#fff;border-radius:50%;box-shadow:0 4px 15px rgba(0,0,0,0.3);transition:transform 0.2s}");
  client.println(".slider::-webkit-slider-thumb:active{transform:scale(1.1)}");
  
  // Quick Colors Grid
  client.println(".colors-grid{display:grid;grid-template-columns:repeat(4,1fr);gap:12px}");
  client.println(".color-btn{aspect-ratio:1;border-radius:16px;border:2px solid transparent;cursor:pointer;transition:all 0.3s;position:relative;overflow:hidden}");
  client.println(".color-btn::before{content:'';position:absolute;inset:0;background:inherit;filter:blur(8px);opacity:0;transition:opacity 0.3s}");
  client.println(".color-btn:active{transform:scale(0.9)}");
  client.println(".color-btn:active::before{opacity:1}");
  client.println(".color-btn.red{background:linear-gradient(135deg,#ef4444,#dc2626)}");
  client.println(".color-btn.green{background:linear-gradient(135deg,#22c55e,#16a34a)}");
  client.println(".color-btn.blue{background:linear-gradient(135deg,#3b82f6,#2563eb)}");
  client.println(".color-btn.yellow{background:linear-gradient(135deg,#eab308,#ca8a04)}");
  client.println(".color-btn.cyan{background:linear-gradient(135deg,#06b6d4,#0891b2)}");
  client.println(".color-btn.magenta{background:linear-gradient(135deg,#d946ef,#c026d3)}");
  client.println(".color-btn.white{background:linear-gradient(135deg,#f8fafc,#e2e8f0)}");
  client.println(".color-btn.off{background:linear-gradient(135deg,#374151,#1f2937);display:flex;align-items:center;justify-content:center}");
  client.println(".color-btn.off::after{content:'';width:60%;height:3px;background:#ef4444;border-radius:2px;transform:rotate(-45deg)}");
  
  // Animation Cards
  client.println(".anim-grid{display:grid;grid-template-columns:repeat(2,1fr);gap:12px}");
  client.println(".anim-btn{padding:20px 16px;border-radius:20px;border:1px solid var(--border);background:var(--glass);backdrop-filter:blur(10px);cursor:pointer;text-decoration:none;color:var(--text);text-align:center;transition:all 0.3s}");
  client.println(".anim-btn:active{transform:scale(0.97);background:rgba(255,255,255,0.1)}");
  client.println(".anim-btn.active{border-color:var(--accent);box-shadow:0 0 20px var(--glow)}");
  client.println(".anim-icon{font-size:28px;margin-bottom:8px;display:block}");
  client.println(".anim-label{font-size:13px;font-weight:500}");
  client.println(".anim-btn.rainbow .anim-icon{animation:rainbowText 2s linear infinite}");
  client.println("@keyframes rainbowText{0%{filter:hue-rotate(0deg)}100%{filter:hue-rotate(360deg)}}");
  client.println(".anim-btn.pulse .anim-icon{animation:pulse 1.5s ease-in-out infinite}");
  client.println("@keyframes pulse{0%,100%{opacity:1;transform:scale(1)}50%{opacity:0.5;transform:scale(0.9)}}");
  client.println(".anim-btn.alarm .anim-icon{animation:alarm 0.5s ease-in-out infinite}");
  client.println("@keyframes alarm{0%,100%{filter:drop-shadow(0 0 8px #ef4444)}50%{filter:drop-shadow(0 0 8px #3b82f6)}}");
  
  // Footer
  client.println(".footer{text-align:center;padding:20px 0;color:var(--text-dim);font-size:12px}");
  client.println(".footer .wifi-badge{display:inline-flex;align-items:center;gap:6px;background:var(--glass);padding:8px 16px;border-radius:20px;margin-top:8px}");
  client.println(".footer .wifi-icon{width:14px;height:14px;fill:currentColor}");
  
  client.println("</style></head><body>");
  client.println("<div class='container'>");
  
  // Header
  client.println("<div class='header'>");
  client.println("<h1>✨ RGB LED Kontrol</h1>");
  client.println("<p>Akıllı LED Yönetim Paneli</p>");
  client.println("</div>");
  
  // LED Preview
  client.println("<div style='text-align:center'>");
  client.println("<div class='led-preview' id='ledPreview'></div>");
  client.println("<div class='status-badge'><span class='status-dot' id='statusDot'></span><span id='statusText'>"+currentColorName+"</span></div>");
  client.println("</div>");
  
  // Color Picker Card
  client.println("<div class='card'>");
  client.println("<div class='card-title'><svg viewBox='0 0 24 24' fill='none' stroke='currentColor' stroke-width='2'><circle cx='12' cy='12' r='10'/><path d='M12 2a10 10 0 0 1 0 20'/></svg>Renk Seçici</div>");
  client.println("<div class='color-section'>");
  client.println("<div class='color-wheel-wrap'>");
  client.println("<div class='color-wheel' id='colorWheel'></div>");
  client.println("<input type='color' class='color-picker-native' id='picker' value='"+currentColorHex+"'>");
  client.println("</div>");
  client.println("<button class='apply-btn' onclick='applyColor()'>🎨 Rengi Uygula</button>");
  client.println("</div></div>");
  
  // Brightness Card
  client.println("<div class='card'>");
  client.println("<div class='brightness-section'>");
  client.println("<div class='brightness-header'>");
  client.println("<div class='card-title' style='margin:0'><svg viewBox='0 0 24 24' fill='none' stroke='currentColor' stroke-width='2'><circle cx='12' cy='12' r='5'/><path d='M12 1v2M12 21v2M4.22 4.22l1.42 1.42M18.36 18.36l1.42 1.42M1 12h2M21 12h2M4.22 19.78l1.42-1.42M18.36 5.64l1.42-1.42'/></svg>Parlaklık</div>");
  client.println("<div class='brightness-value' id='briVal'>"+String((brightness*100)/255)+"%</div>");
  client.println("</div>");
  client.println("<div class='slider-track'>");
  client.println("<input type='range' class='slider' id='bri' min='0' max='255' value='"+String(brightness)+"' oninput='updateBrightness(this.value)'>");
  client.println("</div></div></div>");
  
  // Quick Colors Card
  client.println("<div class='card'>");
  client.println("<div class='card-title'><svg viewBox='0 0 24 24' fill='none' stroke='currentColor' stroke-width='2'><rect x='3' y='3' width='18' height='18' rx='2'/><path d='M3 9h18M9 21V9'/></svg>Hızlı Renkler</div>");
  client.println("<div class='colors-grid'>");
  client.println("<a href='/red' class='color-btn red'></a>");
  client.println("<a href='/green' class='color-btn green'></a>");
  client.println("<a href='/blue' class='color-btn blue'></a>");
  client.println("<a href='/yellow' class='color-btn yellow'></a>");
  client.println("<a href='/cyan' class='color-btn cyan'></a>");
  client.println("<a href='/magenta' class='color-btn magenta'></a>");
  client.println("<a href='/white' class='color-btn white'></a>");
  client.println("<a href='/off' class='color-btn off'></a>");
  client.println("</div></div>");
  
  // Animations Card
  String activeAnim = "";
  if (animationMode == 2) activeAnim = "rainbow";
  else if (animationMode == 3) activeAnim = "pulse";
  else if (animationMode == 4) activeAnim = "alarm";
  
  client.println("<div class='card'>");
  client.println("<div class='card-title'><svg viewBox='0 0 24 24' fill='none' stroke='currentColor' stroke-width='2'><polygon points='5 3 19 12 5 21 5 3'/></svg>Animasyonlar</div>");
  client.println("<div class='anim-grid'>");
  client.println("<a href='/animation/rainbow' class='anim-btn rainbow "+(activeAnim=="rainbow"?String("active"):String(""))+"'><span class='anim-icon'>🌈</span><span class='anim-label'>Rainbow</span></a>");
  client.println("<a href='/animation/pulse' class='anim-btn pulse "+(activeAnim=="pulse"?String("active"):String(""))+"'><span class='anim-icon'>💓</span><span class='anim-label'>Pulse</span></a>");
  client.println("<a href='/animation/alarm' class='anim-btn alarm "+(activeAnim=="alarm"?String("active"):String(""))+"'><span class='anim-icon'>🚨</span><span class='anim-label'>Alarm</span></a>");
  client.println("<a href='/animation/0' class='anim-btn'><span class='anim-icon'>⏹️</span><span class='anim-label'>Durdur</span></a>");
  client.println("</div></div>");
  
  // Footer
  client.println("<div class='footer'>");
  client.println("<div>Tasarım: Modern LED Kontrol v2.0</div>");
  client.println("<div class='wifi-badge'><svg class='wifi-icon' viewBox='0 0 24 24' fill='currentColor'><path d='M1 9l2 2c4.97-4.97 13.03-4.97 18 0l2-2C16.93 2.93 7.08 2.93 1 9zm8 8l3 3 3-3c-1.65-1.66-4.34-1.66-6 0zm-4-4l2 2c2.76-2.76 7.24-2.76 10 0l2-2C15.14 9.14 8.87 9.14 5 13z'/></svg>"+String(ssid)+"</div>");
  client.println("</div>");
  
  client.println("</div>");
  
  // JavaScript
  client.println("<script>");
  client.println("const picker=document.getElementById('picker');");
  client.println("const bri=document.getElementById('bri');");
  client.println("const briVal=document.getElementById('briVal');");
  client.println("const ledPreview=document.getElementById('ledPreview');");
  client.println("const statusDot=document.getElementById('statusDot');");
  
  client.println("picker.addEventListener('input',function(){");
  client.println("  ledPreview.style.background=this.value;");
  client.println("  ledPreview.style.boxShadow='0 0 60px '+this.value+'80, 0 0 120px '+this.value+'40, inset 0 0 40px rgba(255,255,255,0.2)';");
  client.println("  statusDot.style.background=this.value;");
  client.println("  statusDot.style.boxShadow='0 0 10px '+this.value;");
  client.println("});");
  
  client.println("function updateBrightness(val){");
  client.println("  briVal.textContent=Math.round(val/255*100)+'%';");
  client.println("  fetch('/api/brightness?value='+val).catch(()=>{});");
  client.println("}");
  
  client.println("function applyColor(){");
  client.println("  const c=picker.value;");
  client.println("  const b=bri.value;");
  client.println("  fetch('/api/set?hex='+encodeURIComponent(c)+'&bri='+b)");
  client.println("    .then(()=>setTimeout(()=>location.reload(),300))");
  client.println("    .catch(()=>location.reload());");
  client.println("}");
  
  client.println("document.getElementById('colorWheel').addEventListener('click',function(){picker.click();});");
  
  client.println("</script>");
  client.println("</body></html>");
}

void sendPlainOK(WiFiClient &client, const String& msg="OK") {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-type:text/plain; charset=utf-8");
  client.println("Connection: close");
  client.println();
  client.println(msg);
}

// ---------------- setup / loop ----------------
void setup() {
  Serial.begin(115200);

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // PWM başlat
  ledcSetup(CH_R, PWM_FREQ, PWM_RES);
  ledcSetup(CH_G, PWM_FREQ, PWM_RES);
  ledcSetup(CH_B, PWM_FREQ, PWM_RES);
  ledcAttachPin(PIN_R, CH_R);
  ledcAttachPin(PIN_G, CH_G);
  ledcAttachPin(PIN_B, CH_B);

  turnOffLED();

  Serial.println("Erisim Noktasi kuruluyor...");
  WiFi.softAP(ssid, password);
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("IP: ");
  Serial.println(myIP);

  server.begin();
}

void loop() {
  handleAnimations();
  handleButton();

  WiFiClient client = server.available();
  if (!client) return;

  // İlk satırı oku
  String request = client.readStringUntil('\r');
  client.flush();

  // --- API: custom color + brightness ---
  if (request.indexOf("GET /api/set") != -1) {
    String hex = getParam(request, "hex");  // "#RRGGBB" veya "RRGGBB"
    String bri = getParam(request, "bri");  // 0..255

    if (bri.length() > 0) {
      int b = bri.toInt();
      if (b < 0) b = 0;
      if (b > 255) b = 255;
      brightness = b;
    }

    uint8_t r,g,b;
    if (parseHex(hex, r,g,b)) {
      baseR=r; baseG=g; baseB=b;
      currentColorHex = "#" + String(hex); // görünüm için
      currentColorName = "Özel Renk";
      animationMode = 0;
      writeRGB(baseR, baseG, baseB);
      sendPlainOK(client, "OK set");
    } else {
      sendPlainOK(client, "ERR hex");
    }
    client.stop();
    return;
  }

  if (request.indexOf("GET /api/brightness") != -1) {
    String v = getParam(request, "value");
    int b = v.toInt();
    if (b < 0) b = 0;
    if (b > 255) b = 255;
    brightness = b;

    // mevcut rengi/parlaklığı hemen uygula (animasyonda da zaten etkiler)
    if (animationMode == 0) writeRGB(baseR, baseG, baseB);

    sendPlainOK(client, "OK brightness");
    client.stop();
    return;
  }

  // --- Animasyonlar ---
  if (request.indexOf("GET /animation/rainbow") != -1) { animationMode = 2; currentColorName="Rainbow"; currentColorHex="#ff00ff"; }
  else if (request.indexOf("GET /animation/pulse") != -1)  { animationMode = 3; currentColorName="Pulse";  /* pulse base color kullanır */ }
  else if (request.indexOf("GET /animation/alarm") != -1)  { animationMode = 4; currentColorName="Alarm";  currentColorHex="#ff0000"; }
  else if (request.indexOf("GET /animation/0") != -1)      { animationMode = 0; writeRGB(baseR, baseG, baseB); }

  // --- Sabit renkler ---
  else if (request.indexOf("GET /red") != -1)     { setBaseColor(255,0,0,"Kırmızı","#ff4444"); }
  else if (request.indexOf("GET /green") != -1)   { setBaseColor(0,255,0,"Yeşil","#44ff44"); }
  else if (request.indexOf("GET /blue") != -1)    { setBaseColor(0,0,255,"Mavi","#4444ff"); }
  else if (request.indexOf("GET /yellow") != -1)  { setBaseColor(255,255,0,"Sarı","#ffff44"); }
  else if (request.indexOf("GET /cyan") != -1)    { setBaseColor(0,255,255,"Cyan","#44ffff"); }
  else if (request.indexOf("GET /magenta") != -1) { setBaseColor(255,0,255,"Magenta","#ff44ff"); }
  else if (request.indexOf("GET /white") != -1)   { setBaseColor(255,255,255,"Beyaz","#ffffff"); }
  else if (request.indexOf("GET /off") != -1)     { turnOffLED(); }

  // Sayfayı gönder
  sendControlPage(client);
  client.stop();
}
