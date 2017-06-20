
/********************************************************************************

    処理概要    ：  LOG FUNCTION (C言語)

********************************************************************************/

#include "logprint.h"
#include "mbed.h"
#include <RawSerial.h>

#define LOGCOLORENABLE  1
#define LOGSTRINGMAX    1024

static RawSerial pc(USBTX, USBRX, 115200);

/*------------------------------------------------------------------------------*/
/* プロトタイプ宣言                                 */
/*------------------------------------------------------------------------------*/
/* 公開API */
void logprintf(const char *tag, LOG_LEVEL level, const char *str, ...);

/* 非公開API */

/*------------------------------------------------------------------------------*/
/* グローバル変数の定義                               */
/*------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------*/
/* スタティック変数宣言                               */
/*------------------------------------------------------------------------------*/
static Mutex s_mutex;

/**
 * 
 * @param
 * @return 
 */
void log(const char *tag, LOG_LEVEL level, const char *str, ...)
{
    s_mutex.lock();

    char buf[LOGSTRINGMAX];

    memset(buf, 0, sizeof(buf));

    va_list list;
    va_start(list, str);
    vsnprintf(buf, sizeof(buf)-1, str, list);
    va_end(list);

    switch (level)
    {
    case LOGLEVEL_DEBUG:
#if LOGCOLORENABLE
        pc.printf("\e[90m[%s][D] %s\e[m", tag, buf); // 文字を灰色に
#else
        pc.printf("[%s][D] %s", tag, buf);
#endif
        break;
    case LOGLEVEL_INFO:
        pc.printf("[%s][I] %s", tag, buf);
        break;
    case LOGLEVEL_WARN:
#if LOGCOLORENABLE
        pc.printf("\e[33m[%s][W] %s\e[m", tag, buf); // 文字を黄色に
#else
        pc.printf("[%s][W] %s", tag, buf);
#endif
        break;
    case LOGLEVEL_ERROR:
#if LOGCOLORENABLE
        pc.printf("\e[31m[%s][E] %s\e[m", tag, buf); // 文字を赤色に
#else
        pc.printf("[%s][E] %s", tag, buf);
#endif
        break;

    case LOGLEVEL_MARK:
#if LOGCOLORENABLE
        pc.printf("\e[32m[%s][M] %s\e[m", tag, buf); // 文字を緑色に
#else
        pc.printf("[%s][M] %s", tag, buf);
#endif
        break;
    }

    s_mutex.unlock();
}
