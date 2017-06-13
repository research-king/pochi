

#include "mbed.h"

#include "common.h"
#include "logprint.h"
#include "LedThread.h"

#include "SdUsbConnect.h"
#include "RobotControl.h"

/*------------------------------------------------------------------------------*/
/* プロトタイプ宣言                                                             */
/*------------------------------------------------------------------------------*/
/* 公開API */

/* 非公開API */

/*------------------------------------------------------------------------------*/
/* 変数宣言                                                                    */
/*------------------------------------------------------------------------------*/

// SD & USB
SdUsbConnect s_storage(MOUNT_NAME);
RobotControl s_RobotControl;

//-----------------------------------
// Thread
//-----------------------------------
static Thread s_led_thread;

/**
 * MAIN
 * @param
 * @return 
 */
int main()
{
    char buf[256];
    memset(buf, 0, sizeof(buf));
    snprintf(buf, sizeof(buf) - 1, "PROC START. Build Date: %11.11s %8.8s\r\n",
             __DATE__, __TIME__);

    log("MAIN", LOGLEVEL_MARK, buf);

    Thread::wait(2000);

    //-----------------------------------
    // 初期化処理
    //-----------------------------------
    // ハートビートLEDスレッド起動
    s_led_thread.start(LedThread);

    //-----------------------------------
    // Storage wait
    //-----------------------------------
    s_storage.wait_connect();

    log("MAIN", LOGLEVEL_MARK, "STORAGE CONNECTED\r\n");

    Thread::wait(1000);
    
    //-----------------------------------
    // メイン処理
    //-----------------------------------
    s_RobotControl.powerOn();
}
