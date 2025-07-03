/*

                                           Submit Data to Anedya
                                  =======================================

                          # Dashboard Setup
                            - Create account and login to the dashboard
                            - Create new project.
                            - Create variables: temperature and humidity.
                            - Create a node (e.g., for home- Room1 or study room).

                          Notes: Variable Identifier is essential; fill it accurately.

                          # Hardware Setup
                            - Connect the EC200U module to the 5v 2amp power supply.
                            - Establish serial communication with the EC200U module .

                       Notes: The code is tested on the "Esp-32 S3 Dev Board".

                                                                    Dated: 07-Dec-2024
*/
#include <Arduino.h>
#include <TimeLib.h>

#define EC200U Serial1 // Define Serial1 for the EC200U module

// Configuration Variables
String REGION_CODE = "ap-in-1";
const char *CONNECTION_KEY = "CONNECTION_KEY";
const char *PHYSICAL_DEVICE_ID = "PHYSICAL_DEVICE_ID";

// Root CA Certificate
const char *anedyaecc_cert = R"EOF(
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

String sendATCommand(String command, int timeout);
void EC200U_Module_Setup();
void uploadSSLCertificate();
void https_context_setup();
void syncDeviceTimeViaNTP();
void anedya_sendHeartbeat();
void anedya_submitData(const char *variable_identifier, float value, long long timestamp);

void setup()
{
  Serial.begin(115200);
  EC200U.begin(115200, SERIAL_8N1, 16, 17); // Modem TX-> ESP32S3GPIO 16, RX->ESP32S3 GPIO 17

  Serial.println("Initializing...");
  delay(2000);

  EC200U_Module_Setup();
  uploadSSLCertificate();
  https_context_setup();
  syncDeviceTimeViaNTP(); // Sync the device time
}

void loop()
{
  Serial.println("===============================================");
  anedya_sendHeartbeat();

  const char *variable_identifier = "temperature";
  float variable_value = 26.00;
  long long timestamp = now();
  long long timestamp_ms = timestamp * 1000;
  anedya_submitData(variable_identifier, variable_value, timestamp_ms);

  Serial.println("===============================================");
  delay(15000);
}

String sendATCommand(String command, int timeout)
{
  Serial.println("Command sent: " + command);
  EC200U.println(command); // Send the AT command to the EC200U module

  long long startTime = millis();
  String response = "";
  while (millis() - startTime < timeout)
  {
    // Check if data is available to read from the module
    while (EC200U.available())
    {
      char c = EC200U.read();
      response += c;
    }
    // Optionally add a small delay to reduce CPU usage during the wait
    delay(10);
  }
  if ((millis() - startTime > timeout) && response == "")
  {
    Serial.println("Timeout reached for command: " + command);
    return "Timeout";
  }
  Serial.println("Response: " + response);
  return response;
}

void EC200U_Module_Setup()
{

  // Check Connection Status
  sendATCommand("AT", 500);

  // Restores the module to its original factory configuration.
  sendATCommand("AT&F", 1000);

  // check the network registration status of the module.
  sendATCommand("AT+CREG?", 500);

  // Check Signal Quality
  sendATCommand("AT+CSQ", 500);

  // Activate a PDP (Packet Data Protocol) context
  sendATCommand("AT+CGACT=1,1", 500);
  // Activate GATT
  sendATCommand("AT+CGATT=1", 500);
}

void uploadSSLCertificate()
{
  int certLength = strlen(anedyaecc_cert);

  String cmd = "AT+QFOPEN=\"anedyaecc_crt.pem\",1";
  sendATCommand(cmd, 500);

  /* NOTE:
  The file handle returned by the AT+QFOPEN command is required to write the content of the file.
  Here, the default file handle 1027 is used, but please note that it is recommended to use the actual file handle returned by the AT+QFOPEN command.
  */
  String writeFile_cmd = "AT+QFWRITE=1027," + String(certLength) + ",20";
  sendATCommand(writeFile_cmd, 500);

  sendATCommand(anedyaecc_cert, 1000);   // upload certificate
  sendATCommand("AT+QFCLOSE=1027", 500); // close file
  sendATCommand("AT+QFLST", 300);        // list file
}

void https_context_setup()
{
  Serial.println("Setting up HTTP Context...");

  // Configure SSL context
  sendATCommand("AT+QSSLCFG=\"sslversion\",1,4", 500);
  sendATCommand("AT+QSSLCFG=\"seclevel\",1,2", 500);
  sendATCommand("AT+QSSLCFG=\"cacert\",1,\"anedyaecc_cert.pem\"", 500);
  sendATCommand("AT+QHTTPCFG=\"sslctxid\",1", 500); // Bind SSL context to HTTP

  // Configure HTTP context
  sendATCommand("AT+QHTTPCFG=\"contextid\",1", 500);

  sendATCommand("AT+QHTTPCFG=\"responseheader\",1", 500);

  sendATCommand("AT+QHTTPCFG=\"contenttype\",1", 500);

  sendATCommand("AT+QHTTPCFG=\"rspout/auto\",1", 500);

  sendATCommand("AT+QHTTPCFG=\"closed/ind\",1", 500);

  // Set headers for the HTTP request
  sendATCommand("AT+QHTTPCFG=\"header\",\"Content-Type: application/json\"", 500);

  sendATCommand("AT+QHTTPCFG=\"header\",\"Accept: application/json\"", 500);

  sendATCommand("AT+QHTTPCFG=\"header\",\"Auth-mode: key\"", 500);

  sendATCommand("AT+QHTTPCFG=\"header\",\"Authorization:" + String(CONNECTION_KEY) + "\"", 500);

  // Check the final configuration
  sendATCommand("AT+QHTTPCFG?", 1000);
}
void syncDeviceTimeViaNTP()
{
  int maxRetries = 3;
  bool success = false;

  for (int attempt = 1; attempt <= maxRetries; attempt++)
  {
    Serial.println("Attempting NTP sync: Attempt " + String(attempt));

    // Send NTP request
    String response = sendATCommand("AT+QNTP=1,\"pool.ntp.org\",123", 5000);

    // Print the raw response for debugging
    Serial.println("Raw NTP Response: " + response);

    // Check if the response contains time data
    if (response.indexOf("+QNTP: 0") != -1)
    {
      Serial.println("NTP synchronization successful!");
      String ntpRequest = "AT+QNTP=1,\"pool.ntp.org\",123";
      response.replace(ntpRequest, "");

      // Extract and clean up the response to get only the date-time string
      int startIdx = response.indexOf("\"") + 1; // Find first quote
      int endIdx = response.lastIndexOf("\"");   // Find last quote

      // Check if the time string exists within quotes and extract it
      if (startIdx != -1 && endIdx != -1 && endIdx > startIdx)
      {
        String dateTimeStr = response.substring(startIdx, endIdx); // Extract date-time
        Serial.println("Extracted Date-Time: " + dateTimeStr);

        // Parse the date-time string based on the format: "YYYY/MM/DD,HH:MM:SS+TZ"
        int year, month, day, hour, minute, second;
        int parsed = sscanf(dateTimeStr.c_str(), "%d/%d/%d,%d:%d:%d", &year, &month, &day, &hour, &minute, &second);

        if (parsed != 6)
        {
          Serial.println("Error parsing date-time string.");
          return;
        }

        // Debugging: Print parsed values
        Serial.printf("Parsed Date: %d/%d/%d %d:%d:%d\n", year, month, day, hour, minute, second);

        // Convert to UNIX timestamp
        tmElements_t tm;
        tm.Year = year - 1970; // tm.Year is offset from 1970
        tm.Month = month;
        tm.Day = day;
        tm.Hour = hour;
        tm.Minute = minute;
        tm.Second = second;

        // Convert time to epoch using makeTime
        time_t epochTime = makeTime(tm); // Convert to epoch
        Serial.println("Epoch Time before timezone adjustment: " + String(epochTime));

        // Print the adjusted epoch time
        Serial.println("Epoch Time (UTC): " + String(epochTime));

        // Set the system time
        setTime(epochTime);

        // Print local time for debugging
        Serial.print("Device time updated to IST! Epoch Time: ");
        Serial.println(epochTime);

        success = true;
        break; // Exit retry loop
      }
    }
    else if (response.indexOf("+QNTP: 1") != -1)
    {
      Serial.println("NTP synchronization failed! Error code: 1");
    }
    else
    {
      Serial.println("Unexpected response: " + response);
    }

    // Retry logic
    if (!success)
    {
      Serial.println("Retrying NTP synchronization...");
      delay(2000); // Wait before retrying
    }
  }

  if (!success)
  {
    Serial.println("Failed to synchronize time after " + String(maxRetries) + " attempts.");
  }
}

void anedya_submitData(const char *variable_identifier, float value, long long timestamp)
{
  // Set URL for HTTP request
  sendATCommand("AT+QHTTPCFG=\"url\",\"https://device.ap-in-1.anedya.io/v1/submitData\"", 400);

  // HTTP POST command
  String payload = "{\"data\":[{\"variable\":\"" + String(variable_identifier) + "\",\"value\":" + String(value) + ",\"timestamp\": " + String(timestamp) + "}]}";
  Serial.println("Payload : " + payload);
  String postCmd = "AT+QHTTPPOST=" + String(payload.length()) + ",80,80";
  if (sendATCommand(postCmd, 5000).indexOf("CONNECT") != -1)
  {
    sendATCommand(payload, 300); // Send JSON payload
  }
}

void anedya_sendHeartbeat()
{
  // Set URL for HTTP request
  sendATCommand("AT+QHTTPCFG=\"url\",\"https://device.ap-in-1.anedya.io/v1/heartbeat\"", 400);

  String payload = "{\"reqId\": \"Heartbeat\"}";

  // HTTP POST command
  String postCmd = "AT+QHTTPPOST=" + String(payload.length()) + ",80,80";
  if (sendATCommand(postCmd, 5000).indexOf("CONNECT") != -1)
  {
    sendATCommand(payload, 300); // Send JSON payload
  }
}
