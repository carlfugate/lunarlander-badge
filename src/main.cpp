#include "FeatureFlags.h"
#include "Includes.h" // the Includes file with the whole list there
#include "QA/ntp_sync.h" // Include the NTPManager class
#include "QA/QA_Test_Sequence.h"
#include "Hardware/Screen_Module.h"
#include "Hardware/RotaryEncoder_Module.h"
#include "Hardware/BadgeRegistration.h" // For badge registration

#define TFT_HOR_RES   240
#define TFT_VER_RES   320
#define TFT_ROTATION  LV_DISPLAY_ROTATION_270

/*LVGL draw into this buffer, 1/10 screen size usually works well. The size is in bytes*/
#define DRAW_BUF_SIZE (TFT_HOR_RES * TFT_VER_RES / 10 * (LV_COLOR_DEPTH / 16))
uint32_t draw_buf[DRAW_BUF_SIZE / 4];

uint32_t badge_boot_ms = 0;

// Create MAX17048 object
Adafruit_MAX17048 max17048;
bool max17048_available = false;  // Track if MAX17048 is actually available

// Using HSPI as it's unused
SPIClass hspi = SPIClass(HSPI);

// Array of LED pins
int ledPins[] = {LED_PIN_1, LED_PIN_2, LED_PIN_3, LED_PIN_4, LED_PIN_5, LED_PIN_6};
int numLeds = sizeof(ledPins) / sizeof(ledPins[0]);
bool ledStatus = false;

// Define color array and number of colors for NeoPixel
uint32_t colors[] = {
    statusLED.Color(255, 0, 0),   // Red
    statusLED.Color(0, 255, 0),   // Green
    statusLED.Color(0, 0, 255)    // Blue
};
int numColors = sizeof(colors) / sizeof(colors[0]);

// use Arduinos millis() as tick source
static uint32_t my_tick(void)
{
    return millis();
}

// Read the touchpad
void my_touchpad_read( lv_indev_t * indev, lv_indev_data_t * data )
{
    lv_display_rotation_t rotation = lv_disp_get_rotation(lv_disp_get_default());
    uint16_t x, y;
    if (Screen_Module_GetTouch(x, y)) {
        data->state = LV_INDEV_STATE_PRESSED;
        data->point.x = x;
        data->point.y = y;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

NTPManager ntpManager("pool.ntp.org", "UTC"); // Always use UTC for NTP and timestamps

void setup() {
    Serial.begin(115200);

    while (!Serial);
    
    // Suppress I2C errors for prototype hardware with unreliable connections
    esp_log_level_set("*", ESP_LOG_WARN);  // Set all logs to warning or higher
    esp_log_level_set("Wire", ESP_LOG_NONE);  // Suppress Wire library
    esp_log_level_set("I2C", ESP_LOG_NONE);   // Suppress I2C driver
    esp_log_level_set("i2c", ESP_LOG_NONE);   // Suppress i2c (lowercase)

   // Initiating the LVGL library
   lv_init();

   /*Set a tick source so that LVGL will know how much time elapsed. */
   lv_tick_set_cb(my_tick);

   /* register print function for debugging */
   #if LV_USE_LOG != 0
       lv_log_register_print_cb( my_print );
   #endif

   lv_display_t * disp;
   /*TFT_eSPI can be enabled lv_conf.h to initialize the display in a simple way*/
   disp = lv_tft_espi_create(TFT_HOR_RES, TFT_VER_RES, draw_buf, sizeof(draw_buf));

   /*Initialize the (dummy) input device driver*/
   lv_indev_t * indev = lv_indev_create();
   lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER); /*Touchpad should have POINTER type*/
   lv_indev_set_read_cb(indev, my_touchpad_read);
   
   lv_display_set_rotation(disp, TFT_ROTATION);
   
    setScreenRotation(TFT_ROTATION);

    // Initialize each LED pin as an output
    for (int i = 0; i < numLeds; i++) {
        pinMode(ledPins[i], OUTPUT);
    }

    // Initialize I2C for MAX17048
    Wire.begin(TOUCH_I2C_SDA, TOUCH_I2C_SCL);
    Wire.setClock(100000); // Set to standard 100kHz I2C speed for stability
    delay(10); // Minimal I2C stabilization

    // Try to initialize MAX17048 with minimal retry delay
    for (int i = 0; i < 2; i++) {
        if (max17048.begin(&Wire)) {
            max17048_available = true;
            break;
        }
        delay(10);
    }
    
    if (max17048_available) {
        Serial.println("MAX17048 found!");
    } else {
        Serial.println("MAX17048 not detected - battery monitoring disabled.");
        Serial.println("(This is normal for prototype hardware with incomplete connections)");
    }

    // Initialize the touch screen module
    Screen_Module_InitTouch();

    // Initialize buttons with pull-up resistors
    pinMode(BUTTON_ENTER_PIN, INPUT_PULLUP);
    pinMode(BUTTON_BACK_PIN, INPUT_PULLUP);
    

    // Check for QA Mode (hold ENTER button during power-on)
    if (digitalRead(BUTTON_ENTER_PIN) == LOW) {
        Serial.println("QA Mode: Hardware Self-Test Starting...");
        runHardwareSelfTest();
        // Optionally halt or reset after test
        while (1) { delay(1000); }
    }

    // Display ESP32 information on startup
    printDeviceInfo();

    // Initialize the NeoPixels LED
    initStatusLED();    // Initialize NeoPixels on GPIO18
    initNeoPixels();

    // Initialize SPI for SD card with custom pins
    hspi.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
    
    // Intializing the screen
    // ft6336u.begin(); // Removed, now handled by Screen_Module_InitTouch()
    
    // Initialize SD card
    if (SD.begin(SD_CS, hspi)) {
        Serial.println("SD card initialized successfully.");
        if (SD.cardSize() > 0) {
            Serial.print("SD Card Size: ");
            Serial.print(SD.cardSize() / (1024 * 1024));
            Serial.println(" MB");
        } else {
            Serial.println("SD card detected but unable to determine size.");
        }
    } else {
        Serial.println("No SD card detected. Please insert an SD card.");
    }

    // Initialize rotary encoder
    RotaryEncoder_Module_Init();

    // Initialize boot button
    pinMode(BOOT_BUTTON_PIN, INPUT_PULLUP);
    
    // Initialize buzzer pin
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW);

    // --- Themed boot display ---
    displayBootTerminalHeader();

#if FF_SKIP_BOOT_CHECKS
    // Fast boot — skip WiFi, NTP, and registration
    displayBootStatusLine("> Comms link ...........  [OK]");
    displayBootStatusLine("> Power systems ........  [OK]");
    displayBootStatusLine("> Sensors ..............  [OK]");
    displayBootStatusLine("> Cargo bay ............  [OK]");
    displayBootStatusLine("> Mission control ......  [OK]");
    displayBootStatusLine("> Time sync ............  [OK]");
    displayBootComplete(true);
#else
    tft.println("> Comms link ...........");
    scanWiFiNetworks();
    bool wifiConnected = connectToWiFiWithResult();
    displayBootStatusLine(wifiConnected ? "  [OK]" : "  [FAIL]", wifiConnected);

    char buf[64];
    snprintf(buf, sizeof(buf), "> Power systems ........  v%s", BADGE_VERSION);
    displayBootStatusLine(buf);

    snprintf(buf, sizeof(buf), "> Sensors ..............  %s", max17048_available ? "[OK]" : "[FAIL]");
    displayBootStatusLine(buf, max17048_available);

    snprintf(buf, sizeof(buf), "> Cargo bay ............  %s", SD.cardSize() > 0 ? "[OK]" : "[FAIL]");
    displayBootStatusLine(buf, SD.cardSize() > 0);

    tft.println("> Mission control ......");
    int regResult = handleBadgeRegistrationWithResult();
    bool regOk = (regResult == 200 || regResult == 201);
    displayBootStatusLine(regOk ? "  [OK]" : "  [FAIL]", regOk);

    tft.println("> Time sync ............");
    bool ntpOk = ntpManager.init();
    displayBootStatusLine(ntpOk ? "  [OK]" : "  [SKIP]", ntpOk);

    displayBootComplete(wifiConnected && max17048_available && regOk);
#endif
    delay(1500);
    // --- End themed boot display ---

    badge_boot_ms = millis();

    // Remove all old boot/status messages

    // Initialize Main Menu
    create_main_menu();  // Show the OTA update check
}

void loop() {
    lv_timer_handler();
    delay(5);
}