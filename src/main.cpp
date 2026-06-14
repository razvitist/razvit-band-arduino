#include <Arduino.h>
#include <LittleFS.h>
#include "web.h"
#include "run.h"
#include "pins.h"

#if defined(ESP32)
#include <WiFi.h>
#include "soc/soc.h"          // brownout
#include "soc/rtc_cntl_reg.h" // brownout
#include "ulp.h"
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
ADC_MODE(ADC_VCC);
#endif

void setup()
{
#if defined(ESP32)
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); // brownout

  setCpuFrequencyMhz(80);

  // pinMode(R_PIN, OUTPUT);
  // pinMode(G_PIN, OUTPUT);
  // pinMode(B_PIN, OUTPUT);
  // digitalWrite(B_PIN, 1);
  ledcSetup(1, 5000, 10);
  ledcSetup(2, 5000, 10);
  ledcSetup(3, 5000, 10);
  ledcAttachPin(R_PIN, 1);
  ledcAttachPin(G_PIN, 2);
  ledcAttachPin(B_PIN, 3);
  ledcWrite(3, 1023);

  pinMode(0, INPUT_PULLUP);
  pinMode(19, INPUT_PULLUP);

  ulp();
#endif

  // Настройки общие для web и run
  Serial.begin(115200); // 9600
  Serial.println();

#if defined(ESP8266)
  Serial.println(String("Volage-") + ESP.getVcc());
  if (ESP.getVcc() < 3100)
  { // 3100
    Serial.println("LOW BATTERY. Sleep...");
    ESP.deepSleep(0);
  }
#endif

  LittleFS.begin();

#if defined(ESP32)
  // digitalWrite(B_PIN, 0);
  ledcWrite(3, 0);
#elif defined(ESP8266)
  pinMode(L_PIN, OUTPUT);
  digitalWrite(L_PIN, 1);
  pinMode(H_PIN, OUTPUT);
#endif

  pinMode(BTN_PIN, INPUT_PULLUP); // 5
  if (digitalRead(BTN_PIN) == 0)
  { // digitalRead(5) == 0
    Serial.println("Запуск Web-интерфейса...");
    web();
  }
  else
  {
    Serial.println("Запуск браслета...");
    run();
  }
}

void loop() {}