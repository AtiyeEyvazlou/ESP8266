#include <ESP8266WiFi.h>
#include <PubSubClient.h>


#define ADC_MIN_VALUE 13
#define ADC_MAX_VALUE 245

const int8_t tblTemperatureSensor[] = {
    91, 83, 77, 72, 68, 65, 62, 59, 56, 54,
    52, 50, 48, 46, 45, 43, 42, 41, 39, 38,
    37, 36, 35, 34, 33, 32, 31, 30, 29, 28,
    28, 27, 26, 25, 25, 24, 23, 22, 22, 21,
    21, 20, 19, 19, 18, 18, 17, 17, 16, 15,
    15, 14, 14, 13, 13, 13, 12, 12, 11, 11,
    10, 10, 9, 9, 8, 8, 8, 7, 7, 6,
    6, 6, 5, 5, 4, 4, 4, 3, 3, 3,
    2, 2, 2, 1, 1, 1, 0, 0, -1, -1,
    -1, -2, -2, -2, -3, -3, -3, -4, -4, -4,
    -4, -5, -5, -5, -6, -6, -6, -7, -7, -7,
    -8, -8, -8, -8, -9, -9, -9, -10, -10, -10,
    -11, -11, -11, -11, -12, -12, -12, -13, -13, -13,
    -14, -14, -14, -14, -15, -15, -15, -16, -16, -16,
    -16, -17, -17, -17, -18, -18, -18, -19, -19, -19,
    -19, -20, -20, -20, -21, -21, -21, -22, -22, -22,
    -22, -23, -23, -23, -24, -24, -24, -25, -25, -25,
    -26, -26, -26, -27, -27, -27, -28, -28, -28, -29,
    -29, -29, -30, -30, -30, -31, -31, -32, -32, -32,
    -33, -33, -34, -34, -34, -35, -35, -36, -36, -36,
    -37, -37, -38, -38, -39, -39, -40, -40, -41, -41,
    -42, -42, -43, -43, -44, -45, -45, -46, -46, -47,
    -48, -49, -49, -50, -51, -52, -53, -54, -55, -56,
    -57, -59, -60
};


struct WiFiConfig {
  const char *ssid;
  const char *password;
};

WiFiConfig wifi = { "your_ssid", "your_password" };

struct MQTTConfig {
  const char *broker;
  const char *topic;
  const char *username;
  const char *password;
  int port;
};

MQTTConfig mqtt = { "broker.emqx.io", 
                    "emqx/temp3", 
                    "emqx",
                    "public",
                    8883 };

struct SensorConfig {
  const int pin;         // Sensor Pin
  const int numReadings; // Number of samples for moving average
  int readings[4];       // Array to store readings 
  int currentIndex;      // Current index in array
  long sum;              // Sum of readings
  int average;           // Calculated average
  float temperature;     // Temperature value

  SensorConfig() 
    : pin(A0), numReadings(4), currentIndex(0), sum(0), average(0), temperature(0.0) {
    for (int i = 0; i < numReadings; i++) {
      readings[i] = 0;
    }
  }
};
                    
bool  wifi_sleeping = false;
volatile bool shouldWakeUp = false;

SensorConfig sensor;
WiFiClientSecure espClient;
PubSubClient client(espClient);

const char fingerprint[] PROGMEM = "9B:98:97:3C:2B:08:5A:83:23:0C:18:B9:34:14:61:07:F6:59:78:63";

//*******************************Functions**************************************//
void setup() 
{
  Serial.begin(115200);
  wiFi_INIT();
  mqtt_INIT();
}

void loop() 
{  
  Serial.flush();
  
  goToSleep();
  
  if (shouldWakeUp) {
    shouldWakeUp = false; 
    wakeup();         
  }
  delay(100);
}

//*****************************************************************************//
void callback()
{
  ESP.wdtFeed();
  shouldWakeUp = true;
  Serial.flush();
}
//*****************************************************************************//
void goToSleep()
{
    WiFi.disconnect();
    WiFi.forceSleepBegin();
    delay(200);
    
    uint32_t sleep_time_in_ms = 1000;
    
    wifi_set_opmode(NULL_MODE);
    wifi_fpm_set_sleep_type(LIGHT_SLEEP_T);
    wifi_fpm_open();
    wifi_fpm_set_wakeup_cb(callback);
    wifi_sleeping = true;
    wifi_fpm_do_sleep(sleep_time_in_ms * 1000); 
    wifi_sleeping = false;
    esp_delay(sleep_time_in_ms + 1, [](){ return wifi_sleeping; });
  
}
//*****************************************************************************//
void wakeup()
{
  WiFi.forceSleepWake();
  Serial.println("Waking up...");
  delay(10);
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("connecting to wifi...");
    wiFi_INIT();
  }
  mqtt_INIT();
  read_average_temp();
  publish_temp();
}
//*****************************************************************************//
void publish_temp()
{    
  if (client.publish(mqtt.topic,String(sensor.temperature).c_str() )) 
  {
    Serial.print("Published temp = ");
    Serial.println(sensor.temperature);
  } 
  else 
  {
    Serial.println("Message publish failed!");
  }
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
void wifi_reconnect()
{
  Serial.println("wifi_reconnect...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(wifi.ssid, wifi.password);
  while (WiFi.status() != WL_CONNECTED) 
  {
    Serial.print(".");
    delay(100);
  }
    Serial.println("wifi connected ...");
}
//*****************************************************************************//
void mqtt_INIT() 
{
  Serial.println("mqtt init...");
  espClient.setFingerprint(fingerprint);
  client.setServer(mqtt.broker, mqtt.port);
  
  String client_id = "esp8266-client-";
  client_id += String(WiFi.macAddress());
  
  if (client.connect(client_id.c_str(), mqtt.username, mqtt.password)) 
  {
    Serial.println("MQTT broker connected");
  } 
  else
  {
    Serial.print("MQTT connection failed, retrying...");
  }
}
//*****************************************************************************//
void mqtt_reconnect()
{
  espClient.setFingerprint(fingerprint);
  client.setServer(mqtt.broker, mqtt.port);
  if (client.connect("ididit", mqtt.username, mqtt.password)) 
  {
    Serial.println("MQTT broker connected");
  }
  else
  {
    Serial.print("MQTT connection failed, retrying...");
  }
}
//*****************************************************************************//
void read_average_temp(){ 

    int newReading = analogRead(sensor.pin);
    sensor.sum -= sensor.readings[sensor.currentIndex];
    sensor.readings[sensor.currentIndex] = newReading;
    sensor.sum += newReading;
    sensor.currentIndex = (sensor.currentIndex + 1) % sensor.numReadings;
    sensor.average = sensor.sum >> 2 ;
    sensor.temperature =   rawadc2celsius(sensor.average>>2); // shifted because the used LUT is calibrated for 8 bit adc
}
//*****************************************************************************//
double rawadc2celsius(int adcValue) {
  
    if (adcValue < ADC_MIN_VALUE) {
      return 99;
    } 
    else if (adcValue > ADC_MAX_VALUE) {
      return -99;
    } 
    else {
        return tblTemperatureSensor[adcValue - ADC_MIN_VALUE]; 
    }
}
//*****************************End of code*************************************//
