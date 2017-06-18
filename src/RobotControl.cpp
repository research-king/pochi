
#include "RobotControl.h"

#include "common.h"
#include "logprint.h"

#include "TCPSocket.h"
#include "http_request.h"

#define WATSON_POST_URL "http://gateway-a.watsonplatform.net/visual-recognition/api/v3/classify?api_key=c3659879198b8ccb65af7464b051a13a08567fb0&version=2016-05-2"
#define WATSON_GET_URL "https://gateway-a.watsonplatform.net/visual-recognition/api/v3/classify?api_key=b59c9434aebd9ab825d1b015f31266475bbf9cd3&url=http://weekly.ascii.jp/elem/000/000/346/346250/1032kanna-top_1200x.jpg&version=2016-05-19"

#define TEST_WIFI_SSID "4CE67630E22B"
#define TEST_WIFI_PASS "t3340pn5mkmkh"

static char g_ssid[PATH_MAX];
static char g_pass[PATH_MAX];
static char g_post_url[PATH_MAX];

bool is_connect_wifi = false;

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
    if (is_connect_wifi == true)
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
        if( fp != NULL)
        {
          fgets(g_ssid, PATH_MAX, fp);
          fgets(g_pass, PATH_MAX, fp);
          fgets(g_post_url, PATH_MAX, fp);
          fclose(fp);
          if( g_ssid[strlen(g_ssid)-2] == '\r')
          {
            // CR-LF
            g_ssid[strlen(g_ssid)-2] = '\0';
            g_pass[strlen(g_pass)-2] = '\0';
            g_post_url[strlen(g_post_url)-2] = '\0';
          }
          else
          {
            // LF
            g_ssid[strlen(g_ssid)-1] = '\0';
            g_pass[strlen(g_pass)-1] = '\0';
            g_post_url[strlen(g_post_url)-1] = '\0';
          }
          
          ret = m_pWifiInterface->connect(g_ssid, g_pass, NSAPI_SECURITY_WPA2);
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
    
    is_connect_wifi = true;
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
          char filename[PATH_MAX];
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

                  char *reqbuf = new char[len+strlen(s_boundary)+strlen(s_contdisp)+strlen(s_conttype)+strlen(e_boundary)];
                  char *imgbuf = new char[len];
                  
                  memcpy(reqbuf, s_boundary, strlen(s_boundary));
                  memcpy(&reqbuf[strlen(s_boundary)], s_contdisp, strlen(s_contdisp));
                  memcpy(&reqbuf[strlen(s_boundary)+strlen(s_contdisp)], s_conttype, strlen(s_conttype));
                  fread( imgbuf, 1, len, fp);
                  fclose(fp);
                  memcpy(&reqbuf[strlen(s_boundary)+strlen(s_contdisp)+strlen(s_conttype)], imgbuf, len);
                  memcpy(&reqbuf[strlen(s_boundary)+strlen(s_contdisp)+strlen(s_conttype)+len], e_boundary, strlen(e_boundary));
                  
                  // make http request
                  char *url;
                  
                  if( strlen(g_post_url) > 10 )
                  {
                    url = (char*)g_post_url;
                  }
                  else
                  {
                    url = (char*)WATSON_POST_URL;
                  }
                  HttpRequest* request = new HttpRequest(m_pWifiInterface, HTTP_POST, url);
 
                  request->set_header("Content-Type", "multipart/form-data; boundary=----WebKitFormBoundaryZRUHAuSd1pDiYfK5");
                  request->set_header("Connection", "keep-alive");
                  request->set_header("User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/54.0.2840.99 Safari/537.36");
                  HttpResponse* response;
                  
                  // send request
                  response = request->send(reqbuf, strlen(s_boundary)+strlen(s_contdisp)+strlen(s_conttype)+len+strlen(e_boundary));
                  log("POCHI", LOGLEVEL_INFO, "POST FILE SIZE = %d\r\n", strlen(s_boundary)+strlen(s_contdisp)+strlen(s_conttype)+len+strlen(e_boundary));

                  if(response != NULL)
                  {

                      
                      log("POCHI", LOGLEVEL_INFO, "RESPONSE status is %d - %s\r\n", response->get_status_code(), response->get_status_message().c_str());
                      log("POCHI", LOGLEVEL_INFO, "RESPONSE body is:\r\n[%s]\r\n", response->get_body_as_string().c_str());
                      //printf("RESPONSE body is:\r\n[%s]\r\n", response->get_body_as_string().c_str());
#if 0
                      Thread::wait(2000);
                      
                      char *watson_resp = (char*)response->get_body_as_string().c_str();
                      printf("watson_resp:%d\r\n", strlen(watson_resp));
                      
                      int sendlen = strlen(s_boundary)+strlen(s_contdisp)+strlen(s_conttype)+len+strlen(e_boundary);
                      
                      char s_jsondisp[] = "Content-Disposition: form-data; name=\"jsonString\"; filename=\"jsonString.json\"\r\n";
                      char s_jsontype[] = "Content-Type: application/json\r\n\r\n";
                      
                      char *reqbuf2 = new char[sendlen+strlen(s_boundary)+strlen(s_jsondisp)+strlen(s_jsontype)+strlen(watson_resp)+strlen(s_boundary)];
                      int reqlen;
                      
                      memcpy(reqbuf2, s_boundary, reqlen);
                      reqlen = strlen(s_boundary);
                      
                      memcpy(&reqbuf2[reqlen], s_jsondisp, strlen(s_jsondisp));
                      reqlen += strlen(s_jsondisp);
                      
                      memcpy(&reqbuf2[reqlen], s_jsontype, strlen(s_jsontype));
                      reqlen += strlen(s_jsontype);
                      
                      memcpy(&reqbuf2[reqlen], watson_resp, strlen(watson_resp));
                      reqlen += strlen(watson_resp);
                      
                      memcpy(&reqbuf2[reqlen], s_boundary, strlen(s_boundary));
                      reqlen += strlen(s_boundary);
                      
                      memcpy(&reqbuf2[reqlen], s_contdisp, strlen(s_contdisp));
                      reqlen += strlen(s_contdisp);
                      
                      memcpy(&reqbuf2[reqlen], s_conttype, strlen(s_conttype));
                      reqlen += strlen(s_conttype);
                      
                      memcpy(&reqbuf2[reqlen], imgbuf, len);
                      reqlen += len;
                      
                      memcpy(&reqbuf2[reqlen], e_boundary, strlen(e_boundary));
                      reqlen += strlen(e_boundary);

                      //HttpRequest* request2 = new HttpRequest(m_pWifiInterface, HTTP_POST, "http://posttestserver.com/post.php");
                      log("POCHI", LOGLEVEL_INFO, "MAKE REQ\r\n");
                      HttpRequest* request2 = new HttpRequest(m_pWifiInterface, HTTP_POST, "http://104.199.222.173/r-king/pochi/webapi.php");
                      request2->set_header("Content-Type", "multipart/form-data; boundary=----WebKitFormBoundaryZRUHAuSd1pDiYfK5");
                      request2->set_header("Connection", "keep-alive");
                      request2->set_header("User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/54.0.2840.99 Safari/537.36");
                      HttpResponse* response2;
                      
                      log("POCHI", LOGLEVEL_INFO, "SEND REQ\r\n");
                      response2 = request2->send(reqbuf2, reqlen);
                      
                      if(response2 != NULL)
                      {
                        log("POCHI", LOGLEVEL_INFO, "RESPONSE status is %d - %s\r\n", response2->get_status_code(), response2->get_status_message().c_str());
                        log("POCHI", LOGLEVEL_INFO, "RESPONSE body is:\r\n[%s]\r\n", response2->get_body_as_string().c_str());
                        //printf("RESPONSE body is:\r\n[%s]\r\n", response2->get_body_as_string().c_str());
                      }
                      else
                      {
                        log("POCHI", LOGLEVEL_WARN, "RESPONSE2 IS NULL!\r\n");
                      }
                      delete request2;
                      delete[] reqbuf2;
#endif
                  }
                  else
                  {
                      log("POCHI", LOGLEVEL_WARN, "RESPONSE IS NULL!\r\n");
                  }
                   
                  delete request; // also clears out the response
                  delete[] reqbuf;
                  delete[] imgbuf;
              }
          }
      }
      Thread::wait(10000);
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
