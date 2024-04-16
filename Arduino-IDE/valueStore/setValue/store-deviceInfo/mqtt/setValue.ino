/*

                                     Store Device Info - Example-ValueStore(mqtt)
                Disclaimer: This code is for hobbyists for learning purposes. Not recommended for production use!!

                            # Dashboard Setup
                             - Create account and login to the dashboard
                             - Create new project.
                             - Create a node (e.g., for home- Room1 or study room).
                            

                  Note: The code is tested on the "Esp-32 Wifi, Bluetooth, Dual Core Chip Development Board (ESP-WROOM-32)"
                  For more info, visit- https://docs.anedya.io/valuestore/intro
                                                                                           Dated: 15-April-2024
*/
#include <Arduino.h>
#include <WiFi.h>            //library to handle wifi connection 
#include <PubSubClient.h>     // library to establish mqtt connection
#include <WiFiClientSecure.h>  //library to maintain the secure connection 
#include <ArduinoJson.h> // Include the Arduino library to make json or abstract the value from the json
#include <TimeLib.h>     // Include the Time library to handle time synchronization with ATS (Anedya Time Services)

String regionCode = "ap-in-1";                   // Anedya region code (e.g., "ap-in-1" for Asia-Pacific/India) | For other country code, visity [https://docs.anedya.io/device/intro/#region]
String deviceID = "<PHYSICAL-DEVICE-UUID>";   // Fill your device Id , that you can get from your node description
String connectionKey = "<CONNECTION-KEY>";  // Fill your connection key, that you can get from your node description
char ssid[] = "<SSID>";  // Your WiFi network SSID
char pass[] = "<PASSWORD>";  // Your WiFi network password

// MQTT connection settings
const char *mqtt_broker = "device.ap-in-1.anedya.io";  // MQTT broker address
const char *mqtt_username = deviceID;  // MQTT username
const char *mqtt_password = connectionKey;  // MQTT password
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

long long setValueTimer,updateInterval,timer;   //timer variable for request handling
String valueRes;  //variable to store the response 


void connectToMQTT();
void mqttCallback(char *topic, byte *payload, unsigned int length);
void anedya_setValue(String KEY, String TYPE, String VALUE);


// WiFi and MQTT client initialization
WiFiClientSecure esp_client;
PubSubClient mqtt_client(esp_client);


void setup()
{
  Serial.begin(115200); // Initialize serial communication with  your device compatible baud rate
  delay(1500);          // Delay for 1.5 seconds

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

  timer=millis();

 
  esp_client.setCACert(ca_cert);   // Set Root CA certificate
mqtt_client.setServer(mqtt_broker, mqtt_port);  // Set the MQTT server address and port for the MQTT client to connect to anedya broker
mqtt_client.setKeepAlive(60);  // Set the keep alive interval (in seconds) for the MQTT connection to maintain connectivity
mqtt_client.setCallback(mqttCallback);  // Set the callback function to be invoked when MQTT messages are received
connectToMQTT(); // Attempt to establish a connection to the anedya broker
mqtt_client.setBufferSize(5000);
long long bufferSize= mqtt_client.getBufferSize();
Serial.println(bufferSize);
mqtt_client.subscribe(responseTopic.c_str());  //subscribe to get response
mqtt_client.subscribe(errorTopic.c_str());    //subscibe to get error

}

void loop()
{
  updateInterval=5000;
  if(millis()-timer>=updateInterval){

  String boardInfo = "Chip ID:" + String((uint32_t)ESP.getEfuseMac(), HEX) +", "+
                   "CPU Frequency:" + String(ESP.getCpuFreqMHz()) +", "+
                   "Flash Size:" + String(ESP.getFlashChipSize() / 1024 / 1024) + " MB" +", "+
                   "Free Heap Size:" + String(ESP.getFreeHeap()) +", "+
                   "Sketch Size:" + String(ESP.getSketchSize() / 1024) + " KB" +", "+
                   "Free Sketch Space:" + String(ESP.getFreeSketchSpace() / 1024) + " KB" +", "+
                   "Flash Speed:" + String(ESP.getFlashChipSpeed() / 1000000) + " MHz";

  anedya_setValue("009", "string", boardInfo); /* anedya_setValue("<-KEY->","<-dataType->","<-VALUE->")
                                                 1 parameter- key, 
                                                 2 parameter- The value can hold any of the following types of data: string, binary, float, boolean
                                                 3 parameter- value.  For detailed info, visit-https://docs.anedya.io/valuestore/intro/        */


   timer=millis();      
  }
  mqtt_client.loop();
}
//<---------------------------------------------------------------------------------------------------------------------------->
void connectToMQTT()
{
  while (!mqtt_client.connected())
  {
    const char *client_id = deviceID;
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
     valueRes = str_res;
     Serial.println(valueRes);
}

void anedya_setValue(String KEY, String TYPE, String VALUE)
{
  boolean check = true;
  String strSetTopic = "$anedya/device/" + String(deviceID) + "/valuestore/setValue/json";
  const char *setTopic = strSetTopic.c_str();
  while (check)
  {
    if (mqtt_client.connected())
    {
      if (millis() - setValueTimer >= 2000)
      {
        setValueTimer = millis();
       String valueJsonStr = "{\"reqId\": \"\",\"key\":\"" + KEY + "\",\"value\": \"" + VALUE + "\",\"type\": \"" + TYPE + "\"}";
        const char *setValuePayload = valueJsonStr.c_str();
      mqtt_client.publish(setTopic, setValuePayload);
      }
      mqtt_client.loop();
      if (valueRes != "")
      {
        // Parse the JSON response
        DynamicJsonDocument jsonResponse(100);    // Declare a JSON document with a capacity of 200 bytes
        deserializeJson(jsonResponse, valueRes);  // Deserialize the JSON response from the server into the JSON document
        int errorCode = jsonResponse["errCode"];  // Get the server receive time from the JSON document
        if (errorCode == 0)
        {
          Serial.println("value set!!");
        }
        else
        {
          Serial.println("Failed to set value!");
          Serial.println(valueRes);
        }
        check = false;
        setValueTimer=5000;
        valueRes="";
      }
    }
    else
    {
      connectToMQTT();
    } // mqtt connect check end
  }
}

