#ifndef QA_SERIAL_CMD_H
#define QA_SERIAL_CMD_H

#ifdef FF_SERIAL_TEST

void serial_cmd_init();
void serial_cmd_poll();  // Call from loop()
void serial_cmd_log(const char *tag, const char *fmt, ...);  // Structured logging

#else

inline void serial_cmd_init() {}
inline void serial_cmd_poll() {}
inline void serial_cmd_log(const char*, const char*, ...) {}

#endif
#endif
