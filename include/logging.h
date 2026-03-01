#ifndef LOGGING_H
#define LOGGING_H

#include <stdarg.h>

typedef enum {
  LOG_LEVEL_DEBUG,
  LOG_LEVEL_INFO,
  LOG_LEVEL_WARN,
  LOG_LEVEL_ERROR
} LogLevel;

void Logger_Init(const char *filename);
void Logger_Log(LogLevel level, const char *file, int line, const char *fmt,
                ...);
void Logger_Close(void);

#define LOG_DEBUG(...)                                                         \
  Logger_Log(LOG_LEVEL_DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_INFO(...)                                                          \
  Logger_Log(LOG_LEVEL_INFO, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_WARN(...)                                                          \
  Logger_Log(LOG_LEVEL_WARN, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_ERROR(...)                                                         \
  Logger_Log(LOG_LEVEL_ERROR, __FILE__, __LINE__, __VA_ARGS__)

#endif
