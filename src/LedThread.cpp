
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
    log("LED", LOGLEVEL_DEBUG, "LED THREAD START\r\n");

    s_isLife = true;

    while (s_isLife)
    {
        s_led = !s_led;

        s_LedCnt++;

        log("LED", LOGLEVEL_DEBUG, "LED THREAD LOOP %d\r\n", s_LedCnt); // 生存確認用ログ

        Thread::wait(1000);
    }
}
