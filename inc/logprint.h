#ifndef LOGPRINT_H
#define LOGPRINT_H

enum LOG_LEVEL {
    LOGLEVEL_DEBUG,
    LOGLEVEL_INFO,
    LOGLEVEL_WARN,
    LOGLEVEL_ERROR,
    LOGLEVEL_MARK,
};

extern void log(const char* tag, LOG_LEVEL level, const char* str);

#endif  // LOGPRINT_H