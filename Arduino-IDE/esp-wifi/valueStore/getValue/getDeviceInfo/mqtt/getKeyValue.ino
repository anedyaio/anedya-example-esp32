/*
                                  Get Stored Device Info - Example-ValueStore(mqtt)
                Disclaimer: This code is for hobbyists for learning purposes. Not recommended for production use!!

                            # Dashboard Setup
                             - Create account and login to the dashboard
                             - Create new project.
                             - Create a node (e.g., for home- Room1 or study room).

                  Note: The code is tested on the "Esp-32 Wifi, Bluetooth, Dual Core Chip Development Board (ESP-WROOM-32)"
                  For more info, visit- https://docs.anedya.io/valuestore/intro
                                                                                           Dated: 28-August-2024
*/
#include <Arduino.h>
#include <WiFi.h>             //library to handle wifi connection
#include <PubSubClient.h>     // library to establish mqtt connection
#include <WiFiClientSecure.h> //library to maintain the secure connection
#include <ArduinoJson.h>      // Include the Arduino library to make json or abstract the value from the json
#include <TimeLib.h>          // Include the Time library to handle time synchronization with ATS (Anedya Time Services)

// ----------------------------- Anedya and Wifi credentials --------------------------------------------
String REGION_CODE = "ap-in-1";                   // Anedya region code (e.g., "ap-in-1" for Asia-Pacific/India) | For other country code, visity [https://docs.anedya.io/device/#region]
const char *CONNECTION_KEY = "";  // Fill your connection key, that you can get from your node description
const char *PHYSICAL_DEVICE_ID = ""; // Fill your device Id , that you can get from your node description
const char *SSID = "";     
const char *PASSWORD = ""; 


// MQTT connection settings
String str_mqtt_broker = "mqtt." + REGION_CODE + ".anedya.io";
const char *mqtt_broker = str_mqtt_broker.c_str();                          // MQTT broker address
const char *mqtt_username = PHYSICAL_DEVICE_ID;                                      // MQTT username
const char *mqtt_password = CONNECTION_KEY;                                 // MQTT password
const int mqtt_port = 8883;                                                 // MQTT port
String responseTopic = "$anedya/device/" + String(PHYSICAL_DEVICE_ID) + "/response"; // MQTT topic for device responses
String errorTopic = "$anedya/device/" + String(PHYSICAL_DEVICE_ID) + "/errors";      // MQTT topic for device errors

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

long long updateInterval, timer, lastSubmittedHeartbeat_timer; // timer variable for request handling
String valueRes;                 // variable to store the response

void connectToMQTT();
void mqttCallback(char *topic, byte *payload, unsigned int length);
void anedya_getNodeValue(String KEY);
void anedya_getGlobalValue(String ID, String KEY);
void anedya_sendHeartbeat();

// WiFi and MQTT client initialization
WiFiClientSecure esp_client;
PubSubClient mqtt_client(esp_client);

void setup()
{
    Serial.begin(115200); // Initialize serial communication with  your device compatible baud rate
    delay(1500);          // Delay for 1.5 seconds

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

    timer = millis();
    lastSubmittedHeartbeat_timer = millis();

    esp_client.setCACert(ca_cert);                 // Set Root CA certificate
    mqtt_client.setServer(mqtt_broker, mqtt_port); // Set the MQTT server address and port for the MQTT client to connect to anedya broker
    mqtt_client.setKeepAlive(60);                  // Set the keep alive interval (in seconds) for the MQTT connection to maintain connectivity
    mqtt_client.setCallback(mqttCallback);         // Set the callback function to be invoked when MQTT messages are received
    mqtt_client.setBufferSize(2048);
    connectToMQTT(); // Attempt to establish a connection to the anedya broker
}

void loop()
{
    updateInterval = 15000;
    if (millis() - timer >= updateInterval)
    {
        anedya_getNodeValue("DeviceInfo");
        anedya_getGlobalValue("global-key-value", "global-key");
        timer = millis();
    }

    if (millis() - lastSubmittedHeartbeat_timer >= 5000){
        anedya_sendHeartbeat();
        lastSubmittedHeartbeat_timer = millis();
    }
    mqtt_client.loop();
}
//<---------------------------------------------------------------------------------------------------------------------------->
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
    char res[2048] = "";
    for (unsigned int i = 0; i < length; i++)
    {
        if (i < sizeof(res) - 1)
        { // Ensure we don't overflow the buffer
            res[i] = payload[i];
        }
    }
    res[length] = '\0'; // Null-terminate the string
    String str_res(res);
    valueRes = str_res;
    Serial.println(valueRes);
}

void anedya_getNodeValue(String KEY)
{
    String strSetTopic = "$anedya/device/" + String(PHYSICAL_DEVICE_ID) + "/valuestore/getValue/json";
    const char *setTopic = strSetTopic.c_str();
    if (mqtt_client.connected())
    {
        String valueJsonStr = "{"
                              "\"reqId\": \"\","
                              "\"namespace\": {"
                              "\"scope\": \"self\","
                              "\"id\": \"\""
                              "},"
                              "\"key\": \"" +
                              KEY + "\""
                                    "}";
        const char *setValuePayload = valueJsonStr.c_str();
        mqtt_client.publish(setTopic, setValuePayload);

        mqtt_client.loop();
    }
    else
    {
        connectToMQTT();
    } // mqtt connect check end
}
void anedya_getGlobalValue(String NAMESPACE, String KEY)
{
    String strSetTopic = "$anedya/device/" + String(PHYSICAL_DEVICE_ID) + "/valuestore/getValue/json";
    const char *setTopic = strSetTopic.c_str();
    if (mqtt_client.connected())
    {
        String valueJsonStr = "{"
                              "\"reqId\": \"\","
                              "\"namespace\": {"
                              "\"scope\": \"global\","
                              "\"id\": \"" +
                              NAMESPACE + "\""
                                   "},"
                                   "\"key\": \"" +
                              KEY + "\""
                                    "}";
        const char *setValuePayload = valueJsonStr.c_str();
        mqtt_client.publish(setTopic, setValuePayload);

        mqtt_client.loop();
    }
    else
    {
        connectToMQTT();
    } // mqtt connect check end
}

void anedya_sendHeartbeat()
{
  mqtt_client.connected() ? (void)0 : connectToMQTT();

  String strSubmitTopic = "$anedya/device/" + String(PHYSICAL_DEVICE_ID) + "/heartbeat/json";
  const char *submitTopic = strSubmitTopic.c_str();

  String strLog = "{}";
  const char *submitLogPayload = strLog.c_str();
  mqtt_client.publish(submitTopic, submitLogPayload);
  mqtt_client.loop();
}