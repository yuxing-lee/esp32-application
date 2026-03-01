# ESP32 應用專案集

使用 ESP32 微控制器開發的 IoT 應用，結合各式感測器與雲端通知服務。

## 專案列表

### [smart_shelf_monitor](./smart_shelf_monitor/)

智慧貨架庫存監控系統，使用 HX711 秤重模組自動偵測商品數量，透過 LINE 即時通知庫存變化。

**主要功能：**
- 即時秤重偵測，每 2 秒更新一次
- 入庫 / 出庫自動發送 LINE 通知
- 低庫存警告（可自訂門檻）
- 庫存歸零緊急通知

**使用技術：** ESP32-S3 / HX711 / WiFi / LINE Messaging API
