#include <Arduino.h>
#include <LittleFS.h>
#include "web.h"
#include "run.h"
#include "pins.h"

#if defined(ESP32)
	#include <WiFi.h>
  #include "soc/soc.h" // brownout
  #include "soc/rtc_cntl_reg.h" // brownout

  // #include <freertos/FreeRTOS.h>
  // #include <freertos/task.h>
  // #include <driver/rtc_io.h>
  // #include <driver/gpio.h>
  // #include <esp_sleep.h>
  // #include "esp32/ulp.h"
  // extern "C" {
  //   #include <esp32/ulp.h>
  // }
  // #include "ulp_lp_core.h"
  // #include "esp_err.h"
  #include <freertos/FreeRTOS.h>
  #include <freertos/task.h>
  #include <driver/rtc_io.h>
  #include <driver/gpio.h>
  #include <driver/gpio.h>
  #include <esp_sleep.h>
  #include <esp_log.h>
  #include <esp32/ulp.h>
  #include "ulp_main.h"
#elif defined(ESP8266)
	#include <ESP8266WiFi.h>
	ADC_MODE(ADC_VCC);
#endif

// volatile uint32_t* counter = RTC_SLOW_MEM;

// extern const uint8_t ulp_blink_bin_start[] asm("_binary_ulp_blink_bin_start");
// extern const uint8_t ulp_blink_bin_end[] asm("_binary_ulp_blink_bin_end");

// extern "C"
// {
//   void app_main();
// }

void setup(){
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
  #endif

  // /* Initialize RTC GPIO */
  // rtc_gpio_init(GPIO_NUM_2);
  // rtc_gpio_set_direction(GPIO_NUM_2, RTC_GPIO_MODE_OUTPUT_ONLY);
  // rtc_gpio_set_level(GPIO_NUM_2, 0);

  // /* Load ULP program */
  // esp_err_t err = ulp_load_binary(0, ulp_blink_bin_start,
  //                                 (ulp_blink_bin_end - ulp_blink_bin_start) / sizeof(uint32_t));
  // if (err != ESP_OK) {
  //     printf("Failed to load ULP binary: %s\n", esp_err_to_name(err));
  //     return;
  // }

  // /* Set ULP wake-up period (500ms) */
  // ulp_set_wakeup_period(0, 500000); /* 500ms in microseconds */

  // /* Start ULP program */
  // err = ulp_run(0);
  // if (err != ESP_OK) {
  //     printf("Failed to start ULP: %s\n", esp_err_to_name(err));
  //     return;
  // }

  // /* Enter deep sleep */
  // printf("Entering deep sleep\n");
  // esp_deep_sleep_start();

	// Настройки общие для web и run
	Serial.begin(115200); // 9600
	Serial.println();

	#if defined(ESP8266)
		Serial.println(String("Volage-") + ESP.getVcc());
		if (ESP.getVcc() < 3100) { // 3100
			Serial.println("LOW BATTERY. Sleep...");
			ESP.deepSleep(0);
		}
	#endif

	LittleFS.begin();

  #if defined(ESP32)
    // digitalWrite(B_PIN, 0);
    ledcWrite(3, 0);
  #elif defined(ESP8266)
    pinMode(L_PIN, OUTPUT); digitalWrite(L_PIN, 1);
	  pinMode(H_PIN, OUTPUT);
  #endif

	pinMode(BTN_PIN, INPUT_PULLUP); // 5
	if (digitalRead(BTN_PIN) == 0){ // digitalRead(5) == 0
		Serial.println("Запуск Web-интерфейса...");
		web();
	}
	else {
		Serial.println("Запуск браслета...");
		run();
	}
}

void loop(){}

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/rtc_io.h>
#include <driver/gpio.h>
#include <driver/gpio.h>
#include <esp_sleep.h>
#include <esp_log.h>
#include <esp32/ulp.h>
#include "ulp_main.h"

const gpio_num_t BUTTON_PIN = GPIO_NUM_0;
const gpio_num_t LED_PIN = GPIO_NUM_2;

extern "C"
{
  void app_main();
}

extern const uint8_t ulp_main_bin_start[] asm("_binary_ulp_main_bin_start");
extern const uint8_t ulp_main_bin_end[] asm("_binary_ulp_main_bin_end");

void app_main()
{
  // turn on the LED
  // gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
  // gpio_set_level(LED_PIN, 1);

  initArduino();
  setup();

  // did we wake up from the ULP?
  auto wake_cause = esp_sleep_get_wakeup_cause();
  if (wake_cause != ESP_SLEEP_WAKEUP_ULP)
  {
    // we were woken up for some other reason or it's a fresh boot
    // setup the ULP program
    ESP_LOGI("main", "Loading the ULP binary");
    esp_err_t err = ulp_load_binary(0, ulp_main_bin_start, (ulp_main_bin_end - ulp_main_bin_start) / sizeof(uint32_t));
    ESP_ERROR_CHECK(err);
  }
  else
  {
    // we were woken by the ULP
    ESP_LOGI("main", "Woken up by the ULP");
  }
  gpio_set_direction(BUTTON_PIN, GPIO_MODE_INPUT);
  gpio_set_pull_mode(BUTTON_PIN, GPIO_PULLUP_ONLY);
  // wait for the button to be pushed
  while (gpio_get_level(BUTTON_PIN) == 1)
  {
    vTaskDelay(pdMS_TO_TICKS(100));
  }
  // setup the RCP pin
  rtc_gpio_init(GPIO_NUM_2);
  rtc_gpio_set_direction(GPIO_NUM_2, RTC_GPIO_MODE_OUTPUT_ONLY);
  // goto sleep
  ESP_LOGI("main", "Going to sleep");
  esp_err_t err = esp_sleep_enable_ulp_wakeup();
  ESP_ERROR_CHECK(err);
  err = ulp_set_wakeup_period(0, 500000); // 0.5 seconds
  ESP_ERROR_CHECK(err);
  err = ulp_run(&ulp_entry - RTC_SLOW_MEM);
  ESP_ERROR_CHECK(err);
  esp_deep_sleep_start();
}