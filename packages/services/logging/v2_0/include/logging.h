/*
 * $Id: logging.h,v 1.1.2.3 Exp $
 *
 * $Copyright: Copyright 2006, Broadcom Corporation All Rights Reserved.
 * THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES
 * OF ANY KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE.
 * BROADCOM SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.$
 *
 */

#ifndef _SERVICE_LOGGING_H_
#define _SERVICE_LOGGING_H_

#include <time.h>

#define MAX_LOG_LEVEL_NUMBER  (31)  /* Log level: 0 - 31 */

enum {
    LOGGER_PROP_READABLE    = 1,
    LOGGER_PROP_READ_BY_PTR = 2,
    LOGGER_PROP_CLEARABLE   = 3
};

typedef uint32 logidx_t;
typedef uint8 loglevel_t;
typedef uint32 logcat_t;
typedef time_t logtime_t;
typedef uint32 loglevel_mask_t;

typedef struct _log_entry {
    logidx_t        index;
    loglevel_t      level;
    logcat_t        category;
    logtime_t       time;
    const char *    message;
} LOG_ENTRY;

typedef struct _logger_f {
    
    const char *(*get_name)(struct _logger_f *);
    
    uint32 (*get_properties)(struct _logger_f *);
    
    void (*set_log_level_mask)(loglevel_mask_t, struct _logger_f *);
    
    loglevel_mask_t (*get_log_level_mask)(struct _logger_f *);
    
    void (*log)(
                loglevel_t level, 
                logcat_t category, 
                logtime_t time, 
                const char *message, 
                struct _logger_f *pl
               );
    
    void (*clear)(struct _logger_f *);
    
    uint32 (*get_log_count)(logidx_t *, struct _logger_f *);
    
    bool (*get_log)(LOG_ENTRY *plog, char *buf, int len, struct _logger_f *);
    
    void (*set_mask_observer)(void (*)(struct _logger_f *), struct _logger_f *);

} LOGGER;

void cyg_logging_initialize(void);
void cyg_logging_uninitialize(void);

bool cyg_add_logger(LOGGER *pl);
void cyg_remove_logger(LOGGER *pl);

void cyg_syslog(loglevel_t level, logcat_t cat, const char *message);
void cyg_syslogf(loglevel_t level, logcat_t cat, const char *fmt, ...);
void cyg_syslogv(loglevel_t level, logcat_t cat, const char *fmt, va_list varg);

#endif /* _SERVICE_LOGGING_H_ */
