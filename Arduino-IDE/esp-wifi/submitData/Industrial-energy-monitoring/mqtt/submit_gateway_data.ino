/*

                          Industrial Energy Monitoring Using ESP32

                            # Dashboard Setup
                             - Create account and login to the dashboard
                             - Create new project.
                             - Create variables: temperature and humidity.
                             - Create a parent node (e.g., Gateway Node) and pre-authorize it.
                             - Create a child node (e.g., Child Node 1).
                             - Goto parent node->child nodes->add a Child Node with(Child Node ID and Alias)

                            Note: Variable Identifier is essential; fill it accurately.

                            # Hardware Setup
                             - Properly identify your sensor's pins.
                             - Connect sensor VCC pin to 3V3.
                             - Connect sensor GND pin to GND.
                             - Connect sensor signal pin to D5.

                  Note: The code is tested on the "Esp-32 Wifi, Bluetooth, Dual Core Chip Development Board (ESP-WROOM-32)"

                                                                                           Dated: 02-Dec-2025

*/

#include <Arduino.h>

// Emulate Hardware Sensor?
bool virtual_sensor = true; //(This is true if you don't have a physical sensor)

#include <WiFi.h>             //library to handle wifi connection
#include "time.h"             //library to handle time synchronization
#include <PubSubClient.h>     // library to establish mqtt connection
#include <WiFiClientSecure.h> //library to maintain the secure connection
#include <ArduinoJson.h>      // Include the Arduino library to make json or abstract the value from the json

// ----------------------------- Anedya and Wifi credentials --------------------------------------------
const char *CONNECTION_KEY = "REPLACE_WITH_YOUR_CONNECTION_KEY";         // Fill your connection key, that you can get from your node description
const char *PHYSICAL_DEVICE_ID = "REPLACE_WITH_YOUR_PHYSICAL_DEVICE_ID"; // Fill your device Id , that you can get from your node description
const char *SSID = "REPLACE_WITH_YOUR_SSID";
const char *PASSWORD = "REPLACE_WITH_YOUR_PASSWORD";
String REGION_CODE = "ap-in-1"; // Anedya region code (e.g., "ap-in-1" for Asia-Pacific/India)
                                // For other country code, visit [https://docs.anedya.io/device/#region]

// Anedya Root CA 3 (ECC - 256)(Pem format)| [https://docs.anedya.io/device/mqtt-endpoints/#tls]
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

//--------------------------- MQTT connection settings --------------------------------
String str_mqtt_broker = "mqtt." + REGION_CODE + ".anedya.io";
const char *mqtt_broker = str_mqtt_broker.c_str();                                   // MQTT broker address
const char *mqtt_username = PHYSICAL_DEVICE_ID;                                      // MQTT username
const char *mqtt_password = CONNECTION_KEY;                                          // MQTT password
const int mqtt_port = 8883;                                                          // MQTT port
String responseTopic = "$anedya/device/" + String(PHYSICAL_DEVICE_ID) + "/response"; // MQTT topic for device responses
String errorTopic = "$anedya/device/" + String(PHYSICAL_DEVICE_ID) + "/errors";      // MQTT topic for device errors

// --------------------------- Helper variables --------------------------------
#define HEARTBEAT 1
#define SUBMIT_DATA 2
#define SUBMIT_GATEWAY_DATA 3

String heartbeat_response, submit_data_response, submit_gateway_data_response; // variable to handle response
float temperature;
float humidity;

typedef struct
{
    float temperature;
    float humidity;
} sensors_data_t;

#define NUMBER_OF_SLAVES 2
sensors_data_t child[NUMBER_OF_SLAVES];

//------------------------ Initialize Clients ------------------------
WiFiClientSecure esp_client;          // create a WiFiClientSecure object
PubSubClient mqtt_client(esp_client); // creat a PubSubClient object

//-------------------------- Function Declarations -----------------------
void connectToWiFi();
void connectToMQTT();
void mqttCallback(char *topic, byte *payload, unsigned int length);
void anedya_sendHeartbeat();                                                                 // Function to send heartbeat to the Anedya
void anedya_submit_gatewayData(String variable_identifier, float sensor_data, String child); // Function to submit data to the Anedya

// ----------------------------------------------------
void connectToWiFi()
{
    // Connect to WiFi network
    WiFi.begin(SSID, PASSWORD);
    Serial.println();
    Serial.print("Connecting to WiFi with ssid: " + String(SSID) + ", password: " + String(PASSWORD) + " ...");
    short retry_count = 0;
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
        retry_count++;
        if (retry_count > 15)
            esp_restart();
    }
    Serial.println();
    Serial.print("Connected, IP address: ");
    Serial.println(WiFi.localIP());
    Serial.println(WiFi.gatewayIP());
    Serial.println(WiFi.dnsIP());
}

void connectToMQTT()
{
    if (WiFi.status() != WL_CONNECTED)
        connectToWiFi();
    while (!mqtt_client.connected())
    {
        const char *client_id = PHYSICAL_DEVICE_ID;
        Serial.println("Connecting to Anedya Broker with client id: " + String(client_id) + "username: " + String(mqtt_username) + "password: " + String(mqtt_password) + " ....... ");
        if (mqtt_client.connect(client_id, mqtt_username, mqtt_password))
        {
            Serial.println("Connected to Anedya broker");
            mqtt_client.subscribe(responseTopic.c_str()); // subscribe on the response topic to get response
            mqtt_client.subscribe(errorTopic.c_str());    // subscibe on the error topic to get error
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
    char res[150] = {0};
    for (unsigned int i = 0; i < length; i++)
        res[i] = payload[i];

    String res_str(res);
    // Serial.println("Mqqt Callback: " + res_str);

    JsonDocument jsonResponse;              // Declare a JSON document with a capacity of 200 bytes
    deserializeJson(jsonResponse, res_str); // Deserialize the JSON response from the server into the JSON document
    int reqID = int(jsonResponse["reqId"]); // Get the reqId from the JSON document
    switch (reqID)
    {
    case HEARTBEAT:
        // Serial.println("Heartbeat Response Received");
        heartbeat_response = res_str;
        break;
    case SUBMIT_DATA:
        submit_data_response = res_str;
        // Serial.println("Submit Data Response Received");
        break;
    case SUBMIT_GATEWAY_DATA:
        submit_gateway_data_response = res_str;
        break;
    default:
        Serial.print("Unknown Response Received: ");
        Serial.println(res_str);
    }
}

void setup()
{
    Serial.begin(115200);                                                      // Initialize serial communication with  your device compatible baud rate
    delay(1500);                                                               // Delay for 1.5 seconds
    connectToWiFi();                                                           // Connect to WiFi
    configTime(19800, 0, "time.anedya.io", "time.google.com", "pool.ntp.org"); // Configure time synchronization with NTP servers
    Serial.print("Waiting for time sync...");
    while ((long long)time(nullptr) < 1700000000)
    {
        Serial.print(".");
        delay(1000);
    }
    Serial.println("Done!");

    esp_client.setCACert(ca_cert);                 // Set Root CA certificate for secure connection
    mqtt_client.setServer(mqtt_broker, mqtt_port); // Set the MQTT broker address and port for the MQTT client to connect to anedya broker
    mqtt_client.setKeepAlive(60);                  // Set the keep alive interval (in seconds) for the MQTT connection to maintain connectivity
    mqtt_client.setCallback(mqttCallback);         // Set the callback function to be invoked when MQTT messages are received
    connectToMQTT();                               // Attempt to establish a connection to the anedya broker
}

void loop()
{
    anedya_sendHeartbeat(); // send heartbeat to the Anedya let it know that the device is online

    // Generate random temperature and humidity values
    for (int i = 0; i < NUMBER_OF_SLAVES; i++)
    {
        child[i].temperature = random(20, 30);
        child[i].humidity = random(60, 80);
    }

    for (int i = 0; i < NUMBER_OF_SLAVES; i++)
    {
        String childAlias = "child" + String(i + 1);

        // Submit temperature data to Anedya cloud
        Serial.print("Child " + String(i + 1) + " :: Temperature : ");
        Serial.println(child[i].temperature);
        anedya_submit_gatewayData("temperature", child[i].temperature, childAlias); // submit data to the Anedya

        // Submit humidity data to Anedya cloud
        Serial.print("Child " + String(i + 1) + " :: Humidity : ");
        Serial.println(child[i].humidity);
        anedya_submit_gatewayData("humidity", child[i].humidity, childAlias); // submit data to the Anedya
        delay(1000);
    }
    Serial.println("---------------------------------------------------");
    delay(5000);
}

//<--------------------------------------------------------------------->
// Function to send heartbeat to Anedya cloud
// For more info, visit [https://docs.anedya.io/features/heartbeat/]
void anedya_sendHeartbeat()
{
    if (WiFi.status() != WL_CONNECTED)
        connectToWiFi();
    mqtt_client.connected() ? (void)0 : connectToMQTT();
    String topic = "$anedya/device/" + String(PHYSICAL_DEVICE_ID) + "/heartbeat/json";

    // Construct the JSON payload for heartbeat
    JsonDocument jsonDoc;
    jsonDoc["reqId"] = String(HEARTBEAT);
    String payload;
    serializeJson(jsonDoc, payload);

    // Publish the heartbeat message to the MQTT topic
    Serial.print("Sending Heartbeat: ");
    Serial.println(payload);
    mqtt_client.publish(topic.c_str(), payload.c_str());
    delay(50);
    long long waitForResponse_timestamp = millis();
    while (millis() - waitForResponse_timestamp < 5000)
    {
        if (heartbeat_response != "")
        {
            Serial.println("Heartbeat Response: " + heartbeat_response);
            heartbeat_response = "";
            break;
        }
        mqtt_client.loop();
        delay(100);
    }
}

// Function to submit data to Anedya Cloud
// For more info, visit [https://docs.anedya.io/device/api/gatewaydata/]
void anedya_submit_gatewayData(String variable_identifier, float sensor_data, String child)
{
    if (WiFi.status() != WL_CONNECTED)
        connectToWiFi();
    mqtt_client.connected() ? (void)0 : connectToMQTT(); // check mqtt connection
    String topic = "$anedya/device/" + String(PHYSICAL_DEVICE_ID) + "/gateway/submitdata/json";
    const char *topic_str = topic.c_str();

    // Construct the JSON payload with sensor data and timestamp
    JsonDocument jsonDoc;
    jsonDoc["reqId"] = String(SUBMIT_GATEWAY_DATA);
    jsonDoc["childAlias"] = child;
    jsonDoc["data"][0]["variable"] = variable_identifier;
    jsonDoc["data"][0]["value"] = sensor_data;
    jsonDoc["data"][0]["timestamp"] = (long long)time(nullptr) * 1000;

    String jsonPayload;
    serializeJson(jsonDoc, jsonPayload);
    Serial.print("Submit Data Request: ");
    Serial.println(jsonPayload);
    mqtt_client.publish(topic_str, jsonPayload.c_str());

    boolean check = true;
    long long waitForResponse_timestamp = millis();
    while (check)
    {
        if (millis() - waitForResponse_timestamp >= 5000)
        {
            Serial.println("Failed to push data!! | Response Timeout");
            return;
        }
        mqtt_client.loop();
        if (submit_gateway_data_response != "")
        {
            Serial.print("Submit Data Response: ");
            Serial.println(submit_gateway_data_response);
            // Parse the JSON response
            JsonDocument jsonResponse;
            deserializeJson(jsonResponse, submit_gateway_data_response);
            int errorCode = jsonResponse["errorcode"];
            if (errorCode == 0)
                Serial.println("Data pushed to Anedya!!");
            else if (errorCode == 4040)
            {
                Serial.println("Unknown Variable Identifier, check the variable identifier in the dashboard.");
                Serial.println("Warning: - Don't use the variable name, use the variable identifier");
                Serial.println("         - Variable identifier is case sensitive");
            }
            else
                Serial.println("Failed to push data!!");
            submit_gateway_data_response = "";
            check = false;
        }
    }
}
