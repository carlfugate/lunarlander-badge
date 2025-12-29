// pins.h
#pragma once

// Define LED pins
#define LED_PIN_1 13
#define LED_PIN_2 14
#define LED_PIN_3 15
#define LED_PIN_4 16
#define LED_PIN_5 17
#define LED_PIN_6 46

// Define NeoPixel pin and number of LEDs
#define STATUS_LED_PIN 21
#define NUM_STATUS_LEDS 1  // Update this if you add more NeoPixels

// Define NeoPixel RGB LED pin and number of LEDs on GPIO18
#define NEOPIXEL_PIN 18
#define NUM_NEOPIXELS 6

// Define Buzzer pin
#define BUZZER_PIN 19

// Define Rotary Encoder pins
#define ENCODER_PIN_A 45
#define ENCODER_PIN_B 48
#define ENCODER_BUTTON_PIN 20

// Define Boot Button pin
#define BOOT_BUTTON_PIN 0

// Define SD Card SPI pins
#define SD_MOSI 35
#define SD_SCK 36
#define SD_MISO 37
#define SD_CS 47

// Define pins for Button_Enter and Button_Back
#define BUTTON_ENTER_PIN 38
#define BUTTON_BACK_PIN 39

// Fuel Guage
#define I2C_SDA_PIN 6
#define I2C_SCL_PIN 7

// Define pins for TFT display
#define TFT_RESET 4   // Define as per your setup
#define TFT_DC    5   // Data/Command pin
#define TFT_LED   6   // Backlight control pin
#define TFT_CS    10  // Chip select for TFT
#define TFT_MOSI  11  // MOSI for TFT
#define TFT_SCK   12  // SCK for TFT

// Define pins for touch interface, if used
#define TOUCH_INT_PIN   GPIO_NUM_7
#define TOUCH_I2C_SDA	GPIO_NUM_8
#define TOUCH_I2C_SCL 	GPIO_NUM_9
#define TOUCH_RST	    GPIO_NUM_3