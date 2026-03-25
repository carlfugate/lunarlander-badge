#ifndef QA_BLE_PRESENCE_H
#define QA_BLE_PRESENCE_H

#include <stdint.h>

#define BLE_MAX_CREW 100
#define BLE_CALLSIGN_LEN 10
#define BLE_MSG_NONE 0
#define BLE_NUM_MESSAGES 12

struct CrewEntry {
    char callsign[BLE_CALLSIGN_LEN + 1];
    uint16_t high_score;
    int8_t rssi;
    uint32_t first_seen_met;  // MET seconds when first discovered
    uint32_t last_seen_ms;    // millis() of last sighting
    bool new_discovery;       // true on first frame only
    uint8_t last_msg_id;      // last message received from this badge
    uint32_t last_msg_ms;     // when we received it
};

void ble_presence_init(const char *my_callsign, uint16_t my_score);
void ble_presence_update_score(uint16_t score);
void ble_presence_set_status(uint8_t status); // 0=idle, 1=playing, 2=menu
int ble_presence_nearby_count();  // badges seen in last 30s
int ble_presence_total_count();   // total unique badges ever seen
const CrewEntry* ble_presence_get_crew();
int ble_presence_get_crew_count();
void create_crew_log_window();
void ble_presence_send_message(uint8_t msg_id);
const char* ble_presence_get_message_text(uint8_t msg_id);
void create_comms_window();

bool ble_presence_has_notification();
const char* ble_presence_get_notification();
void ble_presence_clear_notification();

#endif
