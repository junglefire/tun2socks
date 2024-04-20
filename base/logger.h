#ifndef TUN2SOCKS_LOGGER_H__
#define TUN2SOCKS_LOGGER_H__

#include <strings.h>
#include <stdio.h>

/*
 * print hex string
 */
inline
void hex_print(unsigned char* c, int len, char* pipe) {
    for (int i = 0; i < len; i++) {
        if (i % 0x10 == 0) {
            fprintf(stderr, "\n%s %04x ", pipe, i);
        }
        if (i % 8 == 0) {
            fprintf(stderr, " ");
        }
        fprintf(stderr, "%02x ", c[i]);
    }
    fprintf(stderr, "\n\n");
}

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

#if defined DETAIL_MESSAGE 
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

#elif defined SIMPLE_MESSAGE 
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

#elif defined ZLOG_MESSAGE
    #include <zlog.h>
    #define info dzlog_info
    #define debug dzlog_debug
    #define error dzlog_error
#endif 

#endif//TUN2SOCKS_LOGGER_H__