/*!

  Mini-Tablet



    \brief     ESP32-SubscriberCounter
    \details   Displays the number of subscibers for the given account
    \author    Richard van der Weerd
    \version   0.1
    \date      04-03-2018
    \pre       First initialize the system.
    \bug       No bugs discovered yet
    \warning   No warnings yet
    \copyright MIT Public License.
*/

#define WIFI_HOSTNAME "SubscrCounter"

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <U8g2lib.h>
#include <ArduinoJson.h>

#include <Credentials.h>        // comment this line out if you don't have a credentials file

U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(U8G2_R0, 4, 5);

#ifdef CREDENTIALS_H

// wifi
const char* ssid = WIFI_SSID;
const char* password = WIFI_PASS;

// Youtube
String apiKey = YOUTUBE_APIKEY;                      // YouTube Data API v3 key generated here: https://console.developers.google.com
String channelId = YOUTUBE_CHANNEL_ID;               // YouTube channel id: https://www.youtube.com/account_advanced
const char *host = "www.googleapis.com";

#else
// wifi
const char* ssid = "your network ssid";
const char* password = "your password";

//Youtube
String apiKey = "your api key";                      // YouTube Data API v3 key generated here: https://console.developers.google.com
String channelId = "your youtube channel id";        // YouTube channel id: https://www.youtube.com/account_advanced
const char *host = "www.googleapis.com";

#endif

/*******************************************************************************

   SETUP

 *******************************************************************************/

void setup() {
  // Open serial communications and wait for port to open:
  Serial.begin(115200);

  // setup Oled screen
  u8g2.begin();
  u8g2.setFont(u8g2_font_ncenB12_tf);

  u8g2.clearBuffer();
  u8g2.drawStr(10, 15, "Please wait...");
  u8g2.sendBuffer();

  setupWifi();
}


/*******************************************************************************

   LOOP

 *******************************************************************************/

void loop() {
  if( WiFi.status() != WL_CONNECTED) {
    Serial.println("No connection. Resetting...");
    ESP.restart();
  }
  updateOLED(getSubscribers());
  delay(60000);
}


/*******************************************************************************

   SETUP WIFI

 *******************************************************************************/

void setupWifi() {
  // attempt to connect to Wifi network:
//  WiFi.setHostname(WIFI_HOSTNAME);
  WiFi.begin(ssid, password);
  while ( WiFi.status() != WL_CONNECTED ) {
    delay ( 500 );
    Serial.print(".");
  }
  Serial.println();
  Serial.println(WiFi.localIP());
}

/*******************************************************************************

   GET SUBSCIBERS

 *******************************************************************************/

int getSubscribers()
{
  WiFiClientSecure client;
  Serial.print("connecting to "); Serial.println(host);
  if (!client.connect(host, 443)) {
    Serial.println("connection failed");
    return -1;
  }
  String cmd = String("GET /youtube/v3/channels?part=statistics&id=") + channelId + "&key=" + apiKey + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\nUser-Agent: ESP8266/1.1\r\nConnection: close\r\n\r\n";
  client.print(cmd);

  int repeatCounter = 10;
  while (!client.available() && repeatCounter--) {
    delay(500);
  }
  String line, buf = "";
  int startJson = 0;

  while (client.connected() && client.available()) {
    line = client.readStringUntil('\n');
    if (line[0] == '{') startJson = 1;
    if (startJson)
    {
      for (int i = 0; i < line.length(); i++)
        if (line[i] == '[' || line[i] == ']') line[i] = ' ';
      buf += line + "\n";
    }
  }
  client.stop();

  DynamicJsonBuffer jsonBuf;
  JsonObject &root = jsonBuf.parseObject(buf);
  if (!root.success()) {
    Serial.println("parseObject() failed");
    delay(10);
    return -1;
  }

  int subscribers = root["items"]["statistics"]["subscriberCount"];
  Serial.println(subscribers);
  return subscribers;
}

/*******************************************************************************

   UPDATE OLED

 *******************************************************************************/

void updateOLED(uint32_t counter) {
  static uint32_t oldCounter = 0;
  if (counter != oldCounter || counter == 0) {
    char textBuffer[10];
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_ncenB08_tf);
    u8g2.drawStr(10, 62, "youtube subscibers");
    
    u8g2.setFont(u8g2_font_ncenB24_tf);
    if(counter > 4200000000)
      sprintf(textBuffer, "%s", "?");
    else
      sprintf(textBuffer, "%d", counter);
    uint16_t stringWidth = strlen(textBuffer) * 12;
    u8g2.drawStr(64 - stringWidth, 32 + 24 / 2 - 8, textBuffer);
    u8g2.sendBuffer();
  }
}

