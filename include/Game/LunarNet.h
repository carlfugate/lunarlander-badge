#ifndef LUNAR_NET_H
#define LUNAR_NET_H

#ifndef NATIVE_TEST

#include "LunarState.h"
#include <ArduinoJson.h>

enum NetMode {
    NET_DISCONNECTED,
    NET_CONNECTING,
    NET_SPECTATING,
    NET_PLAYING
};

// Server defaults (override via SD card /lander_server.txt)
#ifndef LN_SERVER_HOST
#define LN_SERVER_HOST "lunarlander.local"
#endif
#ifndef LN_SERVER_PORT
#define LN_SERVER_PORT 8080
#endif

void net_init();
bool net_spectate(const char *session_id);
bool net_connect_player();
void net_send_input(const char *action);
void net_send_start(uint8_t difficulty);
void net_create_room(const char *player_name, uint8_t difficulty);
void net_join_room(const char *room_id, const char *player_name);
void net_start_game();
bool net_poll(GameState &gs);
NetMode net_get_mode();
void net_disconnect();
bool net_fetch_rooms(JsonDocument &doc);
bool net_fetch_sessions(JsonDocument &doc);

// Lobby state getters
const char* net_get_room_id();
int net_get_player_count();
bool net_has_lobby_update();
const char* net_get_lobby_json();
bool net_is_creator();
bool net_game_started();
void net_clear_lobby_flags();

#endif // NATIVE_TEST
#endif
