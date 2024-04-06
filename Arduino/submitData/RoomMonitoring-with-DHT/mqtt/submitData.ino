/*

                            Room Monitoring Using ESP32 + DHT sensor (mqtt)
                Disclaimer: This code is for hobbyists for learning purposes. Not recommended for production use!!

                            # Dashboard Setup
                             - Create account and login to the dashboard
                             - Create new project.
                             - Create variables: temperature and humidity.
                             - Create a node (e.g., for home- Room1 or study room).
                            Note: Variable Identifier is essential; fill it accurately.

                            # Hardware Setup
                             - Properly identify your sensor's pins.
                             - Connect sensor VCC pin to 3V3.
                             - Connect sensor GND pin to GND.
                             - Connect sensor signal pin to D5.

                  Note: The code is tested on the "Esp-32 Wifi, Bluetooth, Dual Core Chip Development Board (ESP-WROOM-32)"

                                                                                           Dated: 26-March-2024
 
*/ 
#include <Arduino.h>

// Emulate Hardware Sensor?
bool virtual_sensor = true;

#include <WiFi.h>            //library to handle wifi connection 
#include <PubSubClient.h>     // library to establish mqtt connection
#include <WiFiClientSecure.h>  //library to maintain the secure connection 
#include <ArduinoJson.h> // Include the Arduino library to make json or abstract the value from the json
#include <TimeLib.h>     // Include the Time library to handle time synchronization with ATS (Anedya Time Services)
#include <DHT.h>         // Include the DHT library for humidity and temperature sensor handling

String regionCode = "ap-in-1";                   // Anedya region code (e.g., "ap-in-1" for Asia-Pacific/India) | For other country code, visity [https://docs.anedya.io/device/intro/#region]
const char *deviceID = "<PHYSICAL-DEVICE-UUID>"; // Fill your device Id , that you can get from your node description
const char *connectionkey = "<CONNECTION-KEY>";  // Fill your connection key, that you can get from your node description
// WiFi credentials
const char *ssid = "<SSID>";     // Replace with your WiFi name
const char *pass = "<PASSWORD>"; // Replace with your WiFi password

// MQTT connection settings
const char *mqtt_broker = "device.ap-in-1.anedya.io";  // MQTT broker address
const char *mqtt_username = deviceID;  // MQTT username
const char *mqtt_password = connectionkey;  // MQTT password
const int mqtt_port = 8883;  // MQTT port
String responseTopic = "$anedya/device/" + String(deviceID) + "/response";  // MQTT topic for device responses
String errorTopic = "$anedya/device/" + String(deviceID) + "/errors";  // MQTT topic for device errors

// Root CA Certificate
//fill anedya root certificate. it can be get from [https://docs.anedya.io/device/mqtt-endpoints/#tls]
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

long long submitTimer;   //timer variable for request handling
String timeRes, submitRes;  //variable to store the response 

// Define the type of DHT sensor (DHT11, DHT21, DHT22, AM2301, AM2302, AM2321)
#define DHT_TYPE DHT11
// Define the pin connected to the DHT sensor
#define DHT_PIN 5 // pin marked as D5 on the ESP32
float temperature;
float humidity;

// Function Declarations
void connectToMQTT();
void mqttCallback(char *topic, byte *payload, unsigned int length);
void setDevice_time();                                       // Function to configure the device time with real-time from ATS (Anedya Time Services)
void anedya_submitData(String datapoint, float sensor_data); // Function to submit data to the Anedya server

// WiFi and MQTT client initialization
WiFiClientSecure esp_client;
PubSubClient mqtt_client(esp_client);

// Create a DHT object
DHT dht(DHT_PIN, DHT_TYPE);

void setup()
{
  Serial.begin(115200); // Initialize serial communication with  your device compatible baud rate
  delay(1500);          // Delay for 1.5 seconds

  // Connect to WiFi network
  WiFi.begin(ssid, pass);
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

  submitTimer = millis();
  // Set Root CA certificate
  esp_client.setCACert(ca_cert);
mqtt_client.setServer(mqtt_broker, mqtt_port);  // Set the MQTT server address and port for the MQTT client to connect to anedya broker
mqtt_client.setKeepAlive(60);  // Set the keep alive interval (in seconds) for the MQTT connection to maintain connectivity
mqtt_client.setCallback(mqttCallback);  // Set the callback function to be invoked when MQTT messages are received
connectToMQTT(); // Attempt to establish a connection to the anedya broker

mqtt_client.subscribe(responseTopic.c_str());  //subscribe to get response
mqtt_client.subscribe(errorTopic.c_str());    //subscibe to get error

  setDevice_time();
  // Initialize the DHT sensor
  dht.begin();
}

void loop()
{

  if (!virtual_sensor)
  {
    // Read the temperature and humidity from the DHT sensor
    Serial.println("Fetching data from the Physical sensor");
    temperature = dht.readTemperature();
    humidity = dht.readHumidity();
    if (isnan(humidity) || isnan(temperature))
    {
      Serial.println("Failed to read from DHT !"); // Output error message to serial console
      delay(10000);
      return;
    }
  }
  else
  {
    // Generate random temperature and humidity values
    Serial.println("Fetching data from the Virtual sensor");
    temperature = random(20, 30);
    humidity = random(60, 80);
  }
  Serial.print("Temperature : ");
  Serial.println(temperature);

  // Submit sensor data to Anedya server
  anedya_submitData("temperature", temperature); // submit data to the Anedya

  Serial.print("Humidity : ");
  Serial.println(humidity);

  anedya_submitData("humidity", humidity); // submit data to the Anedya

  Serial.println("-------------------------------------------------");
  delay(5000);
}
//<---------------------------------------------------------------------------------------------------------------------------->
void connectToMQTT()
{
  while (!mqtt_client.connected())
  {
    const char* client_id = deviceID;
    Serial.print("Connecting to Anedya Broker....... ");
    if (mqtt_client.connect(client_id, mqtt_username, mqtt_password))
    {
      Serial.println("Connected to Anedya broker");
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
// For more info, visit [https://docs.anedya.io/devicehttpapi/http-time-sync/]
void setDevice_time()
{
  String timeTopic = "$anedya/device/" + String(deviceID) + "/time/json";
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
        StaticJsonDocument<200> requestPayload;            // Declare a JSON document with a capacity of 200 bytes
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
        DynamicJsonDocument jsonResponse(100);     // Declare a JSON document with a capacity of 200 bytes
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
    }   // while loop end
  }
  else
  {
    connectToMQTT();
  } // mqtt connect check end
} // set device time function end

// Function to submit data to Anedya server
// For more info, visit [https://docs.anedya.io/devicehttpapi/submitdata/]
void anedya_submitData(String datapoint, float sensor_data)
{
  boolean check = true;

  String strSubmitTopic = "$anedya/device/" + String(deviceID) + "/submitdata/json";
  const char *submitTopic = strSubmitTopic.c_str();
  while (check)
  {
    if (mqtt_client.connected())
    {

      if (millis() - submitTimer >= 2000)
      {

        submitTimer = millis();
        // Get current time and convert it to milliseconds
        long long current_time = now();                     // Get the current time
        long long current_time_milli = current_time * 1000; // Convert current time to milliseconds

        // Construct the JSON payload with sensor data and timestamp

        String jsonStr = "{\"data\":[{\"variable\": \"" + datapoint + "\",\"value\":" + String(sensor_data) + ",\"timestamp\":" + String(current_time_milli) + "}]}";
        const char *submitJsonPayload = jsonStr.c_str();
        mqtt_client.publish(submitTopic, submitJsonPayload);
      }
      mqtt_client.loop();
      if (submitRes != "")
      {
        // Parse the JSON response
        DynamicJsonDocument jsonResponse(100);    // Declare a JSON document with a capacity of 200 bytes
        deserializeJson(jsonResponse, submitRes); // Deserialize the JSON response from the server into the JSON document

        int errorCode = jsonResponse["errCode"]; // Get the server receive time from the JSON document
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
        submitTimer=5000;
      }
    }
    else
    {
      connectToMQTT();
    } // mqtt connect check end
  }
}
