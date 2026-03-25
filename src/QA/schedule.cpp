#include "QA/schedule.h"
#include "QA/Menu.h"
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

void writeFileToSD(const char* path, const String& content) {
    File file = SD.open(path, "w");
    if (!file) {
        Serial.printf("Failed to write file: %s\n", path);
        return;
    }
    file.print(content);
    file.close();
}

void fetchScheduleFromAPI() {
    WiFiClientSecure client;
    client.setInsecure();
    HTTPClient http;

    Serial.println("Checking if schedule needs to be updated...");

    lastScheduleHttpCode = 0;
    lastScheduleHttpError = "";

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
    fetchScheduleFromAPI();

    String jsonString;
    if (readScheduleFromSD(jsonString)) {
        parseSchedule(jsonString);
    } else {
        Serial.println("No schedule data available on SD.");
    }
}

void displaySchedule() {
    Serial.println("Displaying Schedule List View...");

    // HUD screen with title and accent line
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x0a0a0f), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

    lv_obj_t *hdr = lv_label_create(scr);
    lv_label_set_text(hdr, "MISSION SCHEDULE");
    lv_obj_set_style_text_color(hdr, lv_color_hex(0x00e5ff), 0);
    lv_obj_set_style_text_font(hdr, &lv_font_unscii_8, 0);
    lv_obj_align(hdr, LV_ALIGN_TOP_LEFT, 8, 8);

    lv_obj_t *line = lv_obj_create(scr);
    lv_obj_set_size(line, 312, 1);
    lv_obj_set_pos(line, 4, 22);
    lv_obj_set_style_bg_color(line, lv_color_hex(0x00e5ff), 0);
    lv_obj_set_style_border_width(line, 0, 0);
    lv_obj_set_style_radius(line, 0, 0);
    lv_obj_set_style_pad_all(line, 0, 0);
    lv_obj_clear_flag(line, LV_OBJ_FLAG_SCROLLABLE);

    load_screen_and_delete_old(scr);

    if (totalEntries == 0) {
        String errMsg = "No schedule data found!\nCheck Wi-Fi, SD card, or API.";
        if (lastScheduleHttpCode != 0) {
            errMsg += "\nHTTP code: ";
            errMsg += String(lastScheduleHttpCode);
            if (lastScheduleHttpError.length() > 0) {
                errMsg += "\n" + lastScheduleHttpError;
            }
        }
        lv_obj_t *errLabel = lv_label_create(scr);
        lv_label_set_text(errLabel, errMsg.c_str());
        lv_obj_set_style_text_color(errLabel, lv_color_hex(0xff4444), 0);
        lv_obj_set_style_text_font(errLabel, &lv_font_unscii_8, 0);
        lv_obj_align(errLabel, LV_ALIGN_TOP_LEFT, 8, 28);
    } else {
        lv_obj_t *cont = lv_obj_create(scr);
        lv_obj_set_size(cont, 304, 170);
        lv_obj_set_pos(cont, 8, 26);
        lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_scroll_dir(cont, LV_DIR_VER);
        lv_obj_set_style_pad_all(cont, 4, 0);
        lv_obj_set_style_pad_row(cont, 4, 0);
        lv_obj_set_style_bg_color(cont, lv_color_hex(0x0a0a0f), 0);
        lv_obj_set_style_border_width(cont, 0, 0);

        for (int i = 0; i < totalEntries; i++) {
            lv_obj_t *card = lv_obj_create(cont);
            lv_obj_set_size(card, 290, LV_SIZE_CONTENT);
            lv_obj_set_style_pad_all(card, 6, 0);
            lv_obj_set_style_radius(card, 4, 0);
            lv_obj_set_style_bg_color(card, lv_color_hex(0x1a1a2e), 0);
            lv_obj_set_style_border_color(card, lv_color_hex(0x00e5ff), 0);
            lv_obj_set_style_border_width(card, 1, 0);
            lv_obj_set_style_border_side(card, LV_BORDER_SIDE_LEFT, 0);
            lv_obj_set_style_shadow_width(card, 0, 0);
            lv_obj_set_style_margin_bottom(card, 4, 0);

            // Time/room header
            lv_obj_t *time_lbl = lv_label_create(card);
            char tbuf[64];
            snprintf(tbuf, sizeof(tbuf), "%s | %s", schedule[i].time.c_str(), schedule[i].room.c_str());
            lv_label_set_text(time_lbl, tbuf);
            lv_obj_set_style_text_color(time_lbl, lv_color_hex(0x00e5ff), 0);
            lv_obj_set_style_text_font(time_lbl, &lv_font_unscii_8, 0);

            // Title + speaker
            lv_obj_t *info_lbl = lv_label_create(card);
            char ibuf[192];
            snprintf(ibuf, sizeof(ibuf), "%s\n%s", schedule[i].title.c_str(), schedule[i].speaker.c_str());
            lv_label_set_text(info_lbl, ibuf);
            lv_label_set_long_mode(info_lbl, LV_LABEL_LONG_WRAP);
            lv_obj_set_width(info_lbl, 270);
            lv_obj_set_style_text_color(info_lbl, lv_color_hex(0xcccccc), 0);
        }
    }

    // Back button
    create_back_button(scr, [](lv_event_t * e) { create_main_menu(false); });
}
