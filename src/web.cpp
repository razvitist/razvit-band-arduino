#include <ESPAsyncWebServer.h>
#include <AsyncJson.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <AsyncElegantOTA.h>
#include "pins.h"

#if defined(ESP32)
	#include <WiFi.h>
  #include <AsyncTCP.h>

  void setColor(int R, int G, int B) {
    ledcWrite(1, R);
    ledcWrite(2, G);
    ledcWrite(3, B);
    // analogWrite(PIN_R, R);
    // analogWrite(PIN_G, G);
    // analogWrite(PIN_B, B);
  }
  int red = 254;
  int green = 1;
  int blue = 127;
  int red_direction = -1;
  int green_direction = 1;
  int blue_direction = -1;
#elif defined(ESP8266)
	#include <ESP8266WiFi.h>
  #include <ESPAsyncTCP.h>
  #include <Hash.h>
#endif

AsyncWebServer server(80);

const String params[] = {
  "name", 
  "code", 
  "ssid", 
  "pass",
  "nickname",
  "mac",
  "temperature",
  "sleep",
  "isLog",
  "ankiOne",
  "ankiMax",
  "ankiRatedOne",
  "ankiRatedMax",
  "ankiAddOne",
  "ankiAddMax",
  "codewarsName",
  "codewarsOne",
  "codewarsMax",
  "foodLen",
  "foodNext",
  "githubAddress",
  "githubOne",
  "githubMax",
  "leetcodeName",
  "leetcodeEasy",
  "leetcodeMedium",
  "leetcodeHard",
  "leetcodeMax",
  "stepikID",
  "stepikOne",
  "stepikMax",
  "yandexLink",
  "yandexOne",
  "yandexMax",
  "alarmTime",
  "alarmLen",
  "alarmAfter",
  "videoOne",
  "videoMax",
  "extensionOne",
  "extensionMax",
  "programOne",
  "programMax",
};

void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Fahrenheit 404. Page burned...");
  // request->send(404, "text/plain", "404 градуса по Фаренгейту. Страница не найдена, видимо она сгорела...");
}

String processor(const String& var){
  DynamicJsonDocument config(2048);
	File file = LittleFS.open("/json/config.json", "r");
  deserializeJson(config, file);
  if (var == "custom1"){
    DynamicJsonDocument customjson(512);
    file = LittleFS.open("/json/custom.json", "r");
    deserializeJson(customjson, file);
    String s = "";
    for (JsonPair kv : customjson.as<JsonObject>()) {
      s += String("<option ${i[0].includes(\"") + kv.key().c_str() + "\") ? \"selected\" : \"\"} value=\"" + kv.key().c_str() + "\">" + kv.key().c_str() + "</option>";
    }
    return s;
    // <option ${i[0].includes(12) ? "selected" : ""} value="12">Anki Add</option>
  }
  if (var == "custom2"){
    DynamicJsonDocument customjson(512);
    file = LittleFS.open("/json/custom.json", "r");
    deserializeJson(customjson, file);
    String s = "";
    for (JsonPair kv : customjson.as<JsonObject>()) {
      s += String("<option value=\"") + kv.key().c_str() + "\">" + kv.key().c_str() + "</option>";
    }
    return s;
    // <option value="12">Anki Add</option>
  }
  if (var == "battery"){
    #if defined(ESP8266)
      return String(int(ESP.getVcc() / 40));
    #elif defined(ESP32)
      // https://randomnerdtutorials.com/power-esp32-esp8266-solar-panels-battery-level-monitoring/
      return String(constrain(map(analogRead(V_PIN), 3000.0f, 4000.0f, 0, 100), 1, 100));
      // return String(analogRead(V_PIN)) + " " + String(map(analogRead(V_PIN), 0.0f, 4095.0f, 0, 100)) + String(" STDBY ") + digitalRead(STDBY_PIN) + " " + analogRead(STDBY_PIN) + String(" CHRG ") + digitalRead(CHRG_PIN) + " " + analogRead(CHRG_PIN);
    #endif
  }
  #if defined(ESP32)
    if (var == "charge"){
      return String(!digitalRead(STDBY_PIN) ? "Standby" : (!digitalRead(CHRG_PIN) ? "Charge" : ""));
    }
  #endif
  if (var == "about"){
    String about = "";
    about += String("Прошивка: ") + __DATE__ " " __TIME__ + "<br>\n"; // Firmware
    about += "Микроконтроллер: "; // Board
    #if defined(ESP8266)
      // https://arduino-esp8266.readthedocs.io/en/latest/libraries.html#esp-specific-apis
      FSInfo fs_info;
      LittleFS.info(fs_info);
      about += String()
      + "ESP8266" + "<br>\n"
      + "Объём памяти: " + ESP.getFlashChipSize() / 1024 + " KB<br>\n" // Flash Chip Size
      + "Частота CPU: " + ESP.getCpuFreqMHz() + " MHz<br>\n" // CPU frequency
      + "Размер прошивки: " + ESP.getSketchSize() / 1024 + " / " + ESP.getFreeSketchSpace() / 1024 + " KB<br>\n" // Sketch size
      + "Файловая система: " + fs_info.usedBytes / 1024 + " / " + fs_info.totalBytes / 1024 + " KB<br>\n"
      + "Версия ядра: " + ESP.getCoreVersion() + "<br>\n" // Core Version
      + "Версия SDK: " + ESP.getSdkVersion() + "<br>\n" // SDK Version
      + "<br><p class=\"text-muted\">"
      + "Flash Chip ID: " + ESP.getFlashChipId() + "<br>\n"
      + "Flash Chip Real Size: " + ESP.getFlashChipRealSize() / 1024 + " KB<br>\n"
      + "Flash Chip Speed: " + ESP.getFlashChipSpeed() + "<br>\n"
      + "Flash Chip Mode: " + ESP.getFlashChipMode() + "<br>\n"
      + "Heap Fragmentation: " + ESP.getHeapFragmentation() + "<br>\n"
      + "FS Block Size: " + fs_info.blockSize + "<br>\n"
      + "FS Max Open Files: " + fs_info.maxOpenFiles + "<br>\n"
      + "FS Max Path Length: " + fs_info.maxPathLength + "<br>\n"
      + "FS Page Size: " + fs_info.pageSize + "<br>\n"
      + "Sketch MD5: " + ESP.getSketchMD5() + "<br>\n"
      + "</p>";
    #elif defined(ESP32)
      // Попробовать пременить методы из 8266
      about += String("ESP32") + "<br>\n";
      esp_chip_info_t chip_info;
      esp_chip_info(&chip_info);
      about += String("Hardware info: ") + "<br>\n";
      about += String(chip_info.cores) + " cores Wifi " + ((chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "") + ((chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "") + "<br>\n";
      about += String("Silicon revision: ") + chip_info.revision + "<br>\n";
      about += String(spi_flash_get_chip_size()/(1024*1024)) + "MB " + ((chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embeded" : "external") + " flash" + "<br>\n";
      String chipId = String((uint32_t)ESP.getEfuseMac(), HEX);
      chipId.toUpperCase();
      about += String("CPU frequency: ") + getCpuFrequencyMhz() + " MHz<br>\n";
      about += String("APD frequency: ") + getApbFrequency() + "<br>\n";
      // about += String("UART baud rate: ") + uartGetBaudRate(0) + "<br>\n";
      about += String("Chip id: ") + chipId + "<br>\n";
      about += String("WiFi.macAddress: ") + WiFi.macAddress() + "<br>\n";
      about += String("esp_get_free_heap_size: ") + esp_get_free_heap_size() + "<br>\n";
      about += String("esp_get_idf_version: ") + esp_get_idf_version() + "<br>\n";
      about += String("LittleFS.totalBytes(): ") + LittleFS.totalBytes() + "<br>\n";
      about += String("LittleFS.usedBytes(): ") + LittleFS.usedBytes() + "<br>\n";
      about += String("Bracelet touch: ") + touchRead(4) + "<br>\n";
      about += String("Resistor touch: ") + touchRead(27) + "<br>\n";
    #else
      // Они этого не увидят, т.к. даже библиотеки не импортируются
      about += String("NOT ESP8266 or ESP32") + "<br>\n";
    #endif
    return about;
  }
  if (config[var].isNull()){
    return String();
  }
  else{
    return config[var];
  }
}

// Список файлов
// https://github.com/littlefs-project/littlefs/issues/2
// void listAllFilesInDir(String dir_path)
// {
// 	Dir dir = LittleFS.openDir(dir_path);
// 	while(dir.next()) {
// 		if (dir.isFile()) {
// 			// print file names
// 			Serial.print("File: ");
// 			Serial.println(dir_path + dir.fileName());
// 		}
// 		if (dir.isDirectory()) {
// 			// print directory names
// 			Serial.print("Dir: ");
// 			Serial.println(dir_path + dir.fileName() + "/");
// 			// recursive file listing inside new directory
// 			listAllFilesInDir(dir_path + dir.fileName() + "/");
// 		}
// 	}
// }

void web(){
	DynamicJsonDocument config(2048);
	File file = LittleFS.open("/json/config.json", "r");
  deserializeJson(config, file);
  IPAddress Ip(10, 1, 1, 1); // Частный IP-адрес - 192.168.*.*, 10.*.*.*, 172.16.*.*
  IPAddress NMask(255, 255, 255, 0);
  WiFi.softAPConfig(Ip, Ip, NMask);
  const char* name = config["name"].as<const char*>();
  const char* code = config["code"].as<const char*>();
  if (String(name).length() > 3 || String(code).length() > 3){
    WiFi.softAP(name, code);
    // CSS / JS
    #if defined(ESP8266)
      Dir dir = LittleFS.openDir("/static");
      while(dir.next()){
        server.serveStatic(dir.fileName().c_str(), LittleFS, (String("/static/") + dir.fileName()).c_str(), "max-age=86400"); 
      }
    #elif defined(ESP32)
      server.serveStatic("bootstrap.min.css", LittleFS, "/static/bootstrap.min.css", "max-age=86400");
      server.serveStatic("bootstrap.min.js", LittleFS, "/static/bootstrap.min.js", "max-age=86400"); 
    #endif
    server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(LittleFS, "/style.css", "text/css");
    });
    server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(LittleFS, "/script.js", "text/javascript");
    });
    // JSON
    // Endswith
    #if defined(ESP8266)
      dir = LittleFS.openDir("/json");
      while(dir.next()){
        String fileName = dir.fileName();
        server.on((String("/") + fileName).c_str(), HTTP_GET, [fileName](AsyncWebServerRequest *request){
          request->send(LittleFS, (String("/json/") + fileName).c_str(), "application/json");
        });
      }
    #elif defined(ESP32)
      server.on("/config.json", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(LittleFS, "/json/config.json", "application/json");
      });
      server.on("/schedule.json", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(LittleFS, "/json/schedule.json", "application/json");
      });
      server.on("/variables.json", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(LittleFS, "/json/variables.json", "application/json");
      });
      server.on("/wifi.json", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(LittleFS, "/json/wifi.json", "application/json");
      });
      server.on("/custom.json", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(LittleFS, "/json/custom.json", "application/json");
      });
    #endif
    server.on("/log.txt", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(LittleFS, "/log.txt", "text");
    });
    server.on("/reset", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(200, "text/plain", "Перезагрузка...");
      ESP.restart();
    });
    server.on("/off", HTTP_GET, [](AsyncWebServerRequest *request){
      #if defined(ESP8266)
        ESP.deepSleep(0);
      #elif defined(ESP32)
        for (int i = 1023; i > 0; i--){
          setColor(i, 0, 0);
          delay(1);
        }
        request->send(200, "text/plain", "Выключение...");
        esp_deep_sleep_start();
      #endif
    });
    // Pages
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(LittleFS, "/index.html", String(), false, processor);
    });
    // for (const char *i : pages){
    //   server.on((String("/") + i).c_str(), HTTP_GET, [i](AsyncWebServerRequest *request){
    //     request->send(LittleFS, (String("/html/") + i + ".html").c_str(), String(), false, processor);
    //   });
    // }
    #if defined(ESP8266)
      dir = LittleFS.openDir("/html");
      while(dir.next()){
        String fileName = dir.fileName();
        server.on((String("/") + fileName.substring(0, fileName.length() - 5)).c_str(), HTTP_GET, [fileName](AsyncWebServerRequest *request){
          request->send(LittleFS, (String("/html/") + fileName).c_str(), String(), false, processor);
        });
      }
      // Battery (изменить делитель)
      server.on("/get-battery", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send_P(200, "text/plain", String(int(ESP.getVcc() / 40)).c_str());
      });
    #elif defined(ESP32)
      server.on("/get-battery", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send_P(200, "text/plain", String(constrain(map(analogRead(V_PIN), 3000.0f, 4000.0f, 0, 100), 1, 100)).c_str());
      });
      server.on("/get-charge", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send_P(200, "text/plain", String(!digitalRead(STDBY_PIN) ? "Standby" : (!digitalRead(CHRG_PIN) ? "Charge" : "")).c_str());
        // request->send_P(200, "text/plain", (String(analogRead(V_PIN)) + " " + String(map(analogRead(V_PIN), 0.0f, 4095.0f, 0, 100)) + String(" STDBY ") + digitalRead(STDBY_PIN) + " " + analogRead(STDBY_PIN) + String(" CHRG ") + digitalRead(CHRG_PIN) + " " + analogRead(CHRG_PIN)).c_str());
      });
      const char *pages[] = {"about", "alarm", "anki", "battery", "codewars", "custom", "extension", "food", "github", "import", "leetcode", "logs", "main", "program", "schedule", "stepik", "video", "wifi", "yandex"};
      for (const char *page : pages){
        server.on((String("/") + page).c_str(), HTTP_GET, [page](AsyncWebServerRequest *request){
          request->send(LittleFS, (String("/html/") + page + ".html").c_str(), String(), false, processor);
        });
      }
    #endif
    server.on("/log-clear", HTTP_GET, [](AsyncWebServerRequest *request){
      LittleFS.open("/log.txt", "w").close();
    });
    // Get
    server.on("/get", HTTP_GET, [] (AsyncWebServerRequest *request){
      DynamicJsonDocument config(2048);
      File file = LittleFS.open("/json/config.json", "r");
      deserializeJson(config, file);
      for (JsonPair kv : config.as<JsonObject>()){
        if (request->hasParam(kv.key().c_str())){
          config[kv.key().c_str()] = request->getParam(kv.key().c_str())->value();
        }
      }
      // for (const String i: params){
      //   if (request->hasParam(i)){
      //     config[i] = request->getParam(i)->value();
      //   }
      // }
      file = LittleFS.open("/json/config.json", "w");
      serializeJsonPretty(config, file);
    });
    // Post Schedule
    AsyncCallbackJsonWebHandler *handlerSchedule = new AsyncCallbackJsonWebHandler("/post-schedule", [](AsyncWebServerRequest *request, JsonVariant &json) {
      Serial.println(json.as<String>());
      File file = LittleFS.open("/json/schedule.json", "w");
      serializeJson(json, file);
    });
    server.addHandler(handlerSchedule);
    // Post Settings
    AsyncCallbackJsonWebHandler *handlerSettings = new AsyncCallbackJsonWebHandler("/post-config", [](AsyncWebServerRequest *request, JsonVariant &json) {
      Serial.println(json.as<String>());
      File file = LittleFS.open("/json/config.json", "w");
      serializeJsonPretty(json, file);
    });
    server.addHandler(handlerSettings);
    // Post Custom
    AsyncCallbackJsonWebHandler *handlerCustom = new AsyncCallbackJsonWebHandler("/post-custom", [](AsyncWebServerRequest *request, JsonVariant &json) {
      Serial.println(json.as<String>());
      File file = LittleFS.open("/json/custom.json", "w");
      serializeJsonPretty(json, file);
    });
    server.addHandler(handlerCustom);
    // Post WiFi
    AsyncCallbackJsonWebHandler *handlerWifi = new AsyncCallbackJsonWebHandler("/post-wifi", [](AsyncWebServerRequest *request, JsonVariant &json) {
      Serial.println(json.as<String>());
      File file = LittleFS.open("/json/wifi.json", "w");
      serializeJson(json, file);
    });
    server.addHandler(handlerWifi);
    server.onNotFound(notFound);
  }
  else {
    WiFi.softAP("Band", "12345678");
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(200, "text/html", "Filesystem error. Update filesystem - <a href=\"/update\">/update</a>");
    });
  }
  AsyncElegantOTA.begin(&server); // OTA
  server.begin();
  #if defined(ESP32)
    pinMode(STDBY_PIN, INPUT_PULLUP);
    pinMode(CHRG_PIN, INPUT_PULLUP);
  #endif

  while(true){
    #if defined(ESP8266)
      if (ESP.getVcc() < 3100) {
        Serial.println(String("Voltage-") + ESP.getVcc() + ". LOW BATTERY. Sleep...");
        ESP.deepSleep(0);
      }
      for(int i = 0; i < 1023; i++) {
        analogWrite(L_PIN, i);
        delay(10);
      }
      for(int i = 1023; i > 0; i--) {
        analogWrite(L_PIN, i);
        delay(10);
      }
    #elif defined(ESP32)
      if (analogRead(V_PIN) < 3000){
        for (int i = 1023; i > 0; i--){
          setColor(i, 0, 0);
          delay(1);
        }
        esp_deep_sleep_start();
      }

      // if (digitalRead()){
      //   setColor(255, 0, 0);
      // }
      // else if (digitalRead()){
      //   setColor(0, 255, 0);
      // }
      // else{
      //   setColor(0, 0, 255);
      // }

      red = red + red_direction;
      green = green + green_direction;
      blue = blue + blue_direction;
      if (red >= 1023 || red <= 0){
        red_direction = red_direction * -1;
      }
      if (green >= 1023 || green <= 0){
        green_direction = green_direction * -1;
      }
      if (blue >= 1023 || blue <= 0){
        blue_direction = blue_direction * -1;
      }
      setColor(red, green, blue);
      delay(2);

      if (digitalRead(19) == 0 && digitalRead(0) == 0){
        for (int i = 1023; i > 0; i--){
          setColor(i, 0, 0);
          delay(1);
        }
        esp_deep_sleep_start();
      }
    #endif
  }
}