#ifndef QA_SERIAL_CMD_H
#define QA_SERIAL_CMD_H

#ifdef FF_SERIAL_TEST

#include <stdarg.h>

typedef void (*serial_cmd_handler_t)(const char *args);

void serial_cmd_init();
void serial_cmd_poll();
void serial_cmd_register(const char *prefix, serial_cmd_handler_t handler, const char *help_text);
void serial_cmd_log(const char *tag, const char *fmt, ...);

#else

inline void serial_cmd_init() {}
inline void serial_cmd_poll() {}
inline void serial_cmd_register(const char*, void(*)(const char*), const char*) {}
inline void serial_cmd_log(const char*, const char*, ...) {}

#endif
#endif
