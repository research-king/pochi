#include "MotorControl.h"

#define MCP_Baud 115200 // MotorControl Port

#define LOGENABLE 0

#if LOGENABLE
#define DEBUG_PRINT(x) printf(x)
#else
#define DEBUG_PRINT(x)
#endif

/******************************************************************************
参考までに(AGB-DCMモジュール)

＜AGB65-DCM WEBサイト＞
http://www.robotsfx.com/robot/AGB65_DCM.html

＜送信フォーマット＞
[255][ID(28)][長(5)][命令(4)][ch1方向][ch1出力(0-100)][ch2方向][ch2出力(0-100)]

・シンクロバイトは２５５で固定です。（データを送信する先頭は必ず２５５）
・IDは設定したIDを指定します。（出荷時はＩＤ＝２８）
・バイト長は５で固定です。
・命令は４で固定です。
・ch1方向は次のようになります。　０＝停止　１＝ＣＷ（時計回り）　２＝ＣＣＷ（反時計まわり）
・ch1出力は１％単位で値を設定します。
・ch2方向と、ch2出力もch1同様です。

*******************************************************************************/

/**
 * コンストラクタ
 * @param tx:TXピン名, rx:RXピン名
 * @return
 */
MotorControl::MotorControl(PinName tx, PinName rx) : ser(tx, rx)
{

    DEBUG_PRINT("MotorControl:MotorControl()\r\n");
}

/**
 * 初期化処理.
 * @param
 * @return TRUE(1):成功, FALSE(0):失敗
 */
unsigned char MotorControl::init()
{

    DEBUG_PRINT("MotorControl:init()\r\n");
    ser.baud(MCP_Baud);
    return 1;
}

/**
 * 前に進む処理.（両モーター同時に時計回り回転）
 * @param
 * @return TRUE(1):成功, FALSE(0):失敗
 */
unsigned char MotorControl::front()
{

    DEBUG_PRINT("MotorControl:front()\r\n");
    char data_front[8] = {0xFF, 0x1C, 0x05, 0x04, 0x01, 0x64, 0x01, 0x64}; //時計回り
    ser.printf(data_front);

    return 1;
}

/**
 * 後ろに進む処理？（両モーター同時に反時計回り回転）
 * @param
 * @return TRUE(1):成功, FALSE(0):失敗
 */
unsigned char MotorControl::back()
{

#if 0    
    DEBUG_PRINT("MotorControl:back()\r\n");    
    char data_back[8] = {0xFF,0x1C,0x05,0x04,0x02,0x28,0x02,0x28};//反時計まわり
    ser.printf(data_back); 
    // backは非対応
#endif

    return 1;
}

/**
 * 左に進む処理？（左モーター停止、右モーター時計回り回転）
 * @param
 * @return TRUE(1):成功, FALSE(0):失敗
 */
unsigned char MotorControl::left()
{

    DEBUG_PRINT("MotorControl:left()\r\n");
    char data_left[8] = {0xFF, 0x1C, 0x05, 0x04, 0x00, 0x00, 0x01, 0x64}; //TODO：要調整、上の＜送信フォーマット＞
    for (int i = 0; i < sizeof(data_left); i++)
    {
        ser.putc(data_left[i]);
    }

    return 1;
}

/**
 * 右に進む処理？（右モーター停止、左モーター時計回り回転）
 * @param
 * @return TRUE(1):成功, FALSE(0):失敗
 */
unsigned char MotorControl::right()
{

    DEBUG_PRINT("MotorControl:right()\r\n");
    char data_right[8] = {0xFF, 0x1C, 0x05, 0x04, 0x01, 0x64, 0x00, 0x00}; //TODO：要調整、上の＜送信フォーマット＞
    for (int i = 0; i < sizeof(data_right); i++)
    {
        ser.putc(data_right[i]);
    }

    return 1;
}

/**
 * 停止処理（両モーター停止）
 * @param
 * @return TRUE(1):成功, FALSE(0):失敗
 */
unsigned char MotorControl::stop()
{

    DEBUG_PRINT("MotorControl:stop()\r\n");
    char data_stop[4] = {0xFF, 0x1C, 0x01, 0x00}; //両モーター停止

    for (int i = 0; i < sizeof(data_stop); i++)
    {
        ser.putc(data_stop[i]);
    }
    return 1;
}