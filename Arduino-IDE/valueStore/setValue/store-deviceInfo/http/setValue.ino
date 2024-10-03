/*

                                     Store Device Info - Example-ValueStore(http)
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
#include <WiFi.h>         // Include the Wifi to handle the wifi connection
#include <HTTPClient.h>   // Include the HTTPClient library to handle HTTP requests
#include <ArduinoJson.h>  //Include the ArduinoJson library to handle json
#include <TimeLib.h>

/*-----------------------------------------------------variable section------------------------------------------------------------*/
// ----------------------------- Anedya and Wifi credentials --------------------------------------------
String REGION_CODE = "ap-in-1";                   // Anedya region code (e.g., "ap-in-1" for Asia-Pacific/India) | For other country code, visity [https://docs.anedya.io/device/#region]
const char *CONNECTION_KEY = "";  // Fill your connection key, that you can get from your node description
const char *PHYSICAL_DEVICE_ID = ""; // Fill your device Id , that you can get from your node description
const char *SSID = "";     
const char *PASSWORD = ""; 

//-----------------------------------helper variable--------------------------
long long updateInterval,timer,heartbeat_timer;   //varibles to insert interval

//---------------------- Function declaration ----------------------------------
void anedya_setValue(String key, String type, String value);  //function declaration to set the value
void anedya_sendHeartbeat();  //function declaration to send heartbeat

/*-----------------------------------------------------setup section------------------------------------------------------------*/
void setup() {
  Serial.begin(115200);  // Initialize serial communication with  your device compatible baud rate
  delay(1500);           // Delay for 1.5 seconds

  WiFi.begin(SSID, PASSWORD);
  Serial.println();
  Serial.print("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Connected, IP address: ");
  Serial.println(WiFi.localIP());
  timer=millis();
  heartbeat_timer=millis();
} 

void loop() {

  updateInterval=15000;
  if(millis()-timer>=updateInterval){
  String boardInfo = "Chip ID:" + String((uint32_t)ESP.getEfuseMac(), HEX) +", "+
                   "CPU Frequency:" + String(ESP.getCpuFreqMHz()) +", "+
                   "Flash Size:" + String(ESP.getFlashChipSize() / 1024 / 1024) + " MB" +", "+
                   "Free Heap Size:" + String(ESP.getFreeHeap()) +", "+
                   "Sketch Size:" + String(ESP.getSketchSize() / 1024) + " KB" +", "+
                   "Free Sketch Space:" + String(ESP.getFreeSketchSpace() / 1024) + " KB" +", "+
                   "Flash Speed:" + String(ESP.getFlashChipSpeed() / 1000000) + " MHz";

  anedya_setValue("001", "string", boardInfo); /* anedya_setValue("<-KEY->","<-dataType->","<-VALUE->")
                                                 1 parameter- key, 
                                                 2 parameter- The value can hold any of the following types of data: string, binary, float, boolean
                                                 3 parameter- value.  For detailed info, visit-https://docs.anedya.io/valuestore/intro/        */
   timer=millis();                             
  }

  if(millis()-heartbeat_timer>=5000){
    anedya_sendHeartbeat();
    heartbeat_timer=millis();
  }
}

void anedya_setValue(String key, String type, String value)  //function to set the value
{
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;                                                                             // Create an instance of HTTPClient
    String setValue_url = "https://device." + REGION_CODE + ".anedya.io/v1/valuestore/setValue";  // Construct the URL for submitting data


    // Prepare data payload in JSON format
    http.begin(setValue_url);                            // Begin an HTTP request to the specified URL
    http.addHeader("Content-Type", "application/json");  // Add a header specifying the content type as JSON
    http.addHeader("Accept", "application/json");        // Add a header specifying the accepted content type as JSON
    http.addHeader("Auth-mode", "key");                  // Add a header specifying the authentication mode as "key"
    http.addHeader("Authorization", CONNECTION_KEY);      // Add a header containing the authorization key

    // Construct the JSON payload
    String valueJsonStr = "{\"reqId\": \"\",\"key\":\"" + key + "\",\"value\": \"" + value + "\",\"type\": \"" + type + "\"}";

    // Send the POST request with the JSON payload to Anedya server
    int httpResponseCode = http.POST(valueJsonStr);

    // Check if the request was successful
    if (httpResponseCode > 0) {
      String response = http.getString();  // Get the response from the server
      // Parse the JSON response
      JsonDocument valueResponse;
      deserializeJson(valueResponse, response);  // Extract the JSON response
                                                       // Extract the server time from the response
      int errorcode = valueResponse["errorcode"];
      if (errorcode == 0) {  
        Serial.println("Value Set!");
      } else {   
        Serial.println("Failed to set the value!!");
        Serial.println(response);  // Print the response
      }                           
    } else {
      Serial.print("Error on sending POST: ");  // Print error message indicating failure to send POST request
      Serial.println(httpResponseCode);         // Print the HTTP response code
    }
    http.end();  // End the HTTP client session
  } else {
    Serial.println("Error in WiFi connection");  // Print error message indicating WiFi connection failure
  }
} 


//---------------------------------- Function for send heartbeat -----------------------------------
void anedya_sendHeartbeat()
{
  if (WiFi.status() == WL_CONNECTED)
  {
    HTTPClient http;                                                                   // Creating an instance of HTTPClient
    String heartbeat_url = "https://device." + REGION_CODE + ".anedya.io/v1/heartbeat"; // Constructing the URL for submitting data

    // Preparing data payload in JSON format
    http.begin(heartbeat_url);                           // Beginning an HTTP request to the specified URL
    http.addHeader("Content-Type", "application/json"); // Adding a header specifying the content type as JSON
    http.addHeader("Accept", "application/json");       // Adding a header specifying the accepted content type as JSON
    http.addHeader("Auth-mode", "key");                 // Adding a header specifying the authentication mode as "key"
    http.addHeader("Authorization", CONNECTION_KEY);     // Adding a header containing the authorization key

    // Constructing the JSON payload with sensor data and timestamp
    String body_payload = "{}";

    // Sending the POST request with the JSON payload to Anedya server
    int httpResponseCode = http.POST(body_payload);

    // Checking if the request was successful
    if (httpResponseCode > 0)
    {
      String response = http.getString(); // Getting the response from the server
      // Parsing the JSON response
      JsonDocument jsonSubmit_response;
      deserializeJson(jsonSubmit_response, response); // Extracting the JSON response
      int errorcode = jsonSubmit_response["errorcode"];
      if (errorcode == 0) // Error code 0 indicates data submitted successfully
      { 
        Serial.println("Sent Heartbeat");
      }
      else
      { 
        Serial.println("Failed to sent heartbeat!!");
        Serial.println(response);  //error code4020 indicate -unknown variable identifier
      }   
    }                        
    else
    {
      Serial.print("Error on sending POST: "); // Printing error message indicating failure to send POST request
      Serial.println(httpResponseCode);        // Printing the HTTP response code
    }
    http.end(); // Ending the HTTP client session
  }
  else
  {
    Serial.println("Error in WiFi connection"); // Printing error message indicating WiFi connection failure
  }
}
