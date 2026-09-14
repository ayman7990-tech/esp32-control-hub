# ESP32 Control Hub

نظام تحكم متكامل بـ ESP32 + واجهة ويب خفيفة + PWA

## المميزات

- WiFiManager للاتصال التلقائي
- تحكم في LED / Relay / Motor
- قراءة حساسات DHT11
- PWA يتثبت على الموبايل
- JSON Config على LittleFS
- تتبع الأجهزة المتصلة
- واجهة عربية RTL

## تشغيل المحاكي

cd ~/esp32-control-hub/simulator
npm install
node server.js

افتح: http://localhost:3000

## رفع على ESP32

1. Arduino IDE + المكتبات:
   - WiFiManager (tzapu)
   - ArduinoJson (Benoit Blanchon)
   - DHT sensor library (Adafruit)
2. Board: ESP32 Dev Module
3. Upload sketch
4. Tools - ESP32 LittleFS Data Upload

## الاستخدام

1. ESP32 يعمل AP: ESP32-Control
2. كلمة السر: 12345678
3. افتح: http://192.168.4.1
4. اختار شبكتك
5. ESP32 هيتصل تلقائيا

## الأطراف (GPIO)

- LED 1: 2
- LED 2: 4
- Relay 1: 5
- Relay 2: 18
- Motor: 19
- DHT11: 34

## API

GET /api/config
GET /api/state
GET /api/toggle?pin=led1
GET /api/sensors
GET /api/devices
GET /api/info

## الترخيص

MIT
