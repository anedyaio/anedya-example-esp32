/*

                                Anedya OTA (Over The Air) Firmware Update
                                    # Dashboard Setup
                                        1. Create account and login
                                        2. Create new project.
                                        3. Create a node (e.g., for home- Room1 or study room).
                                        4. Upload the Asset.
                                        5. Create a Deploymnent.


                  Note: The code is tested on the "Esp-32 Wifi, Bluetooth, Dual Core Chip Development Board (ESP-WROOM-32)"

                                                                                                          Dated: 04-12-2024
*/
#include <Arduino.h>
#include <WiFi.h> // Include the Wifi to handle the wifi connection
#include "HttpsOTAUpdate.h"
#include <HTTPClient.h>          // Include the HTTPClient library to handle HTTP requests
#include <NetworkClientSecure.h> // Include the NetworkClientSecure library to handle secure HTTP requests(https)
#include <ArduinoJson.h>         // Include the Arduino library to make json or abstract the value from the json
#include <TimeLib.h>             // Include the Time library to handle time synchronization with ATS (Anedya Time Services)

/*-----------------------------------------Variable section------------------------------------------------------------------*/
// ----------------------------- Anedya and Wifi credentials --------------------------------------------
String REGION_CODE = "ap-in-1";      // Anedya region code (e.g., "ap-in-1" for Asia-Pacific/India) | For other country code, visity [https://docs.anedya.io/device/#region]
const char *CONNECTION_KEY = "";     // Fill your connection key, that you can get from your node description
const char *PHYSICAL_DEVICE_ID = ""; // Fill your device Id , that you can get from your node description
const char *SSID = "";
const char *PASSWORD = "";

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

// ----------------------------- Helper Variable --------------------------------------------
long last_check_for_ota_update = 0;
long check_for_ota_update_interval = 10000;

/*------------------------------------Object initializing----------------------------------------------------------------------*/
NetworkClientSecure ncc_client; // Initialize the NetworkClientSecure object
static HttpsOTAStatus_t otastatus;

/*------------------------------------Function declarations--------------------------------------------------------------------*/
void setDevice_time(); // Function to configure the device time with real-time from ATS (Anedya Time Services)
bool anedya_check_ota_update(char *assetURL, char *deploymentID);
void anedya_update_ota_status(char *deploymentID, char *deploymentStatus);
void anedya_sendHeartbeat();

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
    case HTTP_EVENT_REDIRECT:
        Serial.println("Http Event Redirect");
        break;
    }
}

/*----------------------------------------SETUP SECTION-------------------------------------------------------------------------------*/
void setup()
{
    Serial.begin(115200); // Initializing serial communication, fill braud rate according to your device compatible
    delay(3000);          // Delaying for 1.5 seconds

    // Connecting to WiFi network
    WiFi.begin(SSID, PASSWORD);
    Serial.println();
    Serial.print("Connecting to WiFi...");
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    Serial.println();
    Serial.print("Connecting to v2, IP address: ");
    Serial.println(WiFi.localIP());

    ncc_client.setCACert(ca_cert); // Set Root CA certificate
    HttpsOTA.onHttpEvent(HttpEvent);

    last_check_for_ota_update = millis();

    // Connecting to ATS(Anedya Time Services) and configuring time synchronization
    setDevice_time(); // Calling function to configure time synchronization
}
/*----------------------------------------------LOOP SECTION---------------------------------------------------------------------------*/

void loop()
{
    if (millis() - last_check_for_ota_update >= check_for_ota_update_interval)
    {
        char assetURL[300];
        char deploymentID[50];
        if (anedya_check_ota_update(assetURL, deploymentID))
        {
            Serial.println("assetURL: " + String(assetURL));

            Serial.println("Deployment ID: " + String(deploymentID));
            anedya_update_ota_status(deploymentID, "start");

            Serial.println("Starting OTA");
            HttpsOTA.begin(assetURL, ca_cert,false);
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
        }else{
            Serial.println("No update available.");
        }
    }
    anedya_sendHeartbeat(); // sending heartbeat
    delay(5000);
}

// ----------------------------Function to synchronize time -----------
// For more info, visit [https://docs.anedya.io/device/api/http-time-sync/]
void setDevice_time()
{
    String time_url = "https://device." + REGION_CODE + ".anedya.io/v1/time"; // url to fetch the real time from ther ATS (Anedya Time Services)

    Serial.println("Time synchronizing......");
    int timeCheck = 1; // iterating to re-sync to ATS (Anedya Time Services), in case of failed attempt
    while (timeCheck)
    {
        long long deviceSendTime = millis();

        // Preparing the request payload
        JsonDocument requestPayload;                       // Declaring a JSON document with a capacity of 200 bytes
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
            delay(2000);                                                      // Delaying for 2 seconds in case of failure
        }
        else if (httpResponseCode == 200)
        {
            timeCheck = 0; // Setting timeCheck to 0 to indicate successful time synchronization
            Serial.println("synchronizing!");
        }

        // Parsing the JSON response
        JsonDocument jsonResponse;                       // Declaring a JSON document with a capacity of 200 bytes
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

// ---------------------- Function to check for OTA update ----------------------
bool anedya_check_ota_update(char *assetURL, char *deploymentID)
{
    if (WiFi.status() == WL_CONNECTED)
    {
        HTTPClient http;                                                                    // Creating an instance of HTTPClient
        String getNextOTA_url = "https://device." + REGION_CODE + ".anedya.io/v1/ota/next"; // Constructing the URL for submitting data

        // Preparing data payload in JSON format
        http.begin(getNextOTA_url);                         // Beginning an HTTP request to the specified URL
        http.addHeader("Content-Type", "application/json"); // Adding a header specifying the content type as JSON
        http.addHeader("Accept", "application/json");       // Adding a header specifying the accepted content type as JSON
        http.addHeader("Auth-mode", "key");                 // Adding a header specifying the authentication mode as "key"
        http.addHeader("Authorization", CONNECTION_KEY);    // Adding a header containing the authorization key

        http.setTimeout(5000);

        String getNextOTA_payload = "{}";
        // Sending the POST request with the JSON payload to Anedya server
        int httpResponseCode = http.POST(getNextOTA_payload);

        // Checking if the request was successful
        if (httpResponseCode > 0)
        {
            String response = http.getString(); // Getting the response from the server
            // Parsing the JSON response
            JsonDocument getNextOTA_response;
            deserializeJson(getNextOTA_response, response); // Extracting the JSON response
            int errorcode = getNextOTA_response["errorcode"];
            if (errorcode == 0) // Error code 0 indicates data submitted successfully
            {
                bool deploymentAvailablity = getNextOTA_response["deploymentAvailable"];
                if (deploymentAvailablity)
                {
                    String assetURL_str = getNextOTA_response["data"]["asseturl"].as<String>();
                    String deploymentID_str = getNextOTA_response["data"]["deploymentId"].as<String>();
                    strcpy(assetURL, assetURL_str.c_str());
                    strcpy(deploymentID, deploymentID_str.c_str());
                    return true;
                }
                else
                {
                    return false;
                }
            }
            else
            {
                Serial.println("API Error!!");
                Serial.println(response); // error code4020 indicate -unknown variable identifier
                return false;
            }
        }
        else
        {
            Serial.print("Error on sending POST: "); // Printing error message indicating failure to send POST request
            Serial.println(httpResponseCode);        // Printing the HTTP response code
            return false;
        }
        http.end(); // Ending the HTTP client session
    }
    else
    {
        Serial.println("Error in WiFi connection"); // Printing error message indicating WiFi connection failure
        return false;
    }
}
// ---------------------- Function to update OTA Status ----------------------
void anedya_update_ota_status(char *deploymentID, char *deploymentStatus)
{
    if (WiFi.status() == WL_CONNECTED)
    {
        HTTPClient http;                                                                 // Creating an instance of HTTPClient
        String url = "https://device." + REGION_CODE + ".anedya.io/v1/ota/updateStatus"; // Constructing the URL for submitting data

        // Preparing data payload in JSON format
        http.begin(url);                                    // Beginning an HTTP request to the specified URL
        http.addHeader("Content-Type", "application/json"); // Adding a header specifying the content type as JSON
        http.addHeader("Accept", "application/json");       // Adding a header specifying the accepted content type as JSON
        http.addHeader("Auth-mode", "key");                 // Adding a header specifying the authentication mode as "key"
        http.addHeader("Authorization", CONNECTION_KEY);    // Adding a header containing the authorization key

        String body = "{\"reqId\": \"Update Status\",\"deploymentId\": \"" + String(deploymentID) + "\",\"status\": \"" + String(deploymentStatus) + "\",\"log\": \"string\"}";

        // Sending the POST request with the JSON payload to Anedya server
        http.setTimeout(5000);
        int httpResponseCode = http.POST(body);

        // Checking if the request was successful
        if (httpResponseCode > 0)
        {
            String response_str = http.getString(); // Getting the response from the server
            // Parsing the JSON response
            JsonDocument response_json;
            deserializeJson(response_json, response_str); // Extracting the JSON response
            int errorcode = response_json["errorcode"];
            if (errorcode == 0) // Error code 0 indicates data submitted successfully
            {
                Serial.println("Status Updated to " + String(deploymentStatus));
            }
            else
            {
                Serial.println("Failed to Update Deployment Status!!");
                Serial.println(response_str);
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

//---------------------------------- Function to send heartbeat -----------------------------------
void anedya_sendHeartbeat()
{
    if (WiFi.status() == WL_CONNECTED)
    {
        HTTPClient http;                                                                     // Creating an instance of HTTPClient
        String submitData_url = "https://device." + REGION_CODE + ".anedya.io/v1/heartbeat"; // Constructing the URL for submitting data

        // Preparing data payload in JSON format
        http.begin(submitData_url);                         // Beginning an HTTP request to the specified URL
        http.addHeader("Content-Type", "application/json"); // Adding a header specifying the content type as JSON
        http.addHeader("Accept", "application/json");       // Adding a header specifying the accepted content type as JSON
        http.addHeader("Auth-mode", "key");                 // Adding a header specifying the authentication mode as "key"
        http.addHeader("Authorization", CONNECTION_KEY);    // Adding a header containing the authorization key

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
                Serial.println(response); // error code4020 indicate -unknown variable identifier
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
