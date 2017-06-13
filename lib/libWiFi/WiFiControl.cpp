#include "WiFiControl.h"

#define LOGENABLE 0

#if LOGENABLE
#define DEBUG_PRINT(x) printf(x)
#else
#define DEBUG_PRINT(x)
#endif


/**
 * コンストラクタ
 * @param tx:TXピン名, rx:RXピン名
 * @return
 */
WiFiControl::WiFiControl(PinName tx, PinName rx)
{
    DEBUG_PRINT("WiFiControl:WiFiControl()\r\n");
}

/**
 * 接続処理.
 * @param
 * @return TRUE(1):成功, FALSE(0):失敗
 */
unsigned char WiFiControl::connect()
{

    DEBUG_PRINT("WiFiControl:connect()\r\n");
    return 1;
}

/**
 * 切断処理.
 * @param
 * @return TRUE(1):成功, FALSE(0):失敗
 */
unsigned char WiFiControl::disconnect()
{

    DEBUG_PRINT("WiFiControl:disconnect()\r\n");
    return 1;
}

