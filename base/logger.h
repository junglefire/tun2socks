#ifndef BEEHIVE_H__
#define BEEHIVE_H__

#include <strings.h>
#include <stdio.h>

/**
 * POSIX does specify that fprintf() calls from different threads of the same process do not interfere 
 * with each other, and if that if they both specify the same target file, their output will not be 
 * intermingled. 
 * POSIX-conforming fprintf() is thread-safe.
 */

/**
 * define logger macro
 */
#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

#define DEBUG 1

// print message detail
#ifdef DETAIL_MESSAGE 
#define info(fmt, args...) \
    do {\
        fprintf(stderr, "[info][%s:%d:%s()] " fmt "\n", __FILENAME__, __LINE__, __func__, ##args);\
    } while(0)

#define debug(fmt, args...) \
    do {\
        if (DEBUG) fprintf(stderr, "[debug][%s:%d:%s()] " fmt "\n", __FILENAME__, __LINE__, __func__, ##args);\
    } while(0)

#define error(fmt, args...) \
    do {\
        fprintf(stderr, "[error][%s:%d:%s()] " fmt "\n", __FILENAME__, __LINE__, __func__, ##args);\
    } while(0)
#endif

// print simple message
#ifdef SIMPLE_MESSAGE 
#define info(fmt, args...) \
    do {\
        fprintf(stderr, "[info] " fmt "\n", ##args);\
    } while(0)

#define debug(fmt, args...) \
    do {\
        if (DEBUG) fprintf(stderr, "[debug] " fmt "\n", ##args);\
    } while(0)

#define error(fmt, args...) \
    do {\
        fprintf(stderr, "[error] " fmt "\n", ##args);\
    } while(0)
#endif

// using zlog lib
#ifdef ZLOG_MESSAGE
#include <zlog.h>

#define info dzlog_info
#define debug dzlog_debug
#define error dzlog_error
#endif // ZLOG_MESSAGE

#endif