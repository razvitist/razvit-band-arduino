#include <ArduinoJson.h>
#include <LittleFS.h>
#include <time.h>
#include <Ticker.h>
#include "modes.h"
#include "pins.h"

#if defined(ESP32)
#include <WiFi.h>
#include <HTTPClient.h>
#include <esp_wpa2.h>
#include <WiFiMulti.h>
WiFiMulti wifiMulti;
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFiMulti.h>
ESP8266WiFiMulti wifiMulti;
#endif

#if defined(ESP32)
void set_color(int R, int G, int B)
{
  ledcWrite(1, R);
  ledcWrite(2, G);
  ledcWrite(3, B);
}
#endif

time_t now;
struct tm tm;

DynamicJsonDocument schedule(4096);

int minutes, wday;
int modeS, modeE;
boolean isWifiOn = 1;
int sleepTime = 60;
int temperature = 0;
boolean isMulti = 0;
int wifiOffTime = 20; // 20 -> 10 сек, чтобы сэкономить энергию

// const byte bssid[] = {0x50, 0xFF, 0x20, 0x53, 0xD5, 0xAD};

boolean isLog = 0;
String printplus = "";
void print(String s)
{
  Serial.print(s);
  if (isLog)
  {
    printplus += s;
  }
}
void println(String s = "")
{
  Serial.println(s);
  if (isLog)
  {
    File file = LittleFS.open("/log.txt", "a");
    file.println(String(tm.tm_hour < 10 ? "0" : "") + tm.tm_hour + ":" + (tm.tm_min < 10 ? "0" : "") + tm.tm_min + ":" + (tm.tm_sec < 10 ? "0" : "") + tm.tm_sec + " " + printplus + s);
    file.close();
    printplus = "";

    if (file.size() > 200 * 1024)
    {
      LittleFS.open("/log.txt", "w").close();
    }
  }
}

Ticker off;
void wifiOff()
{
  println();
  println(String("off ") + sleepTime);
  ESP.deepSleep(sleepTime * 1e6 + 1e6);
}

void setupRun()
{
  // StaticJsonDocument<200> sleepF;
  // File file = LittleFS.open("/sleep.json", "r");
  // deserializeJson(sleepF, file);
  // int sleep = sleepF["sl"].as<int>();
  // Serial.println(sleepF.as<String>());
  // Serial.println(sleep);
  // if (sleep > 0){
  //   Serial.println(String("moreSleep: ") + sleep + " min");
  // 	File file = LittleFS.open("/sleep.json", "w");
  //   if (sleep <= 200){
  // 		sleepF["sl"] = 0;
  // 		serializeJson(sleepF, file);
  //     ESP.deepSleep(sleep * 60e6);
  //   }
  //   else{
  // 		sleepF[0] = sleep - 200;
  // 		serializeJson(sleepF, file);
  //     ESP.deepSleep(1.2e10); // 200 мин.
  //   }
  // }

  DynamicJsonDocument config(2048);
  File file = LittleFS.open("/json/config.json", "r");
  deserializeJson(config, file);
  sleepTime = config["sleep"];
  temperature = config["temperature"];
  isLog = config["isLog"].as<int>();

  DynamicJsonDocument wifilist(512);
  file = LittleFS.open("/json/wifi.json", "r");
  deserializeJson(wifilist, file);
  WiFi.mode(WIFI_STA);
  WiFi.hostname(config["name"].as<String>());
  String mac = config["mac"];
  if (mac.length() == 17)
  { // Set MAC
    // IPAddress ip(172,17,52,68);
    // IPAddress gateway(172,17,52,1);
    // IPAddress subnet(255,255,255,0);
    // WiFi.config(ip, gateway, subnet);

    uint8_t newMACAddress[6];
    for (int i = 0; i < 6; i++)
    {
      // https://stackoverflow.com/questions/31830143/convert-hex-string-to-decimal-in-arduino
      // https://stackoverflow.com/questions/23576827/arduino-convert-a-string-hex-ffffff-into-3-int
      // convert hex to decimal arduino
      // hex string to int arduino
      newMACAddress[i] = (int)strtol(mac.substring(i * 3, i * 3 + 2).c_str(), NULL, 16);
    }
#if defined(ESP8266)
    wifi_set_macaddr(STATION_IF, &newMACAddress[0]);
#endif
  }
  Serial.println(WiFi.macAddress());
  if (wifilist.size() == 0)
  {
    WiFi.persistent(true);
#if defined(ESP8266)
    WiFi.begin(config["ssid"].as<const char *>(), config["pass"].as<const char *>()); // , 0, bssid
#elif defined(ESP32)
    String pass = config["pass"];
    int index = pass.indexOf(';');
    if (index != -1)
    {
      const char *ssid = config["ssid"];
      const char *username = pass.substring(0, index).c_str();
      const char *password = pass.substring(index, pass.length()).c_str();
      WiFi.disconnect(true);
      esp_wifi_sta_wpa2_ent_set_identity((uint8_t *)ssid, strlen(ssid));
      esp_wifi_sta_wpa2_ent_set_username((uint8_t *)username, strlen(username));
      esp_wifi_sta_wpa2_ent_set_password((uint8_t *)password, strlen(password));
      esp_wifi_sta_wpa2_ent_enable();
    }
    else
    {
      WiFi.begin(config["ssid"].as<const char *>(), config["pass"].as<const char *>()); // , 0, bssid
    }
#endif
    off.attach(wifiOffTime, wifiOff);
    while (WiFi.status() != WL_CONNECTED)
    {
      delay(100);
      print(".");
    }
    off.detach();
    println();
    println(WiFi.SSID() + " " + WiFi.RSSI() + " - " + millis());
  }
  else
  {
    isMulti = 1;
    if (WiFi.SSID() != "")
    {
      WiFi.disconnect(true);
#if defined(ESP8266)
      ESP.eraseConfig();
#endif
    }
    wifiMulti.addAP(config["ssid"].as<const char *>(), config["pass"].as<const char *>());
    for (int i = 0; i < int(wifilist.size()); i++)
    {
      wifiMulti.addAP(wifilist[i][0].as<const char *>(), wifilist[i][1].as<const char *>());
    }
    off.attach(wifiOffTime, wifiOff);
    if (wifiMulti.run() == WL_CONNECTED)
    {
      off.detach();
      println(String("Wi-Fi: ") + WiFi.SSID() + " " + WiFi.RSSI() + " - " + millis());
    }
  }
  configTime(3 * 60 * 60, 0, "time.cloudflare.com"); // "pool.ntp.org" "time.google.com" "time.cloudflare.com"

  // BSSID ускоряет только первое подключение (4000 -> 1000)
  // Serial.println(WiFi.BSSIDstr()); // Получить mac адрес роутера
  // Serial.println(WiFi.BSSID());
  // Serial.println(WiFi.channel());
}

void check()
{
  if (isMulti)
  {
    if (wifiMulti.run(10000) == WL_CONNECTED)
    {
      if (!isWifiOn)
      {
        isWifiOn = 1;
        off.detach();
      }
      println(String("Wi-Fi: ") + WiFi.SSID() + " " + WiFi.RSSI());
    }
    else
    {
      if (isWifiOn)
      {
        isWifiOn = 0;
        off.attach(wifiOffTime, wifiOff);
        println("Потерян сигнал Wi-Fi. Отключение через 20 сек.");
      }
    }
  }
  else
  {
    if (WiFi.status() != WL_CONNECTED && isWifiOn)
    {
      isWifiOn = 0;
      off.attach(wifiOffTime, wifiOff);
      println("Потерян сигнал Wi-Fi. Отключение через 20 сек.");
    }
    else if (WiFi.status() == WL_CONNECTED && !isWifiOn)
    {
      isWifiOn = 1;
      off.detach();
      println("Cигнал Wi-Fi восстановлен.");
    }
  }

#if defined(ESP8266)
  int voltage = ESP.getVcc();
  println(String("V-") + voltage);
  if (voltage < 3100)
  { // 3100, доп. проверка, т.к. бывает ошибка
    // delay(500);
    voltage = ESP.getVcc();
    println(String("V-") + voltage);
    if (voltage < 3000)
    {
      println(String("Voltage-") + ESP.getVcc() + ". LOW BATTERY. Sleep...");
      ESP.deepSleep(0); // !!!
    }
  }
#elif defined(ESP32)
  if (analogRead(V_PIN) < 3000)
  {
    for (int i = 1023; i > 0; i--)
    {
      set_color(i, 0, 0);
      delay(1);
    }
    esp_deep_sleep_start();
  }
  if (digitalRead(BTN_PIN) == 0 && digitalRead(0) == 0)
  {
    for (int i = 1023; i > 0; i--)
    {
      set_color(i, 0, 0);
      delay(1);
    }
    esp_deep_sleep_start();
  }
#endif
}

void run()
{
  setupRun();
  while (true)
  {
    time(&now);
    localtime_r(&now, &tm);
    minutes = tm.tm_hour * 60 + tm.tm_min;
    wday = tm.tm_wday ? tm.tm_wday : 7;
    if (tm.tm_year != 70)
    {
      // Serial.println(now); // unix
      // Serial.println(now / 86400); // кол-во дней
      // Serial.println(now / 86400 % 2); // через n дней
      // Serial.println((now / 86400 + 4) % 7);
      // Serial.println((now / 86400 + 4) % 14);
      // Serial.println((now / 86400 + 4) % 14 < 7 ? 0 : 1); // Чётные / нечётные недели
      // Serial.println(int((now / 86400 + 4) % 14 / 2)); // Чётные / нечётные недели
      println("Time");
      File file = LittleFS.open("/json/schedule.json", "r");
      deserializeJson(schedule, file);
      boolean flag = 1;
      for (int i = 0; i < int(schedule.size()); i++)
      {
        // bitRead(int(schedule[i][3]), 7 - wday) - для закодированного
        if (schedule[i][1] <= minutes && schedule[i][2] >= minutes && schedule[i][3][wday - 1])
        {
          flag = 0;
          for (String j : schedule[i][0].as<JsonArray>())
          {
            modes(j, int(schedule[i][1]), int(schedule[i][2]), minutes, wday);
          }
          // for (int j = 0; j < int(schedule[i][0].size()); j++){
          // 	modes(int(schedule[i][0][j]), int(schedule[i][1]), int(schedule[i][2]), minutes, wday);
          // }
          break;
        }
      }
      if (flag)
      { // && event != 10 // Если в расписании пусто + если не включён Anki через event
        println(String("Sleep ") + sleepTime + " s");
        ESP.deepSleep(sleepTime * 1e6 + 1e6);
      }
      else
      {
#if defined(ESP32)
        ledcWrite(3, 1023); // digitalWrite(R_PIN, 0);
        analogWrite(H_PIN, temperature);
        // analogWrite(H1_PIN, temperature);
        // analogWrite(H2_PIN, temperature);
        // analogWrite(H3_PIN, temperature);
        // analogWrite(H4_PIN, temperature);
        // analogWrite(H5_PIN, temperature);
        // analogWrite(H6_PIN, temperature);
        // analogWrite(H7_PIN, temperature);
#elif defined(ESP8266)
        digitalWrite(L_PIN, 0);
        analogWrite(H_PIN, temperature);
#endif
        println("FIRE");
      }
    }
    else
    {
      println("Failed to get time");
    }
    check();
#if defined(ESP32)
    delay(500);
    // esp_sleep_enable_timer_wakeup(500);
    // esp_light_sleep_start();
#elif defined(ESP8266)
    delay(500);
#endif
  }
}