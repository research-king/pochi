
#include "RobotControl.h"

#include "common.h"
#include "logprint.h"

#include "TCPSocket.h"
#include "http_request.h"

//#define WATSON_POST_URL "https://gateway-a.watsonplatform.net/visual-recognition/api/v3/classify?api_key=b59c9434aebd9ab825d1b015f31266475bbf9cd3&version=2016-05-20"
#define WATSON_POST_URL "http://gateway-a.watsonplatform.net/visual-recognition/api/v3/classify?api_key=c3659879198b8ccb65af7464b051a13a08567fb0&version=2016-05-2"
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
        int ret = 0;
        FILE *fp = fopen("/" MOUNT_NAME "/lychee_config.txt", "r");
        char ssid[32];
        char pass[32];
        if( fp != NULL)
        {
          fgets(ssid, 32, fp);
          fgets(pass, 32, fp);
          fclose(fp);
          if( ssid[strlen(ssid)-2] == '\r')
          {
            // CR-LF
            ssid[strlen(ssid)-2] = '\0';
            pass[strlen(pass)-2] = '\0';
          }
          else
          {
            // LF
          ssid[strlen(ssid)-1] = '\0';
          pass[strlen(pass)-1] = '\0';
          }
          
          ret = m_pWifiInterface->connect(ssid, pass, NSAPI_SECURITY_WPA2);
        }
        else
        {
          // default ssid
          ret = m_pWifiInterface->connect(TEST_WIFI_SSID, TEST_WIFI_PASS, NSAPI_SECURITY_WPA2);
        }
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
                  log("POCHI", LOGLEVEL_INFO, "IMAGE FILE SIZE = %d\r\n", len);

                  // create post header
                  char s_boundary[] = "------WebKitFormBoundaryZRUHAuSd1pDiYfK5\r\n";
                  char s_contdisp[] = "Content-Disposition: form-data; name=\"image\"; filename=\"cam_000.jpg\"\r\n";
                  char s_conttype[] = "Content-Type: image/jpeg\r\n\r\n";
                  char e_boundary[] = "\r\n------WebKitFormBoundaryZRUHAuSd1pDiYfK5--";

                  char *buf = new char[len+strlen(s_boundary)+strlen(s_contdisp)+strlen(s_conttype)+strlen(e_boundary)];
                  memcpy(buf, s_boundary, strlen(s_boundary));
                  memcpy(&buf[strlen(s_boundary)], s_contdisp, strlen(s_contdisp));
                  memcpy(&buf[strlen(s_boundary)+strlen(s_contdisp)], s_conttype, strlen(s_conttype));
                  fread( &buf[strlen(s_boundary)+strlen(s_contdisp)+strlen(s_conttype)], 1, len, fp);
                  fclose(fp);
                  memcpy(&buf[strlen(s_boundary)+strlen(s_contdisp)+strlen(s_conttype)+len], e_boundary, strlen(e_boundary));
                  
                  // make http request
                  //HttpRequest* request = new HttpRequest(m_pWifiInterface, HTTP_POST, WATSON_POST_URL);
                  HttpRequest* request = new HttpRequest(m_pWifiInterface, HTTP_POST, "http://104.199.222.173/r-king/space/webapi.php");
                  //HttpRequest* request = new HttpRequest(m_pWifiInterface, HTTP_POST, "http://httpbin.org/post");
 
                  request->set_header("Content-Type", "multipart/form-data; boundary=----WebKitFormBoundaryZRUHAuSd1pDiYfK5");
                  request->set_header("Connection", "keep-alive");
                  request->set_header("User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/54.0.2840.99 Safari/537.36");
                  //request->set_header("Referer", "http://104.199.222.173/r-king/space/sendfiletest.php");
                  HttpResponse* response;
                  
                  // send request
                  response = request->send(buf, strlen(s_boundary)+strlen(s_contdisp)+strlen(s_conttype)+len+strlen(e_boundary));
                  log("POCHI", LOGLEVEL_INFO, "POST FILE SIZE = %d\r\n", strlen(s_boundary)+strlen(s_contdisp)+strlen(s_conttype)+len+strlen(e_boundary));
                  
                  if(response != NULL)
                  {
                      log("POCHI", LOGLEVEL_INFO, "RESPONSE status is %d - %s\r\n", response->get_status_code(), response->get_status_message().c_str());
                      log("POCHI", LOGLEVEL_INFO, "RESPONSE body is:\r\n[%s]\r\n", response->get_body_as_string().c_str());
                  }
                  else
                  {
                      log("POCHI", LOGLEVEL_WARN, "RESPONSE IS NULL!\r\n");
                  }
                   
                  delete request; // also clears out the response
                  delete[] buf;
              }
          }
      }
      Thread::wait(20000);
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
