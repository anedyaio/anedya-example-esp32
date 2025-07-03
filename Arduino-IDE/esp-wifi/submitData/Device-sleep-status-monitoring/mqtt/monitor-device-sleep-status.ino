/*

                            Device Sleep Status Monitoring with Anedya [mqtt]
                Disclaimer: This code is for hobbyists for learning purposes. Not recommended for production use!!

                            # Dashboard Setup
                             - Create account and login to the dashboard
                             - Create new project.
                             - Create variables: deviceSleepStatus.
                             - Create a node (like, Room1 or Device1).
                            Note: Variable Identifier is essential; fill it accurately.


                  Note: The code is tested on the "Esp-32 Wifi, Bluetooth, Dual Core Chip Development Board (ESP-WROOM-32)"

                                                                                           Dated: 09-April-2025

*/

#include <Arduino.h>

#include <WiFi.h>             //library to handle wifi connection
#include <PubSubClient.h>     // library to establish mqtt connection
#include <WiFiClientSecure.h> //library to maintain the secure connection
#include <ArduinoJson.h>      // Include the Arduino library to make json or abstract the value from the json
#include <TimeLib.h>          // Include the Time library to handle time synchronization with ATS (Anedya Time Services)

// ----------------------------- Anedya and Wifi credentials --------------------------------------------
String REGION_CODE = "ap-in-1";                                          // Anedya region code (e.g., "ap-in-1" for Asia-Pacific/India) | For other country code, visity [https://docs.anedya.io/device/#region]
const char *CONNECTION_KEY = "CONNECTION_KEY";  // Fill your connection key, that you can get from your node description
const char *PHYSICAL_DEVICE_ID = "PHYSICAL_DEVICE_ID"; // Fill your device Id , that you can get from your node description
const char *SSID = "YOUR_WIFI_SSID";     
const char *PASSWORD = "YOUR_WIFI_PASSWORD"; 

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

#define TIME_SYNC_BIT 1
#define HEARTBEAT_BIT 2
// #define SUBMIT_FLOAT_DATA_BIT 3
// #define SUBMIT_GEO_COORDINATE_BIT 4
#define SUBMIT_STATUS_BIT 5
// #define SUBMIT_LOG_BIT 6
// #define GET_VALUESTORE_BIT 7
// #define SET_VALUESTORE_BIT 8
// #define OTA_UPDATE_BIT  9

String timeRes, submitRes; // variable to handle response

/*
ESP32 offers a deep sleep mode for effective power
saving as power is an important factor for IoT
applications. In this mode CPUs, most of the RAM,
and all the digital peripherals which are clocked
from APB_CLK are powered off. The only parts of
the chip which can still be powered on are:
RTC controller, RTC peripherals ,and RTC memories
*/
#define uS_TO_S_FACTOR 1000000ULL /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP 60          /* Time ESP32 will go to sleep (in seconds) */

RTC_DATA_ATTR int bootCount = 0;

//------------------------ Initialize Clients ------------------------
WiFiClientSecure esp_client;          // create a WiFiClientSecure object
PubSubClient mqtt_client(esp_client); // creat a PubSubClient object

//-------------------------- Function Declarations -----------------------
void connectToWiFi();
void connectToMQTT();
void mqttCallback(char *topic, byte *payload, unsigned int length);
void syncDeviceTime();                                                             // Function to configure the device time with real-time from ATS (Anedya Time Services)
void anedya_submitStatusData(const char *variable_identifier, const char *status); // Function to submit data to the Anedya
void anedya_sendHeartbeat();                                                       // Function to send heartbeat to the Anedya
void print_wakeup_reason();

void setup()
{
    Serial.begin(115200); // Initialize serial communication with  your device compatible baud rate
    delay(1500);          // Delay for 1.5 seconds

    // Increment boot number and print it every reboot
    ++bootCount;
    Serial.println("Boot number: " + String(bootCount));

    // Print the wakeup reason for ESP32
    print_wakeup_reason();

    /*
    First we configure the wake up source
    We set our ESP32 to wake up every 5 seconds
    */
    esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
    Serial.println("Setup ESP32 to sleep for every " + String(TIME_TO_SLEEP) + " Seconds");

    connectToWiFi();

    // Set Root CA certificate
    esp_client.setCACert(ca_cert);
    mqtt_client.setServer(mqtt_broker, mqtt_port); // Set the MQTT server address and port for the MQTT client to connect to anedya broker
    mqtt_client.setKeepAlive(60);                  // Set the keep alive interval (in seconds) for the MQTT connection to maintain connectivity
    mqtt_client.setCallback(mqttCallback);         // Set the callback function to be invoked when MQTT messages are received
    connectToMQTT();                               // Attempt to establish a connection to the anedya broker

    syncDeviceTime();

    anedya_sendHeartbeat();
    anedya_submitStatusData("deviceSleepStatus", "Awake");

    Serial.println("Going to sleep now...");
    anedya_submitStatusData("deviceSleepStatus", "in-deep-sleep");
    Serial.println("In deep sleep, device will wake up after " + String(TIME_TO_SLEEP) + " seconds");
    mqtt_client.disconnect();
    Serial.flush();
    esp_deep_sleep_start();
    Serial.println("This will never be printed");
}

void loop()
{
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
    JsonDocument jsonResponse;
    deserializeJson(jsonResponse, str_res);
    int reqID = jsonResponse["reqId"];
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
    case SUBMIT_STATUS_BIT:
        Serial.println("Status response message: " + str_res);
        submitRes = str_res;
        break;
    default:
        Serial.println(str_res);
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
            syncTime = false;
        } // response check
    } // while loop end

} // set device time function end

// Function to submit data to Anedya server
// For more info, visit [https://docs.anedya.io/devicehttpapi/submitdata/]
void anedya_submitStatusData(const char *variable_identifier, const char *status)
{
    if (WiFi.status() != WL_CONNECTED)
    {
        connectToWiFi();
    }
    boolean check = true;
    long long waitForResponse_timestamp = millis();

    String strSubmitTopic = "$anedya/device/" + String(PHYSICAL_DEVICE_ID) + "/submitdata/json";
    const char *submitTopic = strSubmitTopic.c_str();

    mqtt_client.connected() ? (void)0 : connectToMQTT(); // check mqtt connection

    // Get current time and convert it to milliseconds
    long long current_time = now();                     // Get the current time
    long long current_time_milli = current_time * 1000; // Convert current time to milliseconds

    // Construct the JSON payload with sensor data and timestamp

    String jsonStr = "{\"reqId\": \"" + String(SUBMIT_STATUS_BIT) + "\",\"data\":[{\"variable\": \"" + String(variable_identifier) + "\",\"value\":\"" + String(status) + "\",\"timestamp\":" + String(current_time_milli) + "}]}";
    const char *submitJsonPayload = jsonStr.c_str();
    Serial.println("Status submission request:" + jsonStr);
    mqtt_client.publish(submitTopic, submitJsonPayload);

    while (check)
    {
        if (millis() - waitForResponse_timestamp >= 5000)
        {
            Serial.println("Failed to push data!!");
            return;
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
                Serial.println("Status submitted successfully!!");
            }
            else if (errorCode == 4040)
            {
                Serial.println("Failed to submit status!!");
                Serial.println("unknown variable Identifier");
                Serial.println(submitRes);
            }
            else
            {
                Serial.println("Failed to submit status!!");
                Serial.println(submitRes);
            }
            submitRes = "";
            check = false;
        }
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

    String strLog = "{\"reqId\": \"" + String(HEARTBEAT_BIT) + "\"}";
    const char *submitLogPayload = strLog.c_str();
    mqtt_client.publish(submitTopic, submitLogPayload);
    mqtt_client.loop();
}

/*
Method to print the reason by which ESP32
has been awaken from sleep
*/
void print_wakeup_reason()
{

    esp_sleep_wakeup_cause_t wakeup_reason;

    wakeup_reason = esp_sleep_get_wakeup_cause();

    switch (wakeup_reason)
    {
    case ESP_SLEEP_WAKEUP_EXT0:
        Serial.println("Wakeup caused by external signal using RTC_IO");
        break;
    case ESP_SLEEP_WAKEUP_EXT1:
        Serial.println("Wakeup caused by external signal using RTC_CNTL");
        break;
    case ESP_SLEEP_WAKEUP_TIMER:
        Serial.println("Wakeup caused by timer");
        break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD:
        Serial.println("Wakeup caused by touchpad");
        break;
    case ESP_SLEEP_WAKEUP_ULP:
        Serial.println("Wakeup caused by ULP program");
        break;
    default:
        Serial.printf("Wakeup was not caused by deep sleep: %d\n", wakeup_reason);
        break;
    }
}