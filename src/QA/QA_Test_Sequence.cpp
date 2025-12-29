#include "QA/WiFi_Settings.h"
#include "Hardware/Status_LED.h"
#include "Hardware/NeoPixelControl.h"
#include "Hardware/Screen_Module.h"
#include "pins.h"
#include <SD.h>
#include <FS.h>
#include <WiFi.h>
#include <Arduino.h>

extern TFT_eSPI tft;
extern WiFiNetwork wifiNetworks[];
extern const int numNetworks;

#define TFT_RED    0xF800
#define TFT_GREEN  0x07E0
#define TFT_BLUE   0x001F
#define TFT_BLACK  0x0000
#define TFT_YELLOW 0xFFE0
#ifndef WL_CONNECTED
#define WL_CONNECTED 3
#endif

// Run all hardware self-tests and report results
void runHardwareSelfTest() {
	Serial.println("\n===== QA MODE: HARDWARE SELF-TEST =====");
	// TODO: Implement actual hardware tests below
	// NeoPixel Test
	Serial.print("NeoPixel Test: ");
	bool neopixel_ok = true;
	for (int i = 0; i < 3; ++i) {
		cycleNeoPixelColors();
		delay(400);
	}
	clearNeoPixels();
	Serial.println("Cycled colors (check visually)");

	// Status LED Test
	Serial.print("Status LED Test: ");
	setSolidRed(); delay(300);
	setSolidGreen(); delay(300);
	setSolidBlue(); delay(300);
	setSolidYellow(); delay(300);
	clearLED();
	Serial.println("Red, Green, Blue, Yellow shown (check visually)");
	// Display Test
	Serial.print("Display Test: ");
	tft.fillScreen(TFT_RED); delay(250);
	tft.fillScreen(TFT_GREEN); delay(250);
	tft.fillScreen(TFT_BLUE); delay(250);
	tft.fillScreen(TFT_BLACK); delay(100);
	tft.setTextColor(TFT_YELLOW, TFT_BLACK);
	tft.setTextSize(2);
	tft.setCursor(10, 10);
	tft.println("QA MODE: DISPLAY OK?");
	Serial.println("Red, Green, Blue, Black, and text shown (check visually)");
	// Wait for Enter button press and release to confirm
	Serial.println("Press ENTER to confirm display is OK...");
	while (digitalRead(BUTTON_ENTER_PIN) == HIGH) { delay(10); } // Wait for press
	while (digitalRead(BUTTON_ENTER_PIN) == LOW) { delay(10); }  // Wait for release

	// Buzzer Test
	Serial.print("Buzzer Test: ");
	tone(BUZZER_PIN, 1000, 400); // 1kHz, 400ms
	delay(500);
	noTone(BUZZER_PIN);
	Serial.println("Tone played (listen)");
	// SD Card Test
	Serial.print("SD Card Test: ");
	bool sd_ok = false;
	if (SD.begin(SD_CS)) {
		File testFile = SD.open("/qa_test.txt", FILE_WRITE);
		if (testFile) {
			testFile.println("QA Self-Test OK");
			testFile.close();
			File readFile = SD.open("/qa_test.txt");
			if (readFile) {
				String line = readFile.readStringUntil('\n');
				readFile.close();
				if (line.indexOf("QA Self-Test OK") >= 0) {
					sd_ok = true;
				}
			}
			SD.remove("/qa_test.txt");
		}
	}
	Serial.println(sd_ok ? "PASS" : "FAIL");

	// WiFi Test
	Serial.print("WiFi Test: ");
	bool wifi_ok = false;
	int n = WiFi.scanNetworks();
	if (n > 0) {
		Serial.print("Found networks: "); Serial.println(n);
		String ssid = WiFi.SSID(0);
		String pass = "";
		for (int i = 0; i < numNetworks; ++i) {
			if (ssid == wifiNetworks[i].ssid) {
				pass = wifiNetworks[i].password;
				break;
			}
		}
		WiFi.begin(ssid.c_str(), pass.c_str());
		unsigned long start = millis();
		while (WiFi.status() != WL_CONNECTED && millis() - start < 5000) {
			delay(200);
		}
		if (WiFi.status() == WL_CONNECTED) {
			Serial.print("Connected to: "); Serial.println(ssid);
			Serial.println("PASS");
			wifi_ok = true;
		} else {
			Serial.println("FAIL (could not connect)");
		}
		WiFi.disconnect();
	} else {
		Serial.println("FAIL (no networks found)");
	}

	// Show summary on display
	tft.fillScreen(TFT_BLACK);
	tft.setTextColor(TFT_YELLOW, TFT_BLACK);
	tft.setTextSize(2);
	tft.setCursor(10, 10);
	tft.println("QA MODE RESULTS:");
	tft.setTextSize(1);
	int y = 40;
	tft.setCursor(10, y); tft.print("NeoPixel: "); tft.println("Check Visually"); y += 18;
	tft.setCursor(10, y); tft.print("Status LED: "); tft.println("Check Visually"); y += 18;
	tft.setCursor(10, y); tft.print("Display: "); tft.println("Check Visually"); y += 18;
	tft.setCursor(10, y); tft.print("Buzzer: "); tft.println("Check/Listen"); y += 18;
	tft.setCursor(10, y); tft.print("SD Card: "); tft.println(sd_ok ? "PASS" : "FAIL"); y += 18;
	tft.setCursor(10, y); tft.print("WiFi: "); tft.println(wifi_ok ? "PASS" : "FAIL");
	Serial.println("======================================\n");
}
// Array of LED pins
extern int ledPins[];
extern int numLeds;

// NeoPixel setup
extern Adafruit_NeoPixel statusLED;
extern Adafruit_NeoPixel neoPixels;  // NeoPixel strip on GPIO18
// extern uint32_t colors[]; // Commented out as it's unused after removing functions
// extern int numColors; // Commented out as it's unused after removing functions

// Rotary Encoder variables for manual control
//extern volatile int encoderValue;
// int currentLED = 0; // Commented out as it's unused after removing functions
// int currentColor = 0; // Commented out as it's unused after removing functions
// bool neoPixelOn = true; // Commented out as it's unused after removing functions
