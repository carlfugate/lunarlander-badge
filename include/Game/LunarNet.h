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

#define LN_SERVER_HOST "lunarlander.local"
#define LN_SERVER_PORT 8080

void net_init();
bool net_spectate(const char *session_id);
bool net_connect_player();
void net_send_input(const char *action);
void net_create_room(const char *player_name, uint8_t difficulty);
void net_join_room(const char *room_id, const char *player_name);
void net_start_game();
bool net_poll(GameState &gs);
NetMode net_get_mode();
void net_disconnect();
bool net_fetch_rooms(JsonDocument &doc);

#endif // NATIVE_TEST
#endif
