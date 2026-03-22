#ifndef NATIVE_TEST

#include "Game/LunarNet.h"
#include <ArduinoWebsockets.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

using namespace websockets;

static WebsocketsClient ws_client;
static NetMode s_mode = NET_DISCONNECTED;
static bool s_has_msg = false;
static String s_msg_buf;

// Lobby state
static String s_room_id;
static int s_player_count = 0;
static bool s_is_creator = false;
static bool s_lobby_update = false;
static bool s_game_started = false;
static String s_lobby_json;

static void on_message(WebsocketsMessage msg) {
    s_msg_buf = msg.data();
    s_has_msg = true;
}

static String build_url(const char *path) {
    return String("ws://") + LN_SERVER_HOST + ":" + LN_SERVER_PORT + path;
}

void net_init() {
    s_mode = NET_DISCONNECTED;
    s_has_msg = false;
    s_msg_buf = "";
    s_room_id = "";
    s_player_count = 0;
    s_is_creator = false;
    s_lobby_update = false;
    s_game_started = false;
    s_lobby_json = "";
}

bool net_spectate(const char *session_id) {
    net_disconnect();
    String url = build_url((String("/spectate/") + session_id).c_str());
    ws_client.onMessage(on_message);
    if (ws_client.connect(url)) {
        s_mode = NET_SPECTATING;
        return true;
    }
    return false;
}

bool net_connect_player() {
    net_disconnect();
    ws_client.onMessage(on_message);
    if (ws_client.connect(build_url("/ws"))) {
        s_mode = NET_PLAYING;
        return true;
    }
    return false;
}

static void send_json(JsonDocument &doc) {
    String out;
    serializeJson(doc, out);
    ws_client.send(out);
}

void net_send_input(const char *action) {
    JsonDocument doc;
    doc["type"] = "input";
    doc["action"] = action;
    send_json(doc);
}

void net_create_room(const char *player_name, uint8_t difficulty) {
    JsonDocument doc;
    doc["type"] = "create_room";
    doc["player_name"] = player_name;
    doc["difficulty"] = difficulty;
    send_json(doc);
}

void net_join_room(const char *room_id, const char *player_name) {
    JsonDocument doc;
    doc["type"] = "join_room";
    doc["room_id"] = room_id;
    doc["player_name"] = player_name;
    send_json(doc);
}

void net_start_game() {
    JsonDocument doc;
    doc["type"] = "start_game";
    send_json(doc);
}

static void parse_lander(JsonObject obj, Lander &l) {
    l.x = obj["x"] | l.x;
    l.y = obj["y"] | l.y;
    l.vx = obj["vx"] | l.vx;
    l.vy = obj["vy"] | l.vy;
    l.rotation = obj["rotation"] | l.rotation;
    l.fuel = obj["fuel"] | l.fuel;
    l.crashed = obj["crashed"] | l.crashed;
    l.landed = obj["landed"] | l.landed;
    l.thrusting = obj["thrusting"] | l.thrusting;
}

bool net_poll(GameState &gs) {
    if (s_mode == NET_DISCONNECTED) return false;
    ws_client.poll();

    if (!s_has_msg) return false;
    s_has_msg = false;

    JsonDocument doc;
    if (deserializeJson(doc, s_msg_buf)) return false;

    const char *type = doc["type"];
    if (!type) return false;

    if (strcmp(type, "telemetry") == 0) {
        parse_lander(doc["lander"], gs.lander);
        return true;
    }

    if (strcmp(type, "init") == 0) {
        // Parse terrain
        JsonArray pts = doc["terrain"]["points"];
        gs.terrain.num_points = 0;
        for (JsonArray pt : pts) {
            if (gs.terrain.num_points >= LN_MAX_TERRAIN_POINTS) break;
            gs.terrain.points[gs.terrain.num_points][0] = pt[0];
            gs.terrain.points[gs.terrain.num_points][1] = pt[1];
            gs.terrain.num_points++;
        }
        gs.terrain.zone_x1 = doc["terrain"]["zone_x1"] | 0;
        gs.terrain.zone_x2 = doc["terrain"]["zone_x2"] | 0;
        gs.terrain.zone_y = doc["terrain"]["zone_y"] | 0;
        if (doc.containsKey("lander")) {
            parse_lander(doc["lander"], gs.lander);
        }
        gs.phase = PHASE_PLAYING;
        return true;
    }

    if (strcmp(type, "game_over") == 0) {
        bool crashed = doc["crashed"] | false;
        gs.phase = crashed ? PHASE_CRASHED : PHASE_LANDED;
        gs.score = doc["score"] | 0;
        return true;
    }

    // Lobby messages
    if (strcmp(type, "room_created") == 0) {
        s_room_id = doc["room_id"] | "";
        s_is_creator = true;
        s_lobby_json = s_msg_buf;
        s_lobby_update = true;
        return false;
    }

    if (strcmp(type, "room_joined") == 0) {
        s_room_id = doc["room_id"] | "";
        s_lobby_json = s_msg_buf;
        s_lobby_update = true;
        return false;
    }

    if (strcmp(type, "player_list") == 0) {
        s_player_count = doc["players"].size();
        s_lobby_json = s_msg_buf;
        s_lobby_update = true;
        return false;
    }

    if (strcmp(type, "countdown") == 0) {
        s_lobby_json = s_msg_buf;
        s_lobby_update = true;
        return false;
    }

    if (strcmp(type, "game_started") == 0) {
        s_game_started = true;
        s_lobby_json = s_msg_buf;
        s_lobby_update = true;
        return false;
    }

    return false;
}

NetMode net_get_mode() {
    return s_mode;
}

void net_disconnect() {
    if (s_mode != NET_DISCONNECTED) {
        ws_client.close();
    }
    s_mode = NET_DISCONNECTED;
    s_has_msg = false;
    s_room_id = "";
    s_player_count = 0;
    s_is_creator = false;
    s_lobby_update = false;
    s_game_started = false;
    s_lobby_json = "";
}

const char* net_get_room_id() { return s_room_id.c_str(); }
int net_get_player_count() { return s_player_count; }
bool net_has_lobby_update() { return s_lobby_update; }
const char* net_get_lobby_json() { return s_lobby_json.c_str(); }
bool net_is_creator() { return s_is_creator; }
bool net_game_started() { return s_game_started; }
void net_clear_lobby_flags() { s_lobby_update = false; }

bool net_fetch_rooms(JsonDocument &doc) {
    HTTPClient http;
    String url = String("http://") + LN_SERVER_HOST + ":" + LN_SERVER_PORT + "/rooms";
    http.begin(url);
    int code = http.GET();
    if (code != 200) {
        http.end();
        return false;
    }
    DeserializationError err = deserializeJson(doc, http.getString());
    http.end();
    return !err;
}

bool net_fetch_sessions(JsonDocument &doc) {
    HTTPClient http;
    String url = String("http://") + LN_SERVER_HOST + ":" + LN_SERVER_PORT + "/sessions";
    http.begin(url);
    int code = http.GET();
    if (code != 200) {
        http.end();
        return false;
    }
    DeserializationError err = deserializeJson(doc, http.getString());
    http.end();
    return !err;
}

#endif // NATIVE_TEST
