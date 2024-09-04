/*

                                     Get Stored Device Info - Example-ValueStore(http)
                Disclaimer: This code is for hobbyists for learning purposes. Not recommended for production use!!

                            # Dashboard Setup
                             - Create account and login to the dashboard
                             - Create new project.
                             - Create a node (e.g., for home- Room1 or study room).


                  Note: The code is tested on the "Esp-32 Wifi, Bluetooth, Dual Core Chip Development Board (ESP-WROOM-32)"
                  For more info, visit- https://docs.anedya.io/valuestore/intro
                                                                                           Dated: 8-April-2024

*/
#include <Arduino.h>
#include <WiFi.h>        // Include the Wifi to handle the wifi connection
#include <HTTPClient.h>  // Include the HTTPClient library to handle HTTP requests
#include <ArduinoJson.h> //Include the ArduinoJson library to handle json
#include <TimeLib.h>

/*-----------------------------------------------------variable section------------------------------------------------------------*/
//------------------Esseential and sensitive variable---------------------------------
String REGION_CODE = "ap-in-1";                // Anedya region code (e.g., "ap-in-1" for Asia-Pacific/India) | For other country code, visity [https://docs.anedya.io/device/#region]
const char *CONNECTION_KEY = "CONNECTION_KEY"; // Fill your connection key, that you can get from your node description
const char *DEVICE_ID = "PHYSICAL_DEVICE_ID";  // Fill your unique device Id
// WiFi credentials
const char *ssid = "SSID";
const char *password = "PASSWORD";

//-----------------------------------helper variable--------------------------
long long updateInterval, timer; // varibles to insert interval
void anedya_getNodeValue(String KEY);
void anedya_getGlobalValue(String ID, String KEY);

/*-----------------------------------------------------setup section------------------------------------------------------------*/
void setup()
{
  Serial.begin(115200); // Initialize serial communication with  your device compatible baud rate
  delay(1500);          // Delay for 1.5 seconds

  WiFi.begin(ssid, password);
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
}

void loop()
{

  updateInterval = 15000;
  if (millis() - timer >= updateInterval)
  {
    anedya_getNodeValue("Device Info");
    anedya_getGlobalValue("global-key-value", "global-key");
    timer = millis();
  }
}

void anedya_getNodeValue(String KEY)
{
  if (WiFi.status() == WL_CONNECTED)
  {
    HTTPClient http;                                                                            // Create an instance of HTTPClient
    String setValue_url = "https://device." + REGION_CODE + ".anedya.io/v1/valuestore/getValue"; // Construct the URL for submitting data

    // Prepare data payload in JSON format
    http.begin(setValue_url);                           // Begin an HTTP request to the specified URL
    http.addHeader("Content-Type", "application/json"); // Add a header specifying the content type as JSON
    http.addHeader("Accept", "application/json");       // Add a header specifying the accepted content type as JSON
    http.addHeader("Auth-mode", "key");                 // Add a header specifying the authentication mode as "key"
    http.addHeader("Authorization", CONNECTION_KEY);     // Add a header containing the authorization key

    // Construct the JSON payload
    String valueJsonStr = "{"
                          "\"reqId\": \"\","
                          "\"namespace\": {"
                          "\"scope\": \"self\","
                          "\"id\": \"\""
                          "},"
                          "\"key\": \"" +
                          KEY + "\""
                                "}";

    // Send the POST request with the JSON payload to Anedya server
    int httpResponseCode = http.POST(valueJsonStr);

    // Check if the request was successful
    if (httpResponseCode > 0)
    {
      String response = http.getString(); // Get the response from the server
      // Parse the JSON response
      JsonDocument valueResponse;
      deserializeJson(valueResponse, response); // Extract the JSON response
                                                // Extract the server time from the response
      int errorcode = valueResponse["errorcode"];
      if (errorcode == 0)
      {
        Serial.println("Fetched key-value successfully!!");
        Serial.println(response); // Print the response
      }
      else
      {
        Serial.println("Failed to fetch key-value!!");
        Serial.println(response); // Print the response
      }
    }
    else
    {
      Serial.print("Error on sending POST: "); // Print error message indicating failure to send POST request
      Serial.println(httpResponseCode);        // Print the HTTP response code
    }
    http.end(); // End the HTTP client session
  }
  else
  {
    Serial.println("Error in WiFi connection"); // Print error message indicating WiFi connection failure
  }
}

void anedya_getGlobalValue(String NAMESPACE, String KEY)
{
  if (WiFi.status() == WL_CONNECTED)
  {
    HTTPClient http;                                                                            // Create an instance of HTTPClient
    String setValue_url = "https://device." + REGION_CODE + ".anedya.io/v1/valuestore/getValue"; // Construct the URL for submitting data

    // Prepare data payload in JSON format
    http.begin(setValue_url);                           // Begin an HTTP request to the specified URL
    http.addHeader("Content-Type", "application/json"); // Add a header specifying the content type as JSON
    http.addHeader("Accept", "application/json");       // Add a header specifying the accepted content type as JSON
    http.addHeader("Auth-mode", "key");                 // Add a header specifying the authentication mode as "key"
    http.addHeader("Authorization", CONNECTION_KEY);     // Add a header containing the authorization key

    String valueJsonStr = "{\"reqId\":\"\",\"namespace\":{\"scope\":\"global\",\"id\":\"" + NAMESPACE+"\"},\"key\":\""+KEY+"\"}";

    // Send the POST request with the JSON payload to Anedya server
    int httpResponseCode = http.POST(valueJsonStr);

    // Check if the request was successful
    if (httpResponseCode > 0)
    {
      String response = http.getString(); // Get the response from the server
      // Parse the JSON response
      JsonDocument valueResponse;
      deserializeJson(valueResponse, response); // Extract the JSON response
                                                // Extract the server time from the response
      int errorcode = valueResponse["errorcode"];
      if (errorcode == 0)
      {
        Serial.println("Fetched key-value successfully!!");
        Serial.println(response); // Print the response
      }
      else
      {
        Serial.println("Failed to fetch key-value!!");
        Serial.println(response); // Print the response
      }
    }
    else
    {
      Serial.print("Error on sending POST: "); // Print error message indicating failure to send POST request
      Serial.println(httpResponseCode);        // Print the HTTP response code
    }
    http.end(); // End the HTTP client session
  }
  else
  {
    Serial.println("Error in WiFi connection"); // Print error message indicating WiFi connection failure
  }
}