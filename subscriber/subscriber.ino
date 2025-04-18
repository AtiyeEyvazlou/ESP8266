#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Ticker.h>
#include <ESPAsyncWebServer.h>
#include <EEPROM.h>
#include "certificate.h"

#define EEPROM_SIZE 256  

struct WiFiConfig {
  const char *ssid;
  const char *password;
};

WiFiConfig wifi = { "your_ssid", "your_password" };


struct MQTTConfig {
  char broker[32];
  const char *topic1;
  const char *topic2;
  const char *topic3;
  const char *username;
  const char *password;
  int port;
};


MQTTConfig mqtt = { "broker.emqx.io", 
                    "emqx/temp1", 
                    "emqx/temp2",
                    "emqx/temp3",
                    "emqx",
                    "public",
                    8883 };

struct PayloadConfig {
  int temp1;
  int temp2;
  int dtemp;
  unsigned int etemp1;
  unsigned int etemp2;
  };

PayloadConfig payload = {0, 0, 0, 0, 0}; 

BearSSL::X509List *serverTrustedCA = nullptr;
BearSSL::WiFiClientSecure espClient;
PubSubClient client(espClient);
Ticker publishTempDiff;
AsyncWebServer server(80);


//*******************************Functions**************************************//
void setup() 
{
  Serial.begin(115200);
  
  wiFi_INIT();
  loadBrokerIP();  
  serverTrustedCA = new BearSSL::X509List(ca_cert);
  mqtt_INIT();
  publishTempDiff.attach(1,publish_temp); 
  startServer();
  ESP.wdtEnable(1500); 
}
//*****************************************************************************//
void loop() 
{
  if (!client.connected()) {
      Serial.println("MQTT disconnected, attempting to reconnect...");
      mqtt_INIT();
    }
    client.loop();
    yield();  
}
//*****************************************************************************//
void loadBrokerIP() 
{
  EEPROM.begin(EEPROM_SIZE);
  char firstChar = EEPROM.read(0);
  if (firstChar == 0xFF || firstChar == 0) 
  { 
    String defaultIP = "broker.emqx.io";
    strncpy(mqtt.broker, defaultIP.c_str(), 32);  
    mqtt.broker[31] = '\0';  
    saveBrokerIP(defaultIP);  
  }
  else 
  {
    for (int i = 0; i < 32; i++) 
    {
      mqtt.broker[i] = EEPROM.read(i);
      if (mqtt.broker[i] == '\0') break; 
    }
  }
  EEPROM.end();
  Serial.print("Loaded MQTT Broker: ");
  Serial.println(mqtt.broker);
}
//*****************************************************************************//
void saveBrokerIP(String newIP) 
{
  EEPROM.begin(EEPROM_SIZE);
  for (int i = 0; i < newIP.length() && i < 31; i++)
  {
    EEPROM.write(i, newIP[i]);
  }
  EEPROM.write(newIP.length() < 31 ? newIP.length() : 31, '\0');  
  EEPROM.commit();
  EEPROM.end();
  strncpy(mqtt.broker, newIP.c_str(), 32);  
  mqtt.broker[31] = '\0';  
  Serial.print("Saved and updated MQTT Broker: ");
  Serial.println(mqtt.broker);
}
//*****************************************************************************//
// Start the web server
void startServer() 
{
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) 
  {
    String html = "<form action='/setip' method='POST'>"
                  "MQTT Broker: <input type='text' name='ip' value='" + String(mqtt.broker) + "'>"
                  "<input type='submit' value='Save'></form>";
    request->send(200, "text/html", html);
  });

  server.on("/setip", HTTP_POST, [](AsyncWebServerRequest *request) 
  {
    if (request->hasParam("ip", true)) 
    {
      String newIP = request->getParam("ip", true)->value();
      saveBrokerIP(newIP);
      request->send(200, "text/html", "Broker IP saved! Rebooting...");
      delay(1000);
      ESP.restart();
    }
  });

  server.begin();
}
//*****************************************************************************//
void wiFi_INIT() 
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(wifi.ssid, wifi.password);
  
  Serial.println("\nConnecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) 
  {
    Serial.print(".");
    delay(100);
  }
  Serial.println("\nConnected to WiFi network");
  Serial.print("ESP8266 IP Address: ");
  Serial.println(WiFi.localIP());
}
//*****************************************************************************//
void mqtt_INIT() 
{
  espClient.setTrustAnchors(serverTrustedCA);
  setClock(); // Required for X.509 validation
  
  client.setServer(mqtt.broker, mqtt.port);
  client.setCallback(callback);
  
  String client_id = "esp8266-client-";
  client_id += String(WiFi.macAddress());
 
  if (client.connect(client_id.c_str(), mqtt.username, mqtt.password)) 
  {
    Serial.println("MQTT broker connected");
    client.publish(mqtt.topic3, "ESP8266 subscribed to temp1 and temp2");
  
    client.subscribe(mqtt.topic1);
    client.subscribe(mqtt.topic2);
  } 
  else 
  {
    Serial.print("MQTT connection failed, rc=");
    Serial.print(client.state());
    Serial.println(" - Retrying in 2s...");
    delay(2000);
  }
}
//*****************************************************************************//
void setClock()
{
  configTime( 4* 3600, 0, "pool.ntp.org", "time.nist.gov");

  Serial.print("Waiting for NTP time sync: ");
  time_t now = time(nullptr);
  while (now < 8 * 3600 * 2) 
  {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }
  Serial.println("");
  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
  Serial.print("Current time: ");
  Serial.print(asctime(&timeinfo));
}
//*****************************************************************************//
void callback(char *topic, byte *message, unsigned int length) 
{
  int num = 0;

  if (strcmp(topic, mqtt.topic1) == 0) 
  {  
    for (int i = 0; i < length; i++) { 
      if ((char)message[i] != '0' && (char)message[i] != '.') {  
        num = num * 10 + ((char)message[i] - '0'); 
      }
    }
    payload.temp1 = num;  
    num = 0; 
    payload.etemp1 = 0;
  }
  else if (strcmp(topic, mqtt.topic2) == 0) 
  {  
    for (int i = 0; i < length; i++) { 
      if ((char)message[i] != '0' && (char)message[i] != '.') {  
        num = num * 10 + ((char)message[i] - '0');  
      }
    }
    payload.temp2 = num;  
    num = 0;  
    payload.etemp2 = 0;
  }
}
//*****************************************************************************//
void publish_temp()
{
    payload.dtemp = abs(payload.temp1 - payload.temp2);  

    if (client.publish(mqtt.topic3, String(payload.dtemp).c_str())) 
    {
    Serial.print("Published temp difference = ");
    Serial.println(payload.dtemp);
    } 
    else 
    {
    Serial.println("Message publish failed!");
    }
    ESP.wdtFeed(); 
}
//*****************************End of code*************************************//
