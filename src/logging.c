#include "logging.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_LOG_SIZE (5 * 1024 * 1024) // 5 MB

static FILE *log_file = NULL;
static char log_filename[256];
static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

static const char *level_strings[] = {"DEBUG", "INFO", "WARN", "ERROR"};

void Logger_Init(const char *filename) {
  pthread_mutex_lock(&log_mutex);
  if (!log_file) {
    strncpy(log_filename, filename, sizeof(log_filename) - 1);
    log_filename[sizeof(log_filename) - 1] = '\0';
    log_file = fopen(log_filename, "a");
  }
  pthread_mutex_unlock(&log_mutex);
}

static void RotateLogIfNeeded() {
  if (!log_file)
    return;

  long size = ftell(log_file);
  if (size >= MAX_LOG_SIZE) {
    fclose(log_file);

    char new_filename[512];
    snprintf(new_filename, sizeof(new_filename), "%s.1", log_filename);
    remove(new_filename); // Remove old backup if it exists
    rename(log_filename, new_filename);

    log_file = fopen(log_filename, "a");
  }
}

void Logger_Log(LogLevel level, const char *file, int line, const char *fmt,
                ...) {
  pthread_mutex_lock(&log_mutex);

  time_t now;
  time(&now);
  struct tm *local = localtime(&now);

  char time_str[64];
  strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S",
           local); // standard format

  // Check if filename contains paths, strip them for shorter logs
  const char *short_file = strrchr(file, '/');
  if (short_file) {
    short_file++;
  } else {
    short_file = file;
  }

  // Print to console (stderr for ERROR, stdout otherwise)
  FILE *console_out = (level == LOG_LEVEL_ERROR) ? stderr : stdout;
  fprintf(console_out, "[%s] [%s] %s:%d: ", time_str, level_strings[level],
          short_file, line);
  va_list args;
  va_start(args, fmt);
  vfprintf(console_out, fmt, args);
  va_end(args);
  fprintf(console_out, "\n");

  // Print to file
  if (log_file) {
    RotateLogIfNeeded();
    fprintf(log_file, "[%s] [%s] %s:%d: ", time_str, level_strings[level],
            short_file, line);

    va_start(args, fmt);
    vfprintf(log_file, fmt, args);
    va_end(args);

    fprintf(log_file, "\n");
    fflush(log_file);
  }

  pthread_mutex_unlock(&log_mutex);
}

void Logger_Close(void) {
  pthread_mutex_lock(&log_mutex);
  if (log_file) {
    fclose(log_file);
    log_file = NULL;
  }
  pthread_mutex_unlock(&log_mutex);
}
