
#include "RobotControl.h"

#include "common.h"
#include "logprint.h"

#include "HTTPClient.h"

#include "TCPSocket.h"

#define WATSON_POST_URL "https://gateway-a.watsonplatform.net/visual-recognition/api/v3/classify?api_key=b59c9434aebd9ab825d1b015f31266475bbf9cd3&version=2016-05-20"
#define WATSON_GET_URL "https://gateway-a.watsonplatform.net/visual-recognition/api/v3/classify?api_key=b59c9434aebd9ab825d1b015f31266475bbf9cd3&url=http://weekly.ascii.jp/elem/000/000/346/346250/1032kanna-top_1200x.jpg&version=2016-05-19"

#define TEST_WIFI_SSID "4CE67630E22B"
#define TEST_WIFI_PASS "t3340pn5mkmkh"
#define TEST_TCP_IPADDR "172.20.10.4"
#define TEST_TCP_PORT 12345

/**
 * コンストラクタ
 * @param
 * @return
 */
RobotControl::RobotControl()
{
    m_isLife = false;
}

/**
 * WIFI接続
 * @param
 * @return
 */
bool RobotControl::isConnectWifi()
{
    if (m_pWifiInterface != NULL)
    {
        //if( m_pWifiInterface->isConnected() == true ) // TODO: 接続状態を判断するAPIなし？
        {
            return true;
        }
    }

    return false;
}

/**
 * WIFI接続
 * @param
 * @return
 */
bool RobotControl::connectWifi()
{
    if (isConnectWifi() == true)
    {
        return true;
    }

    if (m_pWifiInterface == NULL)
    {
        m_pWifiInterface = new ESP32Interface(WIFI_EN_PIN, WIFI_PI_PIN, WIFI_TX_PIN, WIFI_RX_PIN);
    }

    //-----------------------------------
    // AP SCAN
    //-----------------------------------
    WiFiAccessPoint ap[10]; /* Limit number of network arbitrary to 10 */

    int count = m_pWifiInterface->scan(ap, 10);
    for (int i = 0; i < count; i++)
    {
        log("POCHI", LOGLEVEL_INFO, "WIFI STATION: %s RSSI: %hhd Ch: %hhd\r\n", ap[i].get_ssid(),
            ap[i].get_rssi(), ap[i].get_channel());
    }

    Thread::wait(1000);

    //-----------------------------------
    // AP接続
    //-----------------------------------
    while (1)
    {
        int ret = m_pWifiInterface->connect(TEST_WIFI_SSID, TEST_WIFI_PASS, NSAPI_SECURITY_WPA2);
        if (ret != 0)
        {
            log("POCHI", LOGLEVEL_ERROR, "WIFI CONNECT ERROR. SSID=%s, PASS=%s, ret=%d\r\n", 
                TEST_WIFI_SSID, 
                TEST_WIFI_PASS, 
                ret);
        }
        else
        {
            break;
        }
        Thread::wait(5000);
    }

    log("POCHI", LOGLEVEL_MARK, "WIFI CONNECTED. IP=%s, RSSI=%d\r\n",
        m_pWifiInterface->get_ip_address(),
        m_pWifiInterface->get_rssi());

    Thread::wait(1000);

    return true;
}

/**
 * main処理.
 * @param
 * @return 1(1):成功, FALSE(0):失敗
 */
bool RobotControl::powerOn()
{
    log("POCHI", LOGLEVEL_MARK, "POWER ON\r\n");

    m_pMotorControl = new MotorControl(MOTOR_TX_PIN, MOTOR_RX_PIN);
    if (m_pMotorControl != NULL)
    {
        m_pMotorControl->init();
        log("POCHI", LOGLEVEL_MARK, "MOTOR OK\r\n");
    }

    m_pCameraControl = new CameraControl();
    if (m_pCameraControl != NULL)
    {
        m_pCameraControl->initCamera();
        log("POCHI", LOGLEVEL_MARK, "CAMERA OK\r\n");
    }

    //-----------------------------------
    // WIFI CONNECT
    //-----------------------------------
    connectWifi();

    m_isLife = 1;

    static int filecnt = 0;

    //-----------------------------------
    // メイン処理
    //-----------------------------------
    
    
    while (m_isLife)
    {
#if 0
      //-----------------------------------
      // カメラ撮影
      //-----------------------------------
      if (m_pCameraControl != NULL)
      {
          char filename[32];
          sprintf(filename, "/" MOUNT_NAME "/img%d.jpg", filecnt++);
          if (m_pCameraControl->takeCamera(filename) == 1)
          {
              log("POCHI", LOGLEVEL_INFO, "CAMERA TAKE PICTURE SUCCESS. %s\r\n", filename);

              //-----------------------------------
              // 撮影した画像をネット送信
              //-----------------------------------
              FILE *fp = fopen(filename, "r");
              if (fp != NULL)
              {
                  fseek(fp, 0, SEEK_END);
                  int len = ftell(fp);
                  fseek(fp, 0, SEEK_SET); 

                  log("POCHI", LOGLEVEL_DEBUG, "IMAGE FILE SIZE = %d\r\n", len);

                  char *buf = new char[len];
                  fread(buf, 1, sizeof(buf), fp);

                  fclose(fp);
              }
          }
      }
#endif
      int ret = 0;
      char str[1024];
      HTTPClient http(m_pWifiInterface);
      HTTPMap map;

      //POST data
      HTTPText inText(str, sizeof(str));
      map.put("Hello", "World");
      map.put("test", "1234");
      printf("\nTrying to post data...\r\n");
      ret = http.post("http://posttestserver.com/post.php?dir=example", map, &inText);
      
      //ret = http.post("https://www.google.com/", map, &inText);
      //ret = http.post(WATSON_POST_URL, map, &inText);
      if (!ret)
      {
          printf("Executed POST successfully - read %d characters\r\n", strlen(str));
          printf("Result: %s\n", str);
      }
      else
      {
          printf("Error - ret = %d - HTTP return code = %d\r\n", ret, http.getHTTPResponseCode());
      }

     m_pWifiInterface->disconnect();
    
     Thread::wait(3000);
    }
    return true;
}

/**
 * 
 * @param
 * @return 1(1):成功, FALSE(0):失敗
 */
bool RobotControl::gotoFront(void)
{
    if (m_pMotorControl == NULL)
    {
        return false;
    }

    m_pMotorControl->front();

    return true;
}

/**
 * 
 * @param
 * @return 1(1):成功, FALSE(0):失敗
 */
bool RobotControl::gotoBack(void)
{
    if (m_pMotorControl == NULL)
    {
        return false;
    }

    m_pMotorControl->back();

    return true;
}

/**
 * 
 * @param
 * @return 1(1):成功, FALSE(0):失敗
 */
bool RobotControl::turnLeft(void)
{
    if (m_pMotorControl == NULL)
    {
        return false;
    }

    m_pMotorControl->left();

    return true;
}

/**
 * 
 * @param
 * @return 1(1):成功, FALSE(0):失敗
 */
bool RobotControl::turnRight(void)
{
    if (m_pMotorControl == NULL)
    {
        return false;
    }

    m_pMotorControl->right();

    return true;
}

/**
 * 
 * @param
 * @return 1(1):成功, FALSE(0):失敗
 */
bool RobotControl::stop(void)
{
    if (m_pMotorControl == NULL)
    {
        return false;
    }

    m_pMotorControl->stop();

    return true;
}
