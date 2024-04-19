/*

                                            Smart Home (commands)(mqtt)
                Disclaimer: This code is for hobbyists for learning purposes. Not recommended for production use!!

                            # Dashboard Setup
                             - create account and login to the dashboard
                             - Create project.
                             - Create a node (e.g., for home- Room1 or study room).
                             - Create variables: temperature and humidity.
                            Note: Variable Identifier is essential; fill it accurately.

                            # Hardware Setup
                            - connect Led at GPIO pin 5(Marked D5 on the ESP32)

                  Note: The code is tested on the "Esp-32 Wifi, Bluetooth, Dual Core Chip Development Board (ESP-WROOM-32)"

                                                                                           Dated: 28-March-2024

*/
#include <Arduino.h>

#include <WiFi.h>             //library to handle wifi connection
#include <PubSubClient.h>     // library to establish mqtt connection
#include <WiFiClientSecure.h> //library to maintain the secure connection
#include <ArduinoJson.h>      // Include the Arduino library to make json or abstract the value from the json
#include <TimeLib.h>          // Include the Time library to handle time synchronization with ATS (Anedya Time Services)

String regionCode = "ap-in-1";                   // Anedya region code (e.g., "ap-in-1" for Asia-Pacific/India) | For other country code, visity [https://docs.anedya.io/device/intro/#region]
const char *deviceID = "<PHYSICAL-DEVICE-UUID>"; // Fill your device Id , that you can get from your node description
const char *connectionkey = "<CONNECTION-KEY>";  // Fill your connection key, that you can get from your node description
// WiFi credentials
const char *ssid = "<SSID>";     // Replace with your WiFi name
const char *pass = "<PASSWORD>"; // Replace with your WiFi password

// MQTT connection settings
const char *mqtt_broker = "device.ap-in-1.anedya.io";                      // MQTT broker address
const char *mqtt_username = deviceID;                                      // MQTT username
const char *mqtt_password = connectionkey;                                 // MQTT password
const int mqtt_port = 8883;                                                // MQTT port
String responseTopic = "$anedya/device/" + String(deviceID) + "/response"; // MQTT topic for device responses
String errorTopic = "$anedya/device/" + String(deviceID) + "/errors";      // MQTT topic for device errors
String commandTopic = "$anedya/device/" + String(deviceID) + "/commands";  // MQTT topic for device commands
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

String statusTopic = "$anedya/device/" + String(deviceID) + "/commands/updateStatus/json"; // MQTT topic update status of the command
String timeRes, commandId;                                                                 // varibale to time response and store command Id
String ledStatus = "off";                                                                  // variable to store the status of the led
long long responseTimer = 0;                                                               // timer to control flow
bool processCheck = false;                                                                 // check's, to make sure publish for process topic , once.

const int ledPin = 5; // Marked D5 on the ESP32

// Function Declarations
void connectToMQTT();                                               // function to connect with the anedya broker
void mqttCallback(char *topic, byte *payload, unsigned int length); // funstion to handle call back
void setDevice_time();                                              // Function to configure the device time with real-time from ATS (Anedya Time Services)

// WiFi and MQTT client initialization
WiFiClientSecure esp_client;
PubSubClient mqtt_client(esp_client);

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

  // Set Root CA certificate
  esp_client.setCACert(ca_cert);                 // set the ca certificate
  mqtt_client.setServer(mqtt_broker, mqtt_port); // Set the MQTT server address and port for the MQTT client to connect to anedya broker
  mqtt_client.setKeepAlive(60);                  // Set the keep alive interval (in seconds) for the MQTT connection to maintain connectivity
  mqtt_client.setCallback(mqttCallback);         // Set the callback function to be invoked when MQTT messages are received
  connectToMQTT();                               // Attempt to establish a connection to the anedya broker
  mqtt_client.subscribe(responseTopic.c_str());  // subscribe to get response
  mqtt_client.subscribe(errorTopic.c_str());     // subscibe to get error
  mqtt_client.subscribe(commandTopic.c_str());   // subscribe to get command

  setDevice_time(); // function to sync the the device time

  // Initialize the built-in LED pin as an output
  pinMode(ledPin, OUTPUT);
}

void loop()
{

  if (!mqtt_client.connected())
  {
    connectToMQTT();
  }
  if (millis() - responseTimer > 700 && processCheck && commandId != "")
  {                                                                                                                                                               // condition block to publish the command processing message
    String statusProcessingPayload = "{\"reqId\": \"\",\"commandId\": \"" + commandId + "\",\"status\": \"processing\",\"ackdata\": \"\",\"ackdatatype\": \"\"}"; // payload
    mqtt_client.publish(statusTopic.c_str(), statusProcessingPayload.c_str());   // publish the command processing status message
    processCheck = false;
  }
  else if (millis() - responseTimer >= 1000 && commandId != "")
  { // condition block to publish the command success or failure message
    if (ledStatus == "on" || ledStatus == "ON")
    {
      // Turn the LED on (HIGH)
      digitalWrite(ledPin, HIGH);
      Serial.println("Led ON");
      String statusSuccessPayload = "{\"reqId\": \"\",\"commandId\": \"" + commandId + "\",\"status\": \"success\",\"ackdata\": \"\",\"ackdatatype\": \"\"}";
      mqtt_client.publish(statusTopic.c_str(), statusSuccessPayload.c_str());
    }
    else if (ledStatus == "off" || ledStatus == "OFF")
    {
      // Turn the LED off (LOW)
      digitalWrite(ledPin, LOW);
      Serial.println("Led OFF");
      String statusSuccessPayload = "{\"reqId\": \"\",\"commandId\": \"" + commandId + "\",\"status\": \"success\",\"ackdata\": \"\",\"ackdatatype\": \"\"}";
      mqtt_client.publish(statusTopic.c_str(), statusSuccessPayload.c_str());
    }
    else
    {
      String statusSuccessPayload = "{\"reqId\": \"\",\"commandId\": \"" + commandId + "\",\"status\": \"failure\",\"ackdata\": \"\",\"ackdatatype\": \"\"}";
      mqtt_client.publish(statusTopic.c_str(), statusSuccessPayload.c_str());
      Serial.println("Invalid Command");
    }
    commandId = ""; // checks
  }

  mqtt_client.loop();
}
//<---------------------------------------------------------------------------------------------------------------------------->
void connectToMQTT()
{
  while (!mqtt_client.connected())
  {
    const char *client_id = deviceID;       //set the client id
    Serial.print("Connecting to Anedya Broker....... ");
    if (mqtt_client.connect(client_id, mqtt_username, mqtt_password)) // checks to check mqtt connection
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
  StaticJsonDocument<200> Response; // Declare a JSON document with a capacity of 200 bytes
  // Deserialize JSON and handle errors
  DeserializationError error = deserializeJson(Response, str_res);
  if (error)
  {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.c_str());
    // Handle the error accordingly, such as returning or exiting the function
  }

  if (Response.containsKey("deviceSendTime")) // block's to get the device send time
  {
    timeRes = str_res;
  }
  else if (Response.containsKey("command")) // block's to get the command
  {
    ledStatus = Response["data"].as<String>();
    responseTimer = millis();
    commandId = Response["commandId"].as<String>();
    String statusReceivedPayload = "{\"reqId\": \"\",\"commandId\": \"" + commandId + "\",\"status\": \"received\",\"ackdata\": \"\",\"ackdatatype\": \"\"}";
    mqtt_client.publish(statusTopic.c_str(), statusReceivedPayload.c_str());
    processCheck = true;
  }
  else if (Response["errCode"].as<String>() == "0")  
  {
  }
  else
  {
    Serial.println(str_res);
  }
}
// Function to configure time synchronization with Anedya server
// For more info, visit [https://docs.anedya.io/devicehttpapi/http-time-sync/]
void setDevice_time()
{
  String timeTopic = "$anedya/device/" + String(deviceID) + "/time/json"; // time topic wil provide the current time from the anedya server
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
      if (millis() - timeTimer >= iterate) // time to hold publishing
      {
        Serial.print(".");
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

      if (timeRes != "") // processed it got response
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
    }
    // while loop end
  }
  else
  {
    connectToMQTT();
  } // mqtt connect check end
} // set device time function end
