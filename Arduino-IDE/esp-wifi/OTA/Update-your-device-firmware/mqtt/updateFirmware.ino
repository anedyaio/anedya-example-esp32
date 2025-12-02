/*

                                Anedya OTA (Over The Air) Firmware Update
                                    # Dashboard Setup
                                        1. Create account and login
                                        2. Create new project.
                                        3. Create a node (e.g., for home- Room1 or study room).
                                        4. Upload the Asset.
                                        5. Create a Deploymnent.


                  Note: The code is tested on the "Esp-32 Wifi, Bluetooth, Dual Core Chip Development Board (ESP-WROOM-32)"

                                                                                                          Dated: 10-April-2025
*/
#include <Arduino.h>
#include <WiFi.h>             // Include the Wifi to handle the wifi connection
#include <WiFiClientSecure.h> //library to maintain the secure connection
#include <PubSubClient.h>     // library to establish mqtt connection
#include "HttpsOTAUpdate.h"
#include <ArduinoJson.h> // Include the Arduino library to make json or abstract the value from the json
#include <TimeLib.h>     // Include the Time library to handle time synchronization with ATS (Anedya Time Services)

/*-----------------------------Variable section-------------------------------------------*/
// ----------------------------- Anedya and Wifi credentials --------------------------------------------
String REGION_CODE = "ap-in-1";                        // Anedya region code (e.g., "ap-in-1" for Asia-Pacific/India) | For other country code, visit [https://docs.anedya.io/device/#region]
const char *CONNECTION_KEY = "CONNECTION_KEY";         // Fill your connection key, that you can get from your node description
const char *PHYSICAL_DEVICE_ID = "PHYSICAL_DEVICE_ID"; // Fill your device Id , that you can get from your node description
const char *SSID = "YOUR_WIFI_SSID";
const char *PASSWORD = "YOUR_WIFI_PASSWORD";

// Anedya Root CA 3 (ECC - 256)(Pem format)| [https://docs.anedya.io/device/mqtt-endpoints/#tls]
static const char *ca_cert = R"EOF(
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
#define MQTT_BUFFER_SIZE 1024
String str_mqtt_broker = "mqtt." + REGION_CODE + ".anedya.io";
const char *mqtt_broker = str_mqtt_broker.c_str();                                   // MQTT broker address
const char *mqtt_username = PHYSICAL_DEVICE_ID;                                      // MQTT username
const char *mqtt_password = CONNECTION_KEY;                                          // MQTT password
const int mqtt_port = 8883;                                                          // MQTT port
String responseTopic = "$anedya/device/" + String(PHYSICAL_DEVICE_ID) + "/response"; // MQTT topic for device responses
String errorTopic = "$anedya/device/" + String(PHYSICAL_DEVICE_ID) + "/errors";      // MQTT topic for device errors

// ----------------------------- Helper Variable ----------------------------------------
long last_check_for_ota_update = 0;
long check_for_ota_update_interval = 10000;
String timeRes; // variable to handle response

#define TIME_SYNC_BIT 1
#define HEARTBEAT_BIT 2
#define OTA_UPDATE_BIT 9
#define OTA_UPDATE_STATUS_BIT 10

bool deploymentAvailable = false;
bool statusPublished = false;
String assetURL = "";
String deploymentID = "";

/*--------------------Object initializing--------------------------------*/
WiFiClientSecure esp_client;          // create a WiFiClientSecure object
PubSubClient mqtt_client(esp_client); // creat a PubSubClient object
static HttpsOTAStatus_t otastatus;

/*------------------------Function declarations--------------------------*/
void connectToWiFi();
void connectToMQTT();
void mqttCallback(char *topic, byte *payload, unsigned int length);
void syncDeviceTime();       // Function to configure the device time with real-time from ATS (Anedya Time Services)
void anedya_sendHeartbeat(); // Function to send heartbeat to the Anedya
bool anedya_check_ota_update();
void anedya_update_ota_status(String deploymentID, String deploymentStatus);

// ---------------------------- OTA Event Handler ----------------------------
void HttpEvent(HttpEvent_t *event)
{
    switch (event->event_id)
    {
    case HTTP_EVENT_ERROR:
        Serial.println("Http Event Error");
        break;
    case HTTP_EVENT_ON_CONNECTED:
        Serial.println("Http Event On Connected");
        break;
    case HTTP_EVENT_HEADER_SENT:
        Serial.println("Http Event Header Sent");
        break;
    case HTTP_EVENT_ON_HEADER:
        Serial.printf("Http Event On Header, key=%s, value=%s\n", event->header_key, event->header_value);
        break;
    case HTTP_EVENT_ON_DATA:
        break;
    case HTTP_EVENT_ON_FINISH:
        Serial.println("Http Event On Finish");
        break;
    case HTTP_EVENT_DISCONNECTED:
        Serial.println("Http Event Disconnected");
        break;
        // case HTTP_EVENT_REDIRECT:
        //     Serial.println("Http Event Redirect");
        //     break;
    }
}

/*-----------------------------SETUP SECTION----------------------------------*/
void setup()
{
    Serial.begin(115200); // Initialize serial communication with  your device compatible baud rate
    delay(1500);          // Delay for 1.5 seconds

    connectToWiFi();

    esp_client.setCACert(ca_cert);                 // Set Root CA certificate
    mqtt_client.setServer(mqtt_broker, mqtt_port); // Set the MQTT server address and port for the MQTT client to connect to anedya broker
    mqtt_client.setKeepAlive(60);                  // Set the keep alive interval (in seconds)
    mqtt_client.setCallback(mqttCallback);         // Set the callback function to be invoked when MQTT messages are received
    mqtt_client.setBufferSize(MQTT_BUFFER_SIZE);   // Set the MQTT buffer size
    connectToMQTT();                               // Attempt to establish a connection to the anedya broker

    HttpsOTA.onHttpEvent(HttpEvent);
    last_check_for_ota_update = millis();

    syncDeviceTime();
}
/*---------------------LOOP SECTION-------------------------------*/
void loop()
{
    if (millis() - last_check_for_ota_update >= check_for_ota_update_interval)
    {
        anedya_sendHeartbeat(); // sending heartbeat
        bool success = anedya_check_ota_update();
        if (success)
        {
            if (deploymentAvailable)
            {
                anedya_update_ota_status(deploymentID, "start");

                Serial.println("Starting firmware update");
                HttpsOTA.begin(assetURL.c_str(), ca_cert, false);
                Serial.print(" OTA in progress ..");
                while (1)
                {
                    Serial.print(".");
                    otastatus = HttpsOTA.status();
                    if (otastatus == HTTPS_OTA_SUCCESS)
                    {
                        Serial.println("Firmware written successfully");
                        anedya_update_ota_status(deploymentID, "success");
                        ESP.restart();
                    }
                    else if (otastatus == HTTPS_OTA_FAIL)
                    {
                        Serial.println("Firmware Upgrade Fail");
                        anedya_update_ota_status(deploymentID, "failure");
                        break;
                    }
                    delay(1000);
                }
                deploymentAvailable = false;
            }
        }
        else
        {
            Serial.println("Failed to Publish!");
        }
        last_check_for_ota_update = millis();
    }
    mqtt_client.loop();
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
    if (WiFi.status() != WL_CONNECTED)
    {
        connectToWiFi();
    }
    while (!mqtt_client.connected())
    {
        const char *client_id = PHYSICAL_DEVICE_ID;
        Serial.print("Connecting to Anedya Broker....... ");
        if (mqtt_client.connect(client_id, mqtt_username, mqtt_password))
        {
            Serial.println("Connected to Anedya broker");
            mqtt_client.subscribe(responseTopic.c_str(), 1); // subscribe to get response
            mqtt_client.subscribe(errorTopic.c_str(), 1);    // subscibe to get error
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
    Serial.print("Message received on topic: ");
    Serial.println(topic);
    char res[MQTT_BUFFER_SIZE] = "";

    for (unsigned int i = 0; i < length; i++)
    {
        res[i] = payload[i];
    }
    String str_res(res);
    Serial.println("Mqtt Message: " + str_res);
    Serial.println();

    // Parse the JSON response
    JsonDocument jsonResponse;
    deserializeJson(jsonResponse, str_res); // Deserialize the JSON response from into the JSON document
    int reqID = int(jsonResponse["reqId"]); // Get the server receive time from the JSON document

    switch (reqID)
    {
    case TIME_SYNC_BIT:
        timeRes = str_res;
        break;
    case HEARTBEAT_BIT:
        if (jsonResponse["success"])
        {
            Serial.println("Heatbeat sent successfully!");
        }
        else
        {
            Serial.println("Heatbeat sent failed! :" + str_res);
        }
        break;
    case OTA_UPDATE_BIT:
        deploymentAvailable = bool(jsonResponse["deploymentAvailable"]);
        if (deploymentAvailable)
        {
            Serial.println("Update Available!");
            Serial.println("Asset identifier: " + jsonResponse["data"]["assetIdentifier"].as<String>());
            Serial.println("Asset version: " + jsonResponse["data"]["assetVersion"].as<String>());
            deploymentID = jsonResponse["data"]["deploymentId"].as<String>();
            Serial.println("Deployment ID: " + String(deploymentID));
            assetURL = jsonResponse["data"]["asseturl"].as<String>();
            Serial.println("Asset URL: " + String(assetURL));
        }
        else
        {
            Serial.println("No update available.");
        }
        break;
    case OTA_UPDATE_STATUS_BIT:
        statusPublished = true;
        break;
    default:
        Serial.println("Unknown response:" + str_res);
        break;
    }
}
// Function to configure time synchronization with Anedya server
// For more info, visit [https://docs.anedya.io/device/api/http-time-sync/]
void syncDeviceTime()
{
    if (WiFi.status() != WL_CONNECTED)
    {
        connectToWiFi();
    }
    String timeTopic = "$anedya/device/" + String(PHYSICAL_DEVICE_ID) + "/time/json";
    const char *mqtt_topic = timeTopic.c_str();
    // Attempt to synchronize time with Anedya server

    mqtt_client.connected() ? (void)0 : connectToMQTT(); // check mqtt connection

    Serial.print("Time synchronizing......");

    boolean syncTime = true; // iteration to re-sync to ATS (Anedya Time Services), in case of failed attempt
    // Get the device send time

    long long deviceSendTime;
    long long timeTimer = millis();
    while (syncTime)
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
            requestPayload["reqId"] = String(TIME_SYNC_BIT);   // Add a key-value pair to the JSON document
            requestPayload["deviceSendTime"] = deviceSendTime; // Add a key-value pair to the JSON document
            String jsonPayload;                                // Declare a string to store the serialized JSON payload
            serializeJson(requestPayload, jsonPayload);        // Serialize the JSON document into a string

            // Convert String object to pointer to a string literal
            const char *jsonPayloadLiteral = jsonPayload.c_str();
            mqtt_client.publish(mqtt_topic, jsonPayloadLiteral);
        }

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
            syncTime = false;
        }
    }
}

// ---------------------- Function to check for OTA update ----------------------
bool anedya_check_ota_update()
{
    if (WiFi.status() != WL_CONNECTED)
    {
        connectToWiFi();
    }
    mqtt_client.connected() ? (void)0 : connectToMQTT();

    Serial.println("Checking for OTA updates...");
    String getNextDeploymentTopic_str = "$anedya/device/" + String(PHYSICAL_DEVICE_ID) + "/ota/next/json";
    const char *getNextDeploymentTopic = getNextDeploymentTopic_str.c_str();

    String getNextDeploymentPayload_str = "{\"reqId\": \"" + String(OTA_UPDATE_BIT) + "\"}";
    const char *getNextDeploymentPayload = getNextDeploymentPayload_str.c_str();

    // Serial.println("Next Deployment Topic: " + String(getNextDeploymentTopic));
    // Serial.println("Next Deployment Payload: " + String(getNextDeploymentPayload));

    bool success = mqtt_client.publish(getNextDeploymentTopic, getNextDeploymentPayload);
    mqtt_client.loop();
    return success;
}
// ---------------------- Function to update OTA Status ----------------------
void anedya_update_ota_status(String deploymentID, String deploymentStatus)
{
    if (WiFi.status() != WL_CONNECTED)
    {
        connectToWiFi();
    }
    mqtt_client.connected() ? (void)0 : connectToMQTT();

    Serial.println("Checking for OTA updates...");
    String getNextDeploymentTopic_str = "$anedya/device/" + String(PHYSICAL_DEVICE_ID) + "/ota/updateStatus/json";
    const char *getNextDeploymentTopic = getNextDeploymentTopic_str.c_str();

    String getNextDeploymentPayload_str = "{\"reqId\": \"" + String(OTA_UPDATE_STATUS_BIT) + "\", \"deploymentId\": \"" + deploymentID + "\", \"status\": \"" + deploymentStatus + "\", \"log\": \"OK\" }";
    const char *getNextDeploymentPayload = getNextDeploymentPayload_str.c_str();

    mqtt_client.publish(getNextDeploymentTopic, getNextDeploymentPayload);
    mqtt_client.loop();
}

//---------------------------------- Function to send heartbeat -----------------------------------
void anedya_sendHeartbeat()
{
    if (WiFi.status() != WL_CONNECTED)
    {
        connectToWiFi();
    }
    mqtt_client.connected() ? (void)0 : connectToMQTT();

    String strSubmitTopic = "$anedya/device/" + String(PHYSICAL_DEVICE_ID) + "/heartbeat/json";
    const char *submitTopic = strSubmitTopic.c_str();

    String strLog = "{\"reqId\": \"" + String(HEARTBEAT_BIT) + "\"}";

    const char *submitLogPayload = strLog.c_str();
    statusPublished = false;
    mqtt_client.publish(submitTopic, submitLogPayload);
    while (!statusPublished)
    {
        mqtt_client.loop();
        delay(1000);
    }
}
