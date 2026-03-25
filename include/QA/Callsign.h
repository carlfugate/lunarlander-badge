#ifndef QA_CALLSIGN_H
#define QA_CALLSIGN_H

#define MAX_CALLSIGN_LEN 10

void callsign_init();
const char* callsign_get();
void callsign_set(const char* name);
void create_callsign_window();

#endif
