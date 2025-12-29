#include "QA/schedule.h"
#include <ArduinoJson.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <SD.h>
#include "lvgl.h"

#define API_URL "https://bpbskc25.s3.us-east-1.amazonaws.com"
#define SCHEDULE_FILE "/schedule.json"
#define MAX_ENTRIES 10
#define DEFAULT_LAST_MODIFIED "Thu, 01 Jan 1970 00:00:00 GMT"
#define JSON_DOC_SIZE 2048

ScheduleEntry schedule[MAX_ENTRIES];
int totalEntries = 0;

// Add global to store last HTTP error for display
int lastScheduleHttpCode = 0;
String lastScheduleHttpError;

bool readScheduleFromSD(String &jsonString) {
    if (!SD.begin()) {
        Serial.println("SD Card Mount Failed!");
        return false;
    }
    
    File file = SD.open(SCHEDULE_FILE, "r");
    if (!file) {
        Serial.println("Failed to open schedule file on SD");
        return false;
    }
    
    jsonString = file.readString();
    file.close();
    return true;
}

void saveScheduleToSD(const String &jsonString) {
    File file = SD.open(SCHEDULE_FILE, "w");
    if (!file) {
        Serial.println("Failed to write schedule file to SD");
        return;
    }
    file.print(jsonString);
    file.close();
}

// Helper function to read a file from SD
String readFileFromSD(const char* path) {
    File file = SD.open(path, "r");
    if (!file) {
        Serial.printf("Failed to open file: %s\n", path);
        return "";
    }
    String content = file.readString();
    file.close();
    return content;
}

// Helper function to write a file to SD
void writeFileToSD(const char* path, const String& content) {
    File file = SD.open(path, "w");
    if (!file) {
        Serial.printf("Failed to write file: %s\n", path);
        return;
    }
    file.print(content);
    file.close();
}

// Updated fetchScheduleFromAPI to use If-Modified-Since header
void fetchScheduleFromAPI() {
    WiFiClientSecure client;
    client.setInsecure();
    HTTPClient http;

    Serial.println("Checking if schedule needs to be updated...");

    lastScheduleHttpCode = 0;
    lastScheduleHttpError = "";

    // Read last modified time from SD (or use default)
    String lastModifiedTime = readFileFromSD("/schedule_last_modified.txt");
    if (lastModifiedTime.isEmpty()) {
        lastModifiedTime = DEFAULT_LAST_MODIFIED;
    }

    if (http.begin(client, String(API_URL) + SCHEDULE_FILE)) {
        http.addHeader("If-Modified-Since", lastModifiedTime);
        int httpCode = http.GET();
        lastScheduleHttpCode = httpCode;

        if (httpCode == HTTP_CODE_OK) {
            String payload = http.getString();
            parseSchedule(payload);
            saveScheduleToSD(payload);
            String newLastModified = http.header("Last-Modified");
            if (!newLastModified.isEmpty()) {
                writeFileToSD("/schedule_last_modified.txt", newLastModified);
                Serial.println("Updated last modified time on SD.");
            }
            Serial.println("Schedule downloaded and saved to SD.");
        } else if (httpCode == HTTP_CODE_NOT_MODIFIED) {
            Serial.println("Schedule is up to date, no need to fetch.");
        } else {
            Serial.printf("Failed to fetch API, HTTP code: %d\n", httpCode);
            lastScheduleHttpError = http.getString();
            Serial.println("Error payload: " + lastScheduleHttpError);
        }
        http.end();
    } else {
        lastScheduleHttpCode = -999;
        lastScheduleHttpError = "HTTP begin() failed";
        Serial.println("HTTP begin() failed");
    }
}

// Updated parseSchedule with constant for JSON size
void parseSchedule(const String &jsonString) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, jsonString);

    if (error) {
        Serial.print("JSON parsing failed: ");
        Serial.println(error.c_str());
        return;
    }

    totalEntries = std::min((int)doc.size(), MAX_ENTRIES);
    for (int i = 0; i < totalEntries; i++) {
        schedule[i].speaker = doc[i]["speaker"].as<String>();
        schedule[i].title = doc[i]["title"].as<String>();
        schedule[i].time = doc[i]["time"].as<String>();
        schedule[i].room = doc[i]["room"].as<String>();
        schedule[i].description = doc[i]["description"].as<String>();
    }
}

void loadSchedule() {
    Serial.println("Checking for schedule updates...");
    
    fetchScheduleFromAPI();  // Ensure we always check for updates first

    String jsonString;
    if (readScheduleFromSD(jsonString)) {
        parseSchedule(jsonString);
    } else {
        Serial.println("No schedule data available on SD.");
    }
}

// Updated displaySchedule with list view
void displaySchedule() {
    Serial.println("Displaying Schedule List View...");
    static lv_obj_t *scheduleScreen = NULL;
    if (scheduleScreen) {
        lv_obj_del(scheduleScreen);
        scheduleScreen = NULL;
    }
    scheduleScreen = lv_obj_create(NULL);
    lv_scr_load(scheduleScreen);

    if (totalEntries == 0) {
        String errMsg = "No schedule data found!\nCheck Wi-Fi, SD card, or API.";
        if (lastScheduleHttpCode != 0) {
            errMsg += "\nHTTP code: ";
            errMsg += String(lastScheduleHttpCode);
            if (lastScheduleHttpError.length() > 0) {
                errMsg += "\n" + lastScheduleHttpError;
            }
        }
        lv_obj_t *errLabel = lv_label_create(scheduleScreen);
        lv_label_set_text(errLabel, errMsg.c_str());
        lv_obj_set_style_text_color(errLabel, lv_color_hex(0xFF0000), LV_PART_MAIN);
        lv_obj_align(errLabel, LV_ALIGN_CENTER, 0, 0);
    } else {
        lv_obj_t *cont = lv_obj_create(scheduleScreen);
        lv_obj_set_size(cont, 300, 220);
        lv_obj_align(cont, LV_ALIGN_TOP_MID, 0, 0);
        lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_scroll_dir(cont, LV_DIR_VER);
        lv_obj_set_style_pad_all(cont, 6, 0);
        lv_obj_set_style_bg_color(cont, lv_color_hex(0xf8f8ff), LV_PART_MAIN);

        for (int i = 0; i < totalEntries; i++) {
            lv_obj_t *card = lv_obj_create(cont);
            lv_obj_set_size(card, 270, LV_SIZE_CONTENT);
            lv_obj_set_style_pad_all(card, 8, 0);
            lv_obj_set_style_radius(card, 10, 0);
            lv_obj_set_style_bg_color(card, lv_color_hex(0xffffff), LV_PART_MAIN);
            lv_obj_set_style_shadow_width(card, 8, 0);
            lv_obj_set_style_shadow_color(card, lv_color_hex(0x888888), 0);
            lv_obj_set_style_margin_bottom(card, 8, 0);

            char buf[256];
            snprintf(buf, sizeof(buf), "%s | %s\n%s\n%s", schedule[i].time.c_str(), schedule[i].room.c_str(), schedule[i].title.c_str(), schedule[i].speaker.c_str());
            lv_obj_t *label = lv_label_create(card);
            lv_label_set_text(label, buf);
            lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
            lv_obj_set_width(label, 250);
            lv_obj_align(label, LV_ALIGN_TOP_LEFT, 0, 0);
        }
    }

    // Move and resize the back button to bottom right, like main menu
    lv_obj_t *backButton = lv_btn_create(scheduleScreen);
    lv_obj_set_size(backButton, 90, 48); // Main menu button size
    lv_obj_align(backButton, LV_ALIGN_BOTTOM_RIGHT, -10, -10);
    lv_obj_t *backLabel = lv_label_create(backButton);
    lv_label_set_text(backLabel, "Back");
    lv_obj_center(backLabel);
    lv_obj_set_style_text_align(backLabel, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_align(backLabel, LV_ALIGN_CENTER, 0);
    lv_obj_add_event_cb(backButton, [](lv_event_t * e) { create_main_menu(false); }, LV_EVENT_CLICKED, NULL);
}
