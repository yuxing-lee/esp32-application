#include <WiFi.h>
#include <HTTPClient.h>
#include "HX711.h"

#define DT_PIN  4
#define SCK_PIN 5

HX711 scale;

// ===== WiFi 設定 =====
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// ===== LINE Messaging API =====
const char* lineToken = "YOUR_LINE_BOT_TOKEN";
const char* userId = "YOUR_LINE_USER_ID";

// ===== 秤重設定 =====
float calibrationFactor = -453.7;
float boxWeight = 177.3;
float itemWeight = 20.1;
int lowStockAlert = 3;
// =====================

// ===== 穩定判斷設定 =====
// 需連續幾次讀到相同數量才視為穩定（每次間隔 2 秒，設 3 = 約 6 秒）
#define STABLE_THRESHOLD 3
// =======================

int lastQuantity  = -1;
int pendingQuantity = -1;
int stableCount   = 0;
bool alertSent    = false;

void setup() {
  Serial.begin(115200);
  delay(3000);
  
  // 連接 WiFi
  Serial.print("連接 WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" 完成！");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
  
  // 初始化秤
  scale.begin(DT_PIN, SCK_PIN);
  scale.set_scale(calibrationFactor);
  
  Serial.println("移除所有物品...");
  delay(3000);
  scale.tare();
  
  Serial.println("系統啟動！");
  sendLineMessage("智慧貨架系統已啟動");
}

void loop() {
  if (scale.is_ready()) {
    float totalWeight = scale.get_units(10);
    float productWeight = totalWeight - boxWeight;
    if (productWeight < 0) productWeight = 0;

    int quantity = round(productWeight / itemWeight);

    // ===== 穩定判斷 =====
    if (quantity == pendingQuantity) {
      stableCount++;
    } else {
      pendingQuantity = quantity;
      stableCount = 1;
    }

    Serial.print("讀數: ");
    Serial.print(quantity);
    Serial.print("  穩定計數: ");
    Serial.print(stableCount);
    Serial.print("/");
    Serial.println(STABLE_THRESHOLD);

    // 尚未穩定，等待下一次
    if (stableCount < STABLE_THRESHOLD) {
      delay(2000);
      return;
    }

    // 穩定後才比較並通知
    stableCount = STABLE_THRESHOLD; // 防止溢位

    // 初次啟動，設定基準值不通知
    if (lastQuantity == -1) {
      lastQuantity = quantity;
      Serial.print("初始庫存: ");
      Serial.println(quantity);
      delay(2000);
      return;
    }

    // 數量穩定後有變化才通知
    if (quantity != lastQuantity) {
      int diff = quantity - lastQuantity;
      String msg;

      if (diff > 0) {
        msg = "入庫 +" + String(diff) + " 件, 目前庫存: " + String(quantity) + " 件";
      } else {
        msg = "出庫 " + String(diff) + " 件, 目前庫存: " + String(quantity) + " 件";
      }

      sendLineMessage(msg);

      // 低庫存警告（只發一次）
      if (quantity <= lowStockAlert && quantity > 0 && !alertSent) {
        sendLineMessage("警告: 庫存不足! 只剩 " + String(quantity) + " 件");
        alertSent = true;
      }

      // 庫存為零
      if (quantity == 0) {
        sendLineMessage("警告: 庫存為零! 請立即補貨");
      }

      // 補貨後重置警告
      if (quantity > lowStockAlert) {
        alertSent = false;
      }

      lastQuantity = quantity;
    }
  }

  delay(2000);
}

void sendLineMessage(String message) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi 未連線");
    return;
  }
  
  HTTPClient http;
  http.begin("https://api.line.me/v2/bot/message/push");
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Authorization", "Bearer " + String(lineToken));
  
  // JSON 格式（移除特殊符號）
  String body = "{\"to\":\"";
  body += userId;
  body += "\",\"messages\":[{\"type\":\"text\",\"text\":\"";
  body += message;
  body += "\"}]}";
  
  int httpCode = http.POST(body);
  
  if (httpCode == 200) {
    Serial.println("LINE 通知已發送: " + message);
  } else {
    Serial.print("LINE 通知失敗，錯誤碼: ");
    Serial.println(httpCode);
    Serial.println(http.getString());
  }
  
  http.end();
}