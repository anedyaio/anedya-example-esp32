/*

                            Room Monitoring Using ESP32 + DHT sensor (http)
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

                                                                                           Dated: 12-March-2024

*/
#include <Arduino.h>

// Emulate Hardware Sensor?
bool virtual_sensor = true;

#include <WiFi.h>        // Include the Wifi to handle the wifi connection
#include <HTTPClient.h>  // Include the HTTPClient library to handle HTTP requests
#include <ArduinoJson.h> // Include the Arduino library to make json or abstract the value from the json
#include <TimeLib.h>     // Include the Time library to handle time synchronization with ATS (Anedya Time Services)
#include <DHT.h>         // Include the DHT library for humidity and temperature sensor handling

/*-----------------------------------------Variable section------------------------------------------------------------------*/
//---------------essential variable------------------
String regionCode = "ap-in-1";              // Specify the Anedya region code (e.g., "ap-in-1" for Asia-Pacific/India) | For other country codes, visit [https://docs.anedya.io/device/#region]
String deviceId = "<PHYSICAL-DEVICE-UUID>"; // Fill in your device Id, which you can obtain from your node description
String connectionKey = "<CONNECTION-KEY";  // Fill in your connection key, which you can obtain from your node description
const char* ssid = "<SSID>";     // Enter your WiFi network's SSID
const char* password = "<PASSWORD>"; // Enter your WiFi network's password

//-----------DHT sensor variable----------------
#define DHT_TYPE DHT11 // Define the type of DHT sensor (DHT11, DHT21, DHT22, AM2301, AM2302, AM2321)
#define DHT_PIN 5 // Define the pin connected to the DHT sensor, marked as D5 on the ESP32
float temperature;  // variable to store temperature readings from the DHT sensor
float humidity;  //  variable to store humidity readings from the DHT sensor

/*------------------------------------Function declarations--------------------------------------------------------------------*/
void setDevice_time();                                       // Function to configure the device time with real-time from ATS (Anedya Time Services)
void anedya_submitData(String datapoint, float sensor_data); // Function to submit data to the Anedya server

/*------------------------------------Object initializing----------------------------------------------------------------------*/
DHT dht(DHT_PIN, DHT_TYPE); // Initialize the DHT sensor object with specified pin and type

/*----------------------------------------SETUP SECTION-------------------------------------------------------------------------------*/
void setup()
{
  Serial.begin(115200); // Initializing serial communication, fill braud rate according to your device compatible
  delay(1500);          // Delaying for 1.5 seconds

  // Connecting to WiFi network
  WiFi.begin(ssid, password);
  Serial.println();
  Serial.print("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Connecting, IP address: ");
  Serial.println(WiFi.localIP());

  // Connecting to ATS(Anedya Time Services) and configuring time synchronization
  setDevice_time(); // Calling function to configure time synchronization

  // Initializing the DHT sensor
  dht.begin();
}

/*----------------------------------------------LOOP SECTION---------------------------------------------------------------------------*/

void loop()
{
  if (!virtual_sensor)
  {
    // Reading the temperature and humidity from the DHT sensor
    Serial.println("Fetching data from the Physical sensor");
    temperature = dht.readTemperature(); // Reading temperature
    humidity = dht.readHumidity(); // Reading humidity
    if (isnan(humidity) || isnan(temperature))
    {
      Serial.println("Failing to read from DHT !"); 
      delay(10000);
      return;
    }
  }
  else
  {
    // Generating random temperature and humidity values
    Serial.println("Fetching data from the Virtual sensor"); 
    temperature = random(20, 30); 
    humidity = random(60, 80); 
  }
  Serial.print("Temperature : "); 
  Serial.println(temperature); 
  // Submitting sensor data to Anedya server
  anedya_submitData("temperature", temperature); // submitting temperature data to the Anedya

  Serial.print("Humidity : "); 
  Serial.println(humidity); 
  anedya_submitData("humidity", humidity); // submitting humidity data to the Anedya

  delay(15000); 
}

/*<--------------------------------------Function Defination section------------------------------------------------------------------------->*/
// ----------------------------Function to configure time synchronization with Anedya server-----------
// For more info, visit [https://docs.anedya.io/device/api/http-time-sync/]
void setDevice_time()
{
  String time_url = "https://device." + regionCode + ".anedya.io/v1/time"; // url to fetch the real time from ther ATS (Anedya Time Services)

  Serial.println("Time synchronizing......");
  int timeCheck = 1; // iterating to re-sync to ATS (Anedya Time Services), in case of failed attempt
  while (timeCheck)
  {
    long long deviceSendTime = millis();

    // Preparing the request payload
    JsonDocument requestPayload;            // Declaring a JSON document with a capacity of 200 bytes
    requestPayload["deviceSendTime"] = deviceSendTime; // Adding a key-value pair to the JSON document
    String jsonPayload;                                // Declaring a string to store the serialized JSON payload
    serializeJson(requestPayload, jsonPayload);        // Serializing the JSON document into a string

    HTTPClient http;                                    // Creating an instance of HTTPClient
    http.begin(time_url);                               // Beginning an HTTP request to the specified URL
    http.addHeader("Content-Type", "application/json"); // Adding a header specifying the content type as JSON

    int httpResponseCode = http.POST(jsonPayload); // Sending a POST request with the serialized JSON payload

    // Checking if the request was successful
    if (httpResponseCode != 200)
    {
      Serial.println("Failing to fetch server time! Retrying........"); // Printing an error message indicating failure to fetch server time
      delay(2000);                                    // Delaying for 2 seconds in case of failure
    }
    else if (httpResponseCode == 200)
    {
      timeCheck = 0; // Setting timeCheck to 0 to indicate successful time synchronization
      Serial.println("synchronizing!");
    }

    // Parsing the JSON response
    JsonDocument jsonResponse;           // Declaring a JSON document with a capacity of 200 bytes
    deserializeJson(jsonResponse, http.getString()); // Deserializing the JSON response from the server into the JSON document

    long long serverReceiveTime = jsonResponse["serverReceiveTime"]; // Getting the server receive time from the JSON document
    long long serverSendTime = jsonResponse["serverSendTime"];       // Getting the server send time from the JSON document

    // Computing the current time
    long long deviceRecTime = millis();                                                             
    long long currentTime = (serverReceiveTime + serverSendTime + deviceRecTime - deviceSendTime) / 2; // Computing the current time
    long long currentTimeSeconds = currentTime / 1000;                                                 // Converting current time to seconds

    // Setting device time
    setTime(currentTimeSeconds); // Setting the device time based on the computed current time
  }
}

//---------------------------------- Function for submitting data-----------------------------------
// For more info, visit [https://docs.anedya.io/devicehttpapi/submitdata/]
void anedya_submitData(String datapoint, float sensor_data)
{
  if (WiFi.status() == WL_CONNECTED)
  {
    HTTPClient http;                                                                   // Creating an instance of HTTPClient
    String submitData_url = "https://device." + regionCode + ".anedya.io/v1/submitData"; // Constructing the URL for submitting data

    // Getting current device time and converting it to milliseconds
    long long current_time = now();                     // Getting the current time
    long long current_time_milli = current_time * 1000; // Converting current time to milliseconds

    // Preparing data payload in JSON format
    http.begin(submitData_url);                           // Beginning an HTTP request to the specified URL
    http.addHeader("Content-Type", "application/json"); // Adding a header specifying the content type as JSON
    http.addHeader("Accept", "application/json");       // Adding a header specifying the accepted content type as JSON
    http.addHeader("Auth-mode", "key");                 // Adding a header specifying the authentication mode as "key"
    http.addHeader("Authorization", connectionKey);     // Adding a header containing the authorization key

    // Constructing the JSON payload with sensor data and timestamp
    String submitData_str = "{\"data\":[{\"variable\": \"" + datapoint + "\",\"value\":" + String(sensor_data) + ",\"timestamp\":" + String(current_time_milli) + "}]}";

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
        Serial.println("Data pushed to Anedya Cloud!");
      }
      else
      { 
        Serial.println("Failed to push!!");
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
