/*
 * $Id: logging.c,v 1.1.2.3 Exp $
 *
 * $Copyright: Copyright 2006, Broadcom Corporation All Rights Reserved.
 * THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES
 * OF ANY KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE.
 * BROADCOM SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.$
 *
 */

#include <cyg/hal/typedefs.h>
#include <cyg/kernel/kapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <cyg/logging/logging.h>

/* JJA TODO: remove ending '\n' */

struct _logger_entry {
    LOGGER *pLogger;
    struct _logger_entry *pNext;
};

static struct _logger_entry *g_pLoggerList = NULL;
static cyg_mutex_t *g_mutex_loggers = NULL;
static char g_msgbuf[1024];
static loglevel_mask_t g_logmask = 0;

void
cyg_logging_initialize(void)
{
    g_mutex_loggers = malloc(sizeof(cyg_mutex_t));
    if (g_mutex_loggers != NULL) {
        cyg_mutex_init(g_mutex_loggers);
    }
}

void
cyg_logging_uninitialize(void)
{
    if (g_mutex_loggers != NULL) {
        cyg_mutex_destroy(g_mutex_loggers);
        free(g_mutex_loggers);
        g_mutex_loggers = NULL;
    }
}

static void
mask_updated(LOGGER *plogger)
{
    struct _logger_entry *pEntry;
    LOGGER *pl;
    
    if (g_mutex_loggers == NULL) {
        /* Not initialized */
        return;
    }

    /*** Critical Section BEGIN ***/
    cyg_mutex_lock(g_mutex_loggers);
    
    g_logmask = 0;
    for(pEntry = g_pLoggerList; pEntry != NULL ; pEntry = pEntry->pNext) {
        g_logmask |= pEntry->pLogger->get_log_level_mask(pEntry->pLogger);
    }
    
    /*** Critical Section END ***/
    cyg_mutex_unlock(g_mutex_loggers);
}

bool
cyg_add_logger(LOGGER *pl)
{
    struct _logger_entry *pNew, *pEntry;
    
    if (g_mutex_loggers == NULL) {
        /* Not initialized */
        return;
    }

    if (pl == NULL) {
        return FALSE;
    }
    
    pNew = malloc(sizeof(struct _logger_entry));
    if (pNew == NULL) {
        return FALSE;
    }
    
    pNew->pLogger = pl;
    pNew->pNext = NULL;
    
    /*** Critical Section BEGIN ***/
    cyg_mutex_lock(g_mutex_loggers);
    
    if (g_pLoggerList == NULL) {
        g_pLoggerList = pNew;
    } else {
        for(pEntry = g_pLoggerList; ; pEntry = pEntry->pNext) {
            if (pEntry->pNext == NULL) {
                break;
            }
        }
        pEntry->pNext = pNew;
    }
    
    /* Tell the logger we want to be notified when mask changed */
    pl->set_mask_observer(mask_updated, pl);

    /*** Critical Section END ***/
    cyg_mutex_unlock(g_mutex_loggers);
    
    /* Because ecos's mutex can't be entered recursively, we call it here */
    mask_updated(pl);
  
    return TRUE;
}

void
cyg_remove_logger(LOGGER *pl)
{
    struct _logger_entry *pEntry, *pPrev;

    if (g_mutex_loggers == NULL) {
        /* Not initialized */
        return;
    }

    /*** Critical Section BEGIN ***/
    cyg_mutex_lock(g_mutex_loggers);
    
    pPrev = NULL;
    for(pEntry = g_pLoggerList; pEntry != NULL ; pEntry = pEntry->pNext) {
        if (pEntry->pLogger == pl) {
            if (pPrev == NULL) {
                g_pLoggerList = pEntry->pNext;
            } else {
                pPrev->pNext = pEntry->pNext;
            }
            free(pEntry);
            break;
        }
        pPrev = pEntry;
    }
    
    /*** Critical Section END ***/
    cyg_mutex_unlock(g_mutex_loggers);

    /* Update global mask */
    mask_updated(NULL);
}

static void
_cyg_syslog(loglevel_t level, logcat_t cat, const char *message)
{
    struct _logger_entry *pEntry;
    LOGGER *pl;

    if (g_mutex_loggers == NULL) {
        /* Not initialized */
        return;
    }

    for(pEntry = g_pLoggerList; pEntry != NULL ; pEntry = pEntry->pNext) {
        pl = pEntry->pLogger;
        
        /* Check if the logger needs this level */
        if (pl->get_log_level_mask(pl) & (1L << level)) {
            /* Log it */
            pl->log(level, cat, (logtime_t)time(NULL), message, pl);
        }
    }
}

void
cyg_syslog(loglevel_t level, logcat_t cat, const char *message)
{
    if (g_mutex_loggers == NULL) {
        /* Not initialized */
        return;
    }

    /* Check severity level */
    if (level > MAX_LOG_LEVEL_NUMBER) {
        level = MAX_LOG_LEVEL_NUMBER;
    }
    
    /* 
     * XXX: For performance, we should use some better synchronization scheme
     *      (like shared-read/exclusive-write lock) for this hot spot
     *      because 
     */
    
    /*** Critical Section BEGIN ***/
    cyg_mutex_lock(g_mutex_loggers);

    /* For performance, ignore messages no logger wants */
    if (g_logmask & (1L << level)) {
        
        /* Log it */
        _cyg_syslog(level, cat, message);
    }

    /*** Critical Section END ***/
    cyg_mutex_unlock(g_mutex_loggers);
}

void
cyg_syslogv(loglevel_t level, logcat_t cat, const char *fmt, va_list varg)
{
    if (g_mutex_loggers == NULL) {
        /* Not initialized */
        return;
    }

    /* Check severity level */
    if (level > MAX_LOG_LEVEL_NUMBER) {
        level = MAX_LOG_LEVEL_NUMBER;
    }
    
    /*** Critical Section BEGIN ***/
    cyg_mutex_lock(g_mutex_loggers);

    /* For performance, ignore messages no logger wants */
    if (g_logmask & (1L << level)) {

        /* Format message */
        vsnprintf(g_msgbuf, sizeof(g_msgbuf), fmt, varg);
        
        /* Log it */
        _cyg_syslog(level, cat, g_msgbuf);
    }

    /*** Critical Section END ***/
    cyg_mutex_unlock(g_mutex_loggers);
}

void
cyg_syslogf(loglevel_t level, logcat_t cat, const char *fmt, ...)
{
    va_list varg;

    if (g_mutex_loggers == NULL) {
        /* Not initialized */
        return;
    }

    /* Check severity level */
    if (level > MAX_LOG_LEVEL_NUMBER) {
        level = MAX_LOG_LEVEL_NUMBER;
    }
    
    /*** Critical Section BEGIN ***/
    cyg_mutex_lock(g_mutex_loggers);

    /* For performance, ignore messages no logger wants */
    if (g_logmask & (1L << level)) {

        /* Format the message */
        va_start(varg, fmt);
        vsnprintf(g_msgbuf, sizeof(g_msgbuf), fmt, varg);
        va_end(varg);
        
        /* Log it */
        _cyg_syslog(level, cat, g_msgbuf);
    }

    /*** Critical Section END ***/
    cyg_mutex_unlock(g_mutex_loggers);
}
