#pragma once

#include <TFT_eSPI.h>  // TFT library
#include <Arduino.h>

// Structure to represent a schedule entry with details about a session
struct ScheduleEntry {
    String speaker;      // Name of the speaker
    String title;        // Title of the session
    String time;         // Time of the session
    String room;         // Room where the session is held
    String description;  // Description of the session
};

// Array to hold the schedule entries
extern ScheduleEntry schedule[];

// Total number of entries in the schedule
extern int totalEntries;

// Function to create the main menu; optionally checks for OTA updates
extern void create_main_menu(bool show_ota_check);

// Fetch the schedule data from an external API
void fetchScheduleFromAPI();

// Parse the schedule data from a JSON string
void parseSchedule(const String &jsonString);

// Display the schedule on the screen
void displaySchedule();

// Navigate to the next page of the schedule
void nextPage();

// Navigate to the previous page of the schedule
void prevPage();

// Read the schedule data from an SD card
bool readScheduleFromSD(String &jsonString);

// Save the schedule data to an SD card
void saveScheduleToSD(const String &jsonString);

// Load the schedule into memory
void loadSchedule();