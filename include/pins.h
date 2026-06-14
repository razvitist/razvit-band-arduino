#ifndef PINS_H
#define PINS_H

#if defined(ESP32)
// #define R_PIN 22
// #define G_PIN 21
// #define B_PIN 23
// #define STDBY_PIN 16
// #define CHRG_PIN 17
// #define V_PIN 39
// #define BTN_PIN 19
// #define H1_PIN 25
// #define H2_PIN 26
// #define H3_PIN 27
// #define H4_PIN 32
// #define H5_PIN 33
// #define H6_PIN 12
// #define H7_PIN 14
#define R_PIN 26
#define G_PIN 25
#define B_PIN 33
#define STDBY_PIN 4
#define CHRG_PIN 32
#define V_PIN 39
#define BTN_PIN 19
#define H_PIN 27
#elif defined(ESP8266)
#define L_PIN 2
#define H_PIN 14
#define BTN_PIN 5
#endif

#endif