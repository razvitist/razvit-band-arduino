#include <ArduinoJson.h>
#include <LittleFS.h>

#if defined(ESP32)
	#include <WiFi.h>
	#include <HTTPClient.h>
	#include <WiFiClientSecure.h>
#elif defined(ESP8266)
	#include <ESP8266WiFi.h>
	#include <ESP8266HTTPClient.h>
#endif

StaticJsonDocument<512> var;
DynamicJsonDocument config(2048);

static boolean isLog = 0;
static String printplus = "";
static void print(String s){
	Serial.print(s);
	if (isLog){
		printplus += s;
	}
}
static void println(String s = ""){
	Serial.println(s);
	if (isLog){
		File file = LittleFS.open("/log.txt", "a");
		file.println(String("         ") + printplus + s);
		file.close();
		printplus = "";

		if (file.size() > 200 * 1024){
			LittleFS.open("/log.txt", "w").close();
		}
	}
}

void writeVar(){
	File file = LittleFS.open("/json/variables.json", "w");
  serializeJsonPretty(var, file);
}

void sleep(int e, int minutes){
	print("Sleep ");
	if (e - minutes <= 60){ // 200 изменено для работы пищалки// Есть ограничение на время сна
		println(String(e - minutes + 1));
    ESP.deepSleep((e - minutes + 1) * 60e6 + 1e6); // + одна минута всегда
  }
  else{
		// StaticJsonDocument<200> sleep;
		// File file = LittleFS.open("/sleep.json", "w");
		// sleep["sl"] = e - minutes - 200;
		// Serial.println(sleep.as<String>());
  	// serializeJsonPretty(sleep, file);

		// StaticJsonDocument<200> sleepF;
		// File file = LittleFS.open("/sleep.json", "r");
		// deserializeJson(sleepF, file);
		// int sleepp = sleepF["sl"].as<int>();
		// Serial.println(sleepF.as<String>());
		// Serial.println(sleepp);
		println("60"); // 200
		#if defined(ESP32)
			esp_sleep_enable_timer_wakeup(60 * 60e6);
			esp_deep_sleep_start();
		#elif defined(ESP8266)
    	ESP.deepSleep(60 * 60e6); // 1.2e10 // 200 мин.
		#endif
  }
}

String httpRequest(const char *host, String path, const uint16_t port){
	if (port == 443){
		#if defined(ESP32)
			WiFiClientSecure client;
			HTTPClient http;
		#elif defined(ESP8266)
			HTTPClient http;
			BearSSL::WiFiClientSecure client;
		#endif
		client.setInsecure();
		http.begin(client, host, port, path.c_str());
		http.GET();
		return http.getString();
	}
	if (port == 80){
		WiFiClient client;
		HTTPClient http;
		http.begin(client, host, port, path.c_str());
		http.GET();
		return http.getString();
	}
	return String();

	// НЕ РАБОТАЕТ 80
	// HTTPClient http;
	// if (port == 443){
	// 	#if defined(ESP32)
	// 		WiFiClientSecure client;
	// 	#elif defined(ESP8266)
	// 		BearSSL::WiFiClientSecure client;
	// 	#endif
	// 	client.setInsecure();
	// 	http.begin(client, host, port, path.c_str());
	// 	http.GET();
	// 	// Serial.println(http.getString());
	// 	// client.stop(); // ?
	// 	return http.getString();

	// 	// HEAD try
	// 	// const char * headerKeys[] = {"content-length"} ;
	// 	// const size_t numberOfHeaders = 1;
	// 	// HTTPClient http;
	// 	// BearSSL::WiFiClientSecure client;
	// 	// client.setInsecure();
	// 	// http.begin(client, host, port, path.c_str());
	// 	// http.collectHeaders(headerKeys, numberOfHeaders);
	// 	// http.GET();
	// 	// println(http.header("content-length"));
	// 	// // Serial.println(http.getString());
	// 	// // client.stop(); // ?
	// 	// println(http.getString());
	// 	// return http.getString();
	// }
	// else if (port == 80){
	// 	WiFiClient client;
	// 	http.begin(client, host, port, path.c_str());
	// 	http.GET();
	// 	return http.getString();
	// }
	// return String();
}

int getpin(String pin){
	// band 80 / 443
	// 443 - OK
	// 80 - moved permanently
	// 80 - нужно отключить редирект с http на https
	// String("/get/andrei/") + pin
	// 2 apr - 80 даёт ошибку
	return httpRequest("band.razvit.org", String("/get/") + config["nickname"].as<String>() + "/" + pin, 80).toInt();
}

void setpin(String pin, int v){
	httpRequest("band.razvit.org", (String("/set/") + config["nickname"].as<String>() + "/" + pin + "/" + v), 80); 
}

void food(int minutes){
	StaticJsonDocument<64> http;
	deserializeJson(http, httpRequest("blynk-cloud.com", String("/") + config["blynk"].as<String>() + "/get/V2", 80));
	int foodTime = http[1];
	// int foodTime = getpin("food");
	int foodLen = config["foodLen"];
	// int foodNext = config["foodNext"];
	if (minutes - foodTime < foodLen && minutes - foodTime >= 0){ // Если еда, то вырубать на 30 мин.
		println("Food");
		ESP.deepSleep((foodTime - minutes + foodLen) * 60e6 + 1e6);
	}
	else { // Очистить, если прошло больше часа
		httpRequest("blynk-cloud.com", (String("/") + config["blynk"].as<String>() + "/update/V2?value=0&value=0"), 80);
	}
}

void anki(){
	int anki = getpin("anki");
	if (anki > 0){
		setpin("anki", 0);
		print(String(min(config["ankiOne"].as<int>() * anki, config["ankiMax"].as<int>()))); println(" s");
		ESP.deepSleep(min(config["ankiOne"].as<int>() * anki, config["ankiMax"].as<int>()) * 1e6 + 1e6);
	}
}

void stepik(int wday){
	StaticJsonDocument<256> http;
	deserializeJson(http, httpRequest("stepik.org", (String("/api/user-activities/") + config["stepikID"].as<String>()), 443));
	int stepikNew = http["user-activities"][0]["pins"][0];
	if (var["stepikDay"] != wday){
		var["stepikDay"] = wday;
		var["stepik"] = 0;
		writeVar();
	}
	int stepikOld = var["stepik"];
	if (stepikNew - stepikOld > 0){
		var["stepik"] = stepikNew;
		writeVar();
		println(String(min((stepikNew - stepikOld) * config["stepikOne"].as<int>(), config["stepikMax"].as<int>())));
		ESP.deepSleep(min((stepikNew - stepikOld) * config["stepikOne"].as<int>(), config["stepikMax"].as<int>()) * 60e6 + 1e6);
	}
}

void codewars(){
	StaticJsonDocument<200> http;
	deserializeJson(http, httpRequest("www.codewars.com", String("/api/v1/users/") + config["codewarsName"].as<String>(), 443));
	int codewarsNew = http["honor"];
	int codewarsOld = var["codewars"];
	var["codewars"] = codewarsNew;
	writeVar();
	if (codewarsNew - codewarsOld > 0){
		println(String(min(config["codewarsOne"].as<int>() * (codewarsNew - codewarsOld), config["codewarsMax"].as<int>())));
		ESP.deepSleep(min(config["codewarsOne"].as<int>() * (codewarsNew - codewarsOld), config["codewarsMax"].as<int>()) * 60e6 + 1e6);
	}
}

void leetcode(){
	StaticJsonDocument<400> http;
	deserializeJson(http, httpRequest("leetcode-stats-api.herokuapp.com", String("/") + config["leetcodeName"].as<String>(), 443));
	int leetcodeEasyNew = http["easySolved"];
	int leetcodeMediumNew = http["mediumSolved"];
	int leetcodeHardNew = http["hardSolved"];
	int leetcodeEasy = var["leetcodeEasy"];
	int leetcodeMedium = var["leetcodeMedium"];
	int leetcodeHard = var["leetcodeHard"];
	var["leetcodeEasy"] = leetcodeEasyNew;
	var["leetcodeMedium"] = leetcodeMediumNew;
	var["leetcodeHard"] = leetcodeHardNew;
	writeVar();
	if (leetcodeEasy < leetcodeEasyNew || leetcodeMedium < leetcodeMediumNew || leetcodeHard < leetcodeHardNew){
		println(String(min((leetcodeEasyNew - leetcodeEasy) * config["leetcodeEasy"].as<int>() + (leetcodeMediumNew - leetcodeMedium) * config["leetcodeMedium"].as<int>() + (leetcodeHardNew - leetcodeHard) * config["leetcodeHard"].as<int>(), config["leetcodeMax"].as<int>())));
		ESP.deepSleep(min((leetcodeEasyNew - leetcodeEasy) * config["leetcodeEasy"].as<int>() + (leetcodeMediumNew - leetcodeMedium) * config["leetcodeMedium"].as<int>() + (leetcodeHardNew - leetcodeHard) * config["leetcodeHard"].as<int>(), config["leetcodeMax"].as<int>()) * 60e6 + 1e6);
	}
}

void github(){
	StaticJsonDocument<400> http;
	String link = config["githubAddress"];
	link.replace("https://github.com/", "");
	link.replace("github.com/", "");
	link.replace(".git", "");
	deserializeJson(http, httpRequest("api.github.com", String("/repos/") + link + "/languages", 443));
	int githubOld = var["github"];
	int githubNew = 0;
	for (JsonPair key : http.as<JsonObject>()){
		githubNew += http[key.key().c_str()].as<int>();
	}
	var["github"] = githubNew;
	writeVar();
	if (githubNew - githubOld > 0){
		print(String(min((githubNew - githubOld) * config["githubOne"].as<int>(), config["githubMax"].as<int>()))); println(" s");
		ESP.deepSleep(min((githubNew - githubOld) * config["githubOne"].as<int>(), config["githubMax"].as<int>()) * 1e6 + 1e6);
	}
}

void yandex(){
	// StaticJsonDocument<32> http;
	// String link = config["yandexLink"];
	// link.replace(":", "%3A");
	// link.replace("/", "%2F");
	// deserializeJson(http, httpRequest("cloud-api.yandex.net", String("/v1/disk/public/resources?public_key=") + link + "&fields=size", 443));
	// int yandexNew = http["size"];
	// int yandexOld = var["yandex"];
	// if (yandexNew - yandexOld > 0){
	// 	var["yandex"] = yandexNew;
	// 	writeVar();
	// 	print(String(min((yandexNew - yandexOld) * config["yandexOne"].as<int>(), config["yandexMax"].as<int>()))); println(" s");
	// 	ESP.deepSleep(min((yandexNew - yandexOld) * config["yandexOne"].as<int>(), config["yandexMax"].as<int>()) * 1e6 + 1e6);
	// }

	// Google + extension
	int yandexNew = getpin("yandex");
	int yandexOld = var["yandex"];
	var["yandex"] = yandexNew;
	writeVar();
	if (yandexNew - yandexOld > 0){
		print(String(min((yandexNew - yandexOld) * config["yandexOne"].as<int>(), config["yandexMax"].as<int>()))); println(" s");
		ESP.deepSleep(min((yandexNew - yandexOld) * config["yandexOne"].as<int>(), config["yandexMax"].as<int>()) * 1e6 + 1e6);
	}

	// deserializeJson(http, httpRequest("docs.google.com", "/feeds/download/documents/export/Export?id=1xdmmF1hzXMnKbkVUxvqldAZFYzeDdkrni4DuETMrWUk&exportFormat=txt", 443));
}

void video(){
	int vNew = getpin("video");
	int vOld = var["video"];
	var["video"] = vNew;
	writeVar();
	if (vNew - vOld > 0){
		print(String(min(config["videoOne"].as<int>() * (vNew - vOld), config["videoMax"].as<int>()))); println(" s");
		ESP.deepSleep(min(config["videoOne"].as<int>() * (vNew - vOld), config["videoMax"].as<int>()) * 1e6);
	}
}

void extension(){ // Такое же как video
	int vNew = getpin("extension");
	int vOld = var["extension"];
	var["extension"] = vNew;
	writeVar();
	if (vNew - vOld > 0){
		print(String(min(config["extensionOne"].as<int>() * (vNew - vOld), config["extensionMax"].as<int>()))); println(" s");
		ESP.deepSleep(min(config["extensionOne"].as<int>() * (vNew - vOld), config["extensionMax"].as<int>()) * 1e6);
	}
}

void program(){ // Такое же как video
	int vNew = getpin("program");
	int vOld = var["program"];
	var["program"] = vNew;
	writeVar();
	if (vNew - vOld > 0){
		print(String(min(config["programOne"].as<int>() * (vNew - vOld), config["programMax"].as<int>()))); println(" s");
		ESP.deepSleep(min(config["programOne"].as<int>() * (vNew - vOld), config["programMax"].as<int>()) * 1e6);
	}
}

void extensionOld(String name, String request, boolean isVideo, boolean isSec=false){
	if (isVideo){
		// int VNew = getpin(name + "V");
		int VNew = getpin(request);
		int VOld = var[name + "V"];
		var[name + "V"] = VNew;
		writeVar();
		if (VNew - VOld > 0){
			print(String(min(config[name + "VOne"].as<int>() * (VNew - VOld), config[name + "VMax"].as<int>()))); println(" s");
			ESP.deepSleep(min(config[name + "VOne"].as<int>() * (VNew - VOld), config[name + "VMax"].as<int>()) * 1e6 + 1e6);
		}
	}
	else { // ПОЧИНИТЬ ПОТОМ
		int New = getpin(request);
		int Old = var[name];
		var[name] = New;
		writeVar();
		if (New - Old > 0){
			println(String(min(config[name + "One"].as<int>() * (New - Old), config[name + "Max"].as<int>())) + (isSec ? " s" : ""));
			ESP.deepSleep(min(config[name + "One"].as<int>() * (New - Old), config[name + "Max"].as<int>()) * (isSec ? 1e6 : 60e6) + 1e6);
	}
	}
}

void alarm(int minutes, int wday){
	int alarmTime = config["alarmTime"].as<String>().substring(0, 2).toInt() * 60 + config["alarmTime"].as<String>().substring(3, 5).toInt();
	int alarmAfter = config["alarmAfter"].as<int>();
	if (minutes >= alarmTime && minutes <= alarmTime + config["alarmLen"].as<int>()){
		setpin("alarmday", wday);
		setpin("alarmtime", minutes);
	}
	if (minutes >= alarmTime && alarmTime + alarmAfter > minutes){
		ESP.deepSleep((alarmTime + alarmAfter - minutes) * 60e6 + 1e6);
	}
}

void api(){
	int result = httpRequest("band.razvit.org", String("/api/") + config["nickname"].as<String>(), 80).toInt();
	if (result > 0){
		print(String(result)); println(" s");
		ESP.deepSleep(result * 1e6 + 1e6);
	}
}

void custom(String name){
	DynamicJsonDocument customjson(512);
	File file = LittleFS.open("/json/custom.json", "r");
  deserializeJson(customjson, file);

	int one = customjson[name][0];
	int maximum = customjson[name][1];

	print("Custom - "); println(name);
	int vNew = getpin(name);
	int vOld = 0;
	if (var.containsKey(name)){
		vOld = var[name];
	}
	var[name] = vNew;
	writeVar();
	if (vNew - vOld > 0){
		print(String(min(one * (vNew - vOld), maximum))); println(" s");
		ESP.deepSleep(min(one * (vNew - vOld), maximum) * 1e6);
	}
}

void modes(String m, int s, int e, int minutes, int wday){
	File file = LittleFS.open("/json/variables.json", "r");
  deserializeJson(var, file);

	file = LittleFS.open("/json/config.json", "r");
  deserializeJson(config, file);

	isLog = config["isLog"].as<int>();

	if (config["alarmLen"].as<int>() != 0){
		alarm(minutes, wday);
	}
	if (config["foodLen"].as<int>() != 0){
		food(minutes);
	}

	// switch не работает со String
	println(m);
	if (m == "sleep")
		sleep(e, minutes);
	else if (m == "api")
		api();
	else if (m == "anki")
		anki();
	else if (m == "stepik")
		stepik(wday);
	else if (m == "codewars")
		codewars();
	else if (m == "leetcode")
		leetcode();
	else if (m == "github")
		github();
	else if (m == "yandex")
		yandex();
	else if (m == "video")
		video();
	else if (m == "extension")
		extension();
	else if (m == "program")
		program();
	else if (m == "ankirated"){
		extensionOld("ankiRated", "ankiRated", false, true); // Не расширение
	}
	else if (m == "ankiadd"){
		extensionOld("ankiAdd", "ankiAdd", false, true); // Не расширение
	}
	else{
		custom(m);
	}
}