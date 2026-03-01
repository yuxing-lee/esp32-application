# 智慧貨架庫存管理系統

使用 ESP32-S3 微控制器搭配 HX711 秤重模組，實現自動化庫存監控系統，當商品數量變化時即時發送 LINE 通知。

> 開發時間：約 1.5 小時完成

## 專案展示

```
商品放入 → 秤重偵測 → 計算數量 → LINE 通知「入庫 +1 件」
商品取出 → 秤重偵測 → 計算數量 → LINE 通知「出庫 -1 件」
庫存不足 → 自動警告 → LINE 通知「庫存不足！」
```

## 系統架構

```
┌─────────────┐     ┌─────────────┐     ┌─────────────┐
│   荷重元     │ ──→ │   HX711     │ ──→ │  ESP32-S3   │
│ (Load Cell) │     │  秤重模組    │     │   微控制器   │
└─────────────┘     └─────────────┘     └──────┬──────┘
                                               │
                                               ↓ WiFi
                                        ┌─────────────┐
                                        │  LINE API   │
                                        │   推播通知   │
                                        └─────────────┘
```

## 硬體需求

| 項目 | 規格 | 價格 (約) |
|------|------|----------|
| ESP32-S3 開發板 | WROOM 模組，需有 USB 孔 | NT$200-350 |
| HX711 模組 | 24-bit ADC 秤重模組 | NT$40 |
| 荷重元 (Load Cell) | 5kg 或 10kg | NT$100 |
| 400 孔麵包板 | - | NT$40 |
| 杜邦線 | 公對公 | NT$30 |
| USB 充電器 + 線 | 5V 1A 以上 | 用舊的 |
| **總計** | | **約 NT$500** |

## 接線圖

```
ESP32-S3              HX711              荷重元
  3.3V       ───→      VCC
  GND        ───→      GND
  GPIO4      ───→      DT
  GPIO5      ───→      SCK
                                    紅線 ───→ E+
                                    黑線 ───→ E-
                                    白線 ───→ A-
                                    綠線 ───→ A+
```

> 注意：荷重元線的顏色可能因廠牌不同而異，請參考賣家說明。

## 開發環境建置

### 1. 安裝 Arduino IDE

下載網址：https://www.arduino.cc/en/software

### 2. 新增 ESP32 開發板支援

1. 打開 Arduino IDE
2. 檔案 → 偏好設定
3. 「額外的開發板管理員網址」填入：
   ```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
4. 工具 → 開發板 → 開發板管理員
5. 搜尋「ESP32」→ 安裝 **esp32 by Espressif Systems**

### 3. ESP32-S3 特殊設定

| 設定項目 | 選擇 |
|----------|------|
| 開發板 | ESP32S3 Dev Module |
| USB Mode | Hardware CDC and JTAG |
| USB CDC On Boot | Enabled |

### 4. 安裝程式庫

1. 草稿碼 → 匯入程式庫 → 管理程式庫
2. 搜尋「HX711」
3. 安裝 **HX711 by Bogdan Necula**

## 校正流程

### Step 1：取得校正值

上傳校正程式，使用已知重量的物品：

```cpp
#include "HX711.h"

#define DT_PIN  4
#define SCK_PIN 5

HX711 scale;

void setup() {
  Serial.begin(115200);
  delay(3000);
  
  scale.begin(DT_PIN, SCK_PIN);
  
  Serial.println("移除秤上所有物品...");
  delay(5000);
  
  scale.set_scale();
  scale.tare();
  
  Serial.println("放上已知重量的物品...");
  delay(5000);
  
  long reading = scale.get_units(20);
  Serial.print("讀數: ");
  Serial.println(reading);
  Serial.println("校正值 = 讀數 / 實際重量(克)");
}

void loop() {}
```

**計算公式：**
```
校正值 = 讀數 ÷ 實際重量（克）
```

### Step 2：計算商品重量

1. 秤空盒子重量
2. 放入已知數量的商品
3. 計算單件重量

```
單件商品重量 = (總重量 - 盒子重量) ÷ 商品數量
```

## LINE 通知設定

### 1. 建立 LINE Bot

1. 前往 https://developers.line.biz/
2. 登入 LINE 帳號
3. Create New Provider → 輸入名稱
4. Create a Messaging API channel
5. 填寫 Channel 資訊

### 2. 取得 Token 和 User ID

| 項目 | 位置 |
|------|------|
| Channel Access Token | Messaging API 分頁 → Issue |
| Your User ID | Basic settings 分頁 |

### 3. 加入好友

掃描 Messaging API 分頁的 QR Code，加 Bot 為好友。

## 功能說明

| 功能 | 說明 |
|------|------|
| 即時秤重 | 每 2 秒讀取一次重量 |
| 自動計數 | 根據重量計算商品數量 |
| 入庫偵測 | 數量增加時發送 LINE 通知 |
| 出庫偵測 | 數量減少時發送 LINE 通知 |
| 低庫存警告 | 低於設定值時警告（預設 3 件） |
| 零庫存警告 | 庫存為零時緊急通知 |
| **穩定判斷** | **連續 N 次讀到相同數量才觸發通知，防止手動拿取時誤報** |

## 穩定判斷機制

手動拿取商品時，秤重數值可能短暫波動（如 5→3→5），導致誤發入庫/出庫通知。

為解決此問題，加入穩定計數器：只有連續 `STABLE_THRESHOLD` 次讀到相同數量，才視為穩定並觸發通知。

```
讀到數量 → 與暫存值相同？
  ├─ 是 → stableCount + 1
  └─ 否 → 重置 stableCount = 1

stableCount >= STABLE_THRESHOLD → 確認穩定 → 發送通知
```

**參數設定（可依需求調整）：**

```cpp
#define STABLE_THRESHOLD 3  // 3 次 × 2 秒 = 約 6 秒穩定後才通知
```

## 問題排解

| 問題 | 原因 | 解決方法 |
|------|------|----------|
| 序列埠沒有輸出 | ESP32-S3 USB 設定問題 | 啟用 USB CDC On Boot |
| 秤重讀數為負數 | 正常現象，未校正 | 執行校正流程計算校正值 |
| LINE 通知 400 錯誤 | JSON 格式問題 | 移除訊息中的 emoji 和換行符號 |
| HX711 沒有回應 | 接線錯誤 | 檢查 DT、SCK 腳位是否正確 |
| WiFi 連不上 | SSID/密碼錯誤或頻段問題 | ESP32 只支援 2.4GHz WiFi |

## 未來擴充

| 方向 | 說明 |
|------|------|
| 多貨架監控 | 部署多個 ESP32，同時監控多個貨架 |
| 雲端資料庫 | 上傳數據到 Google Sheets 或 Firebase |
| 網頁儀表板 | 即時顯示所有貨架狀態 |
| UHF RFID 整合 | 搭配通道門，追蹤個別商品進出 |
| 自動補貨訂單 | 低庫存時自動發送補貨請求 |

## 技術棧

- **硬體**: ESP32-S3, HX711, Load Cell
- **韌體**: Arduino / C++
- **通訊**: WiFi, HTTPS
- **API**: LINE Messaging API
- **開發工具**: Arduino IDE

## 授權

MIT License

## 作者

[YUXING LI]

---

> 這是一個 IoT 物聯網實作專案，展示如何使用低成本元件建立智慧倉儲管理系統。
