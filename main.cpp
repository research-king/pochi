

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
    log("MAIN", LOGLEVEL_MARK, "PROC START. Build Date: %11.11s %8.8s\r\n",
             __DATE__, __TIME__);

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
    // POCHIメイン処理(中でループ)
    //-----------------------------------
    s_RobotControl.powerOn();
}
