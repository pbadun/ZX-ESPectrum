#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <HTTPClient.h>
#include <time.h>

const char *ssid = "nhome";
const char *pass = "swisscq1";
HTTPClient client;

void initWifI();

void setup_()
{
  Serial.begin(115200);
  delay(10);
  Serial.println('\n');
  Serial.println("Start...");
  initWifI();
}

void initWifI()
{
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED)
  { // Wait for the Wi-Fi to connect
    delay(500);
    Serial.print('.');
  }
  Serial.println('\n');
  Serial.println("Connection established");
  Serial.print("IP address:\t");
  Serial.println(WiFi.localIP());
}

void loop_()
{
  client.begin("http://esp.bb8qq.com/");
  int htmlCode = client.GET();
  if (htmlCode > 0)
  {
    String body = client.getString();
    Serial.println(body);
  }
  else
  {
    Serial.println("Error connect");
  }
  client.end();
  delay(3000);
}