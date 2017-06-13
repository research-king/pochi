
#include "LedThread.h"

#include "mbed.h"
#include "logprint.h"
#include "common.h"

static DigitalOut s_led(HEARTBEAT_LED);
static bool s_isLife = false;
static int s_LedCnt = 0;

/**
 * 
 * @param
 * @return 
 */
void LedThread(void)
{

    log("LED", LOGLEVEL_INFO, "LED THREAD START\r\n");

    char buf[256];
    memset(buf, 0, sizeof(buf));

    s_isLife = true;

    while (s_isLife)
    {

        s_led = !s_led;

        s_LedCnt++;

        snprintf(buf, sizeof(buf) - 1, "LED THREAD LOOP %d\r\n", s_LedCnt);
        log("LED", LOGLEVEL_DEBUG, buf); // 生存確認用ログ

        Thread::wait(1000);
    }
}
