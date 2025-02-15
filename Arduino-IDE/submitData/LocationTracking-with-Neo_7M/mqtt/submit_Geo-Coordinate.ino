
#include <Arduino.h>

unsigned int gpsLocationSubmit_interval = 2000; // time in second
unsigned int heartbeat_interval = 5000;         // time in second

#include <WiFi.h>             //library to handle wifi connection
#include <PubSubClient.h>     // library to establish mqtt connection
#include <WiFiClientSecure.h> //library to maintain the secure connection
#include <ArduinoJson.h>      // Include the Arduino library to make json or abstract the value from the json
#include <TimeLib.h>          // Include the Time library to handle time synchronization with ATS (Anedya Time Services)
#include <TinyGPSPlus.h>
#include <SoftwareSerial.h>

// ----------------------------- Anedya and Wifi credentials --------------------------------------------
String REGION_CODE = "ap-in-1";                   // Anedya region code (e.g., "ap-in-1" for Asia-Pacific/India) | For other country code, visity [https://docs.anedya.io/device/#region]
const char *CONNECTION_KEY = "";  // Fill your connection key, that you can get from your node description
const char *PHYSICAL_DEVICE_ID = ""; // Fill your device Id , that you can get from your node description
const char *SSID = "";     
const char *PASSWORD = ""; 

 
//---------------------GPS-----------------
static const int RXPin = 5, TXPin = 18;

static const uint32_t GPSBaud = 9600;
double latitude, longitude;

// -------------------------MQTT connection settings---------------------
String str_mqtt_broker = "mqtt." + REGION_CODE + ".anedya.io";
const char *mqtt_broker = str_mqtt_broker.c_str();                                   // MQTT broker address
const char *mqtt_username = PHYSICAL_DEVICE_ID;                                      // MQTT username
const char *mqtt_password = CONNECTION_KEY;                                          // MQTT password
const int mqtt_port = 8883;                                                          // MQTT port
String responseTopic = "$anedya/device/" + String(PHYSICAL_DEVICE_ID) + "/response"; // MQTT topic for device responses
String errorTopic = "$anedya/device/" + String(PHYSICAL_DEVICE_ID) + "/errors";      // MQTT topic for device errors

// Root CA Certificate
// fill anedya root certificate. it can be get from [https://docs.anedya.io/device/mqtt-endpoints/#tls]
const char *ca_cert = R"EOF(                           
-----BEGIN CERTIFICATE-----
MIICDDCCAbOgAwIBAgITQxd3Dqj4u/74GrImxc0M4EbUvDAKBggqhkjOPQQDAjBL
MQswCQYDVQQGEwJJTjEQMA4GA1UECBMHR3VqYXJhdDEPMA0GA1UEChMGQW5lZHlh
MRkwFwYDVQQDExBBbmVkeWEgUm9vdCBDQSAzMB4XDTI0MDEwMTAwMDAwMFoXDTQz
MTIzMTIzNTk1OVowSzELMAkGA1UEBhMCSU4xEDAOBgNVBAgTB0d1amFyYXQxDzAN
BgNVBAoTBkFuZWR5YTEZMBcGA1UEAxMQQW5lZHlhIFJvb3QgQ0EgMzBZMBMGByqG
SM49AgEGCCqGSM49AwEHA0IABKsxf0vpbjShIOIGweak0/meIYS0AmXaujinCjFk
BFShcaf2MdMeYBPPFwz4p5I8KOCopgshSTUFRCXiiKwgYPKjdjB0MA8GA1UdEwEB
/wQFMAMBAf8wHQYDVR0OBBYEFNz1PBRXdRsYQNVsd3eYVNdRDcH4MB8GA1UdIwQY
MBaAFNz1PBRXdRsYQNVsd3eYVNdRDcH4MA4GA1UdDwEB/wQEAwIBhjARBgNVHSAE
CjAIMAYGBFUdIAAwCgYIKoZIzj0EAwIDRwAwRAIgR/rWSG8+L4XtFLces0JYS7bY
5NH1diiFk54/E5xmSaICIEYYbhvjrdR0GVLjoay6gFspiRZ7GtDDr9xF91WbsK0P
-----END CERTIFICATE-----
)EOF";

//-----------------------------Helper variable------------------------------
long long submitTimer, submitInterval, lastSubmittedHeatbeat_timestamp; // timer variable for request handling
String timeRes, submitRes;                                              // variable to store the response

//---------------------------object initiazation--------------------------
// The TinyGPSPlus object
TinyGPSPlus gps;
// The serial connection to the GPS device
SoftwareSerial ss(RXPin, TXPin);
// WiFi and MQTT client initialization
WiFiClientSecure esp_client;
PubSubClient mqtt_client(esp_client);

//----------------------------- Function Declarations ----------------------------------------------------
void connectToWiFi();
void connectToMQTT();
void mqttCallback(char *topic, byte *payload, unsigned int length);
void setDevice_time();                                                                     // Function to configure the device time with real-time from ATS (Anedya Time Services)
void anedya_submitLocation(String VARIABLE_IDENTIFIER, double LATITUDE, double LONGITUDE); // Function to submit data to the Anedya server
void anedya_sendHeartbeat();

void setup()
{
  Serial.begin(115200); // Initialize serial communication with  your device compatible baud rate
  delay(150);           // Delay for 1.5 seconds
  ss.begin(GPSBaud);
  delay(150); // Delay for 1.5 seconds

  connectToWiFi();

  submitTimer = millis();
  submitInterval = millis();
  lastSubmittedHeatbeat_timestamp = millis();
  // Set Root CA certificate
  esp_client.setCACert(ca_cert);
  mqtt_client.setServer(mqtt_broker, mqtt_port); // Set the MQTT server address and port for the MQTT client to connect to anedya broker
  mqtt_client.setKeepAlive(60);                  // Set the keep alive interval (in seconds) for the MQTT connection to maintain connectivity
  mqtt_client.setCallback(mqttCallback);         // Set the callback function to be invoked when MQTT messages are received
  connectToMQTT();                               // Attempt to establish a connection to the anedya broker

  setDevice_time();
}

void loop()
{
  while (ss.available() > 0)
  {
    if (gps.encode(ss.read()))
    {
      if (gps.location.isValid())
      {
        latitude = gps.location.lat();
        longitude = gps.location.lng();
      }
      else
      {
        Serial.print(F("INVALID"));
        latitude = 0;
        longitude = 0;
      }
    }
    if ((millis() - submitInterval > gpsLocationSubmit_interval) && latitude != 0)
    {
      Serial.print("Location : ");
      Serial.print(latitude, 7);
      Serial.print(F(","));
      Serial.println(longitude, 7);

      anedya_submitLocation("location", latitude, longitude);
      latitude = 0;
      longitude = 0;
      submitInterval = millis();
    }
    if (millis() - lastSubmittedHeatbeat_timestamp > heartbeat_interval)
    {
      anedya_sendHeartbeat();
      lastSubmittedHeatbeat_timestamp = millis();
    }
  }
  if (millis() > 5000 && gps.charsProcessed() < 10)
  {
    Serial.println(F("No GPS detected: check wiring."));
    delay(4000);
  }
      if (millis() - lastSubmittedHeatbeat_timestamp > heartbeat_interval)
    {
      anedya_sendHeartbeat();
      lastSubmittedHeatbeat_timestamp = millis();
    }
}
//<---------------------------------------------------------------------------------------------------------------------------->
void connectToWiFi()
{
  // Connect to WiFi network
  WiFi.begin(SSID, PASSWORD);
  Serial.println();
  Serial.print("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Connected, IP address: ");
  Serial.println(WiFi.localIP());
}
void connectToMQTT()
{
  while (!mqtt_client.connected())
  {
    const char *client_id = PHYSICAL_DEVICE_ID;
    Serial.print("Connecting to Anedya Broker....... ");
    if (mqtt_client.connect(client_id, mqtt_username, mqtt_password))
    {
      Serial.println("Connected to Anedya broker");
      mqtt_client.subscribe(responseTopic.c_str()); // subscribe to get response
      mqtt_client.subscribe(errorTopic.c_str());    // subscibe to get error
    }
    else
    {
      Serial.print("Failed to connect to Anedya broker, rc=");
      Serial.print(mqtt_client.state());
      Serial.println(" Retrying in 5 seconds.");
      delay(5000);
    }
  }
}
void mqttCallback(char *topic, byte *payload, unsigned int length)
{
  // Serial.print("Message received on topic: ");
  // Serial.println(topic);
  char res[150] = "";

  for (unsigned int i = 0; i < length; i++)
  {
    // Serial.print((char)payload[i]);
    //   Serial.print(payload[i]);
    res[i] = payload[i];
  }
  String str_res(res);
  if (str_res.indexOf("deviceSendTime") != -1)
  {
    timeRes = str_res;
  }
  else
  {
    // Serial.println(str_res);
    submitRes = str_res;
  }
}
// Function to configure time synchronization with Anedya server
// For more info, visit [https://docs.anedya.io/device/api/http-time-sync/]
void setDevice_time()
{
  String timeTopic = "$anedya/device/" + String(PHYSICAL_DEVICE_ID) + "/time/json";
  const char *mqtt_topic = timeTopic.c_str();
  // Attempt to synchronize time with Anedya server
  if (mqtt_client.connected())
  {

    Serial.print("Time synchronizing......");

    boolean timeCheck = true; // iteration to re-sync to ATS (Anedya Time Services), in case of failed attempt
    // Get the device send time

    long long deviceSendTime;
    long long timeTimer = millis();
    while (timeCheck)
    {
      mqtt_client.loop();

      unsigned int iterate = 2000;
      if (millis() - timeTimer >= iterate)
      {
        Serial.print(" .");
        timeTimer = millis();
        deviceSendTime = millis();

        // Prepare the request payload
        JsonDocument requestPayload;                       // Declare a JSON document with a capacity of 200 bytes
        requestPayload["deviceSendTime"] = deviceSendTime; // Add a key-value pair to the JSON document
        String jsonPayload;                                // Declare a string to store the serialized JSON payload
        serializeJson(requestPayload, jsonPayload);        // Serialize the JSON document into a string
        // Convert String object to pointer to a string literal
        const char *jsonPayloadLiteral = jsonPayload.c_str();
        mqtt_client.publish(mqtt_topic, jsonPayloadLiteral);
      } // if end

      if (timeRes != "")
      {
        String strResTime(timeRes);

        // Parse the JSON response
        JsonDocument jsonResponse;                 // Declare a JSON document with a capacity of 200 bytes
        deserializeJson(jsonResponse, strResTime); // Deserialize the JSON response from the server into the JSON document

        long long serverReceiveTime = jsonResponse["serverReceiveTime"]; // Get the server receive time from the JSON document
        long long serverSendTime = jsonResponse["serverSendTime"];       // Get the server send time from the JSON document

        // Compute the current time
        long long deviceRecTime = millis();                                                                // Get the device receive time
        long long currentTime = (serverReceiveTime + serverSendTime + deviceRecTime - deviceSendTime) / 2; // Compute the current time
        long long currentTimeSeconds = currentTime / 1000;                                                 // Convert current time to seconds

        // Set device time
        setTime(currentTimeSeconds); // Set the device time based on the computed current time
        Serial.println("\n synchronized!");
        timeCheck = false;
      } // response check
    } // while loop end
  }
  else
  {
    connectToMQTT();
  } // mqtt connect check end
} // set device time function end

// Function to submit Location to Anedya server
// For more info, visit [https://docs.anedya.io/devicehttpapi/submitdata/]
void anedya_submitLocation(String VARIABLE_IDENTIFIER, double LATITUDE, double LONGITUDE)
{
  boolean check = true;

  String strSubmitTopic = "$anedya/device/" + String(PHYSICAL_DEVICE_ID) + "/submitdata/json";
  const char *submitTopic = strSubmitTopic.c_str();
  while (check)
  {
    if (mqtt_client.connected())
    {
      if (millis() - submitTimer > 2000)
      {
        submitTimer = millis();
        // Get current time and convert it to milliseconds
        long long current_time = now();                     // Get the current time
        long long current_time_milli = current_time * 1000; // Convert current time to milliseconds

        // Construct the JSON payload with sensor data and timestamp

        char jsonstr[1000];
        sprintf(jsonstr, "{\"data\":[{\"variable\": \"%s\",\"value\":{\"lat\":%0.7f ,\"long\": %0.7f },\"timestamp\": %lld }]}", VARIABLE_IDENTIFIER, latitude, longitude, current_time_milli);
        Serial.println(jsonstr);

        const char *submitJsonPayload = jsonstr;
        mqtt_client.publish(submitTopic, submitJsonPayload);
      }
      mqtt_client.loop();
      if (submitRes != "")
      {
        // Parse the JSON response
        JsonDocument jsonResponse;                // Declare a JSON document with a capacity of 200 bytes
        deserializeJson(jsonResponse, submitRes); // Deserialize the JSON response from the server into the JSON document

        int errorCode = jsonResponse["errorcode"]; // Get the server receive time from the JSON document
        if (errorCode == 0)
        {
          Serial.println("Data pushed to Anedya!!");
        }
        else if (errorCode == 4040)
        {
          Serial.println("Failed to push data!!");
          Serial.println("unknown variable Identifier");
          Serial.println(submitRes);
        }
        else
        {
          Serial.println("Failed to push data!!");
          Serial.println(submitRes);
        }
        check = false;
        submitTimer = 5000;
      }
    }
    else
    {
      connectToMQTT();
    } // mqtt connect check end
  }
}

void anedya_sendHeartbeat()
{
  if (WiFi.status() != WL_CONNECTED)
  {
    connectToWiFi();
  }
  mqtt_client.connected() ? (void)0 : connectToMQTT();

  String strSubmitTopic = "$anedya/device/" + String(PHYSICAL_DEVICE_ID) + "/heartbeat/json";
  const char *submitTopic = strSubmitTopic.c_str();

  String strLog = "{}";
  const char *submitLogPayload = strLog.c_str();
  mqtt_client.publish(submitTopic, submitLogPayload);
  mqtt_client.loop();
}