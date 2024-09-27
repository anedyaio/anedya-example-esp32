/*

                            Room Monitoring Using ESP32 + DHT sensor With Device log submission (http)
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

                                                                                           Dated: 22-March-2024

*/
#include <Arduino.h>

// Emulate Hardware Sensor?
bool virtual_sensor = false;

#include <WiFi.h>        // Include the Wifi to handle the wifi connection
#include <HTTPClient.h>  // Include the HTTPClient library to handle HTTP requests
#include <ArduinoJson.h> // Include the Arduino library to make json or abstract the value from the json
#include <TimeLib.h>     // Include the Time library to handle time synchronization with ATS (Anedya Time Services)
#include <DHT.h>         // Include the DHT library for humidity and temperature sensor handling

String regionCode = "ap-in-1";              // Anedya region code (e.g., "ap-in-1" for Asia-Pacific/India) | For other country code, visity [https://docs.anedya.io/device/#region]
String deviceID = "<PHYSICAL-DEVICE-UUID>"; // Fill your device Id , that you can get from your node description
String connectionKey = "<CONNECTION-KEY>";  // Fill your connection key, that you can get from your node description

// Define the type of DHT sensor (DHT11, DHT21, DHT22, AM2301, AM2302, AM2321)
#define DHT_TYPE DHT11
// Define the pin connected to the DHT sensor
#define DHT_PIN 5 // pin marked as D23 on the ESP32
float temperature;
float humidity;

// Your WiFi credentials
char ssid[] = "<SSID>";     // Your WiFi network SSID
char pass[] = "<PASSWORD>"; // Your WiFi network password

// Function declarations
void setDevice_time();                                       // Function to configure the device time with real-time from ATS (Anedya Time Services)
void anedya_submitData(String datapoint, float sensor_data); // Function to submit data to the Anedya server
void anedya_sumitLog(String reqID, String Log);
void anedya_sendHeartbeat();                                 // Function to send heartbeat

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

  // Connect to ATS(Anedya Time Services) and configure time synchronization
  setDevice_time(); // Call function to configure time synchronization

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
      static int counter;
      counter++;
      String deviceLog = "[" + String(counter) + "] Failed to read from DHT !";
      Serial.println(deviceLog);       // Output error message to serial console
      anedya_submitLog("", deviceLog); // anedya_submitLog("<--request ID-->", "<--Logs-->"); request id is optional
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

  anedya_sendHeartbeat();

  delay(15000);
}
//<---------------------------------------------------------------------------------------------------------------------------->
// Function to configure time synchronization with Anedya server
// For more info, visit [https://docs.anedya.io/device/api/http-time-sync/]
void setDevice_time()
{
  String time_url = "https://device." + regionCode + ".anedya.io/v1/time"; // url to fetch the real time from ther ATS (Anedya Time Services)

  // Attempt to synchronize time with Anedya server
  Serial.println("Time synchronizing......");
  int timeCheck = 1; // iteration to re-sync to ATS (Anedya Time Services), in case of failed attempt
  while (timeCheck)
  {
    // Get the device send time
    long long deviceSendTime = millis();

    // Prepare the request payload
    JsonDocument requestPayload;            // Declare a JSON document with a capacity of 200 bytes
    requestPayload["deviceSendTime"] = deviceSendTime; // Add a key-value pair to the JSON document
    String jsonPayload;                                // Declare a string to store the serialized JSON payload
    serializeJson(requestPayload, jsonPayload);        // Serialize the JSON document into a string

    HTTPClient http;                                    // Create an instance of HTTPClient
    http.begin(time_url);                               // Begin an HTTP request to the specified URL
    http.addHeader("Content-Type", "application/json"); // Add a header specifying the content type as JSON

    int httpResponseCode = http.POST(jsonPayload); // Send a POST request with the serialized JSON payload

    // Check if the request was successful
    if (httpResponseCode != 200)
    {
      Serial.println("Failed to fetch server time!"); // Print error message indicating failure to fetch server time
      delay(2000);                                    // Delay for 2 seconds
    }
    else if (httpResponseCode == 200)
    {
      timeCheck = 0; // Set timeCheck to 0 to indicate successful time synchronization
      Serial.println("synchronized!");
    }

    // Parse the JSON response
    JsonDocument jsonResponse;           // Declare a JSON document with a capacity of 200 bytes
    deserializeJson(jsonResponse, http.getString()); // Deserialize the JSON response from the server into the JSON document

    long long serverReceiveTime = jsonResponse["serverReceiveTime"]; // Get the server receive time from the JSON document
    long long serverSendTime = jsonResponse["serverSendTime"];       // Get the server send time from the JSON document

    // Compute the current time
    long long deviceRecTime = millis();                                                                 // Get the device receive time
    long long currentTime = (serverReceiveTime + serverSendTime + deviceRecTime - deviceSendTime) / 2; // Compute the current time
    long long currentTimeSeconds = currentTime / 1000;                                                 // Convert current time to seconds

    // Set device time
    setTime(currentTimeSeconds); // Set the device time based on the computed current time
  }
}

// Function to submit data to Anedya server
// For more info, visit [https://docs.anedya.io/devicehttpapi/submitdata/]
void anedya_submitData(String datapoint, float sensor_data)
{
  if (WiFi.status() == WL_CONNECTED)
  {
    HTTPClient http;                                                                   // Create an instance of HTTPClient
    String senddata_url = "https://device." + regionCode + ".anedya.io/v1/submitData"; // Construct the URL for submitting data

    // Get current time and convert it to milliseconds
    long long current_time = now();                     // Get the current time
    long long current_time_milli = current_time * 1000; // Convert current time to milliseconds

    // Prepare data payload in JSON format
    http.begin(senddata_url);                           // Begin an HTTP request to the specified URL
    http.addHeader("Content-Type", "application/json"); // Add a header specifying the content type as JSON
    http.addHeader("Accept", "application/json");       // Add a header specifying the accepted content type as JSON
    http.addHeader("Auth-mode", "key");                 // Add a header specifying the authentication mode as "key"
    http.addHeader("Authorization", connectionKey);     // Add a header containing the authorization key

    // Construct the JSON payload with sensor data and timestamp
    String jsonStr = "{\"data\":[{\"variable\": \"" + datapoint + "\",\"value\":" + String(sensor_data) + ",\"timestamp\":" + String(current_time_milli) + "}]}";

    // Send the POST request with the JSON payload to Anedya server
    int httpResponseCode = http.POST(jsonStr);

    // Check if the request was successful
    if (httpResponseCode > 0)
    {
      String response = http.getString(); // Get the response from the server
      // Parse the JSON response
      JsonDocument jsonSubmit_response;
      deserializeJson(jsonSubmit_response, response); // Extract the JSON response
                                                      // Extract the server time from the response
      int errorcode = jsonSubmit_response["errorcode"];
      if (errorcode == 0)
      { // error code  0 means data submitted successfull
        Serial.println("Data pushed to Anedya Cloud!");
      }
      else
      { // other errocode means failed to push (like: 4020- mismatch variable identifier...)
        Serial.println("Failed to push!!");
        Serial.println(response); // Print the response
      }                           // Print the response
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

void anedya_submitLog(String reqID, String Log)
{
  if (WiFi.status() == WL_CONNECTED)
  {
    HTTPClient http;                                                                   // Create an instance of HTTPClient
    String sendLogs_url = "https://device." + regionCode + ".anedya.io/v1/logs/submitLogs"; // Construct the URL for submitting data

    // Get current time and convert it to milliseconds
    long long current_time = now();                     // Get the current time
    long long current_time_milli = current_time * 1000; // Convert current time to milliseconds

    // Prepare data payload in JSON format
    http.begin(sendLogs_url);                           // Begin an HTTP request to the specified URL
    http.addHeader("Content-Type", "application/json"); // Add a header specifying the content type as JSON
    http.addHeader("Accept", "application/json");       // Add a header specifying the accepted content type as JSON
    http.addHeader("Auth-mode", "key");                 // Add a header specifying the authentication mode as "key"
    http.addHeader("Authorization", connectionKey);     // Add a header containing the authorization key

      // Construct the JSON payload with deveice log and timestamp
    String strLog = "{\"reqId\":\"" + reqID + "\",\"data\":[{\"timestamp\":" + String(current_time_milli) + ",\"log\":\"" + Log + "\"}]}";

    // Send the POST request with the JSON payload to Anedya server
    int httpResponseCode = http.POST(strLog);

    // Check if the request was successful
    if (httpResponseCode > 0)
    {
      String response = http.getString(); // Get the response from the server
      // Parse the JSON response
      JsonDocument jsonSubmit_response;
      deserializeJson(jsonSubmit_response, response); // Extract the JSON response
                                                      // Extract the server time from the response
      int errorcode = jsonSubmit_response["errorcode"];
      if (errorcode == 0)
      { // error code  0 means data submitted successfull
        Serial.println("Log pushed to Anedya Cloud!");
      }
      else
      { // other errocode means failed to push (like: 4020- mismatch variable identifier...)
        Serial.println("Failed to push!!");
        Serial.println(response); // Print the response
      }                           // Print the response
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


//---------------------------------- Function for send heartbeat -----------------------------------
void anedya_sendHeartbeat()
{
  if (WiFi.status() == WL_CONNECTED)
  {
    HTTPClient http;                                                                   // Creating an instance of HTTPClient
    String submitData_url = "https://device." + regionCode + ".anedya.io/v1/heartbeat"; // Constructing the URL for submitting data



    // Preparing data payload in JSON format
    http.begin(submitData_url);                           // Beginning an HTTP request to the specified URL
    http.addHeader("Content-Type", "application/json"); // Adding a header specifying the content type as JSON
    http.addHeader("Accept", "application/json");       // Adding a header specifying the accepted content type as JSON
    http.addHeader("Auth-mode", "key");                 // Adding a header specifying the authentication mode as "key"
    http.addHeader("Authorization", connectionKey);     // Adding a header containing the authorization key

    // Constructing the JSON payload with sensor data and timestamp
    String submitData_str = "{}";

    // Sending the POST request with the JSON payload to Anedya server
    int httpResponseCode = http.POST(submitData_str);

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

