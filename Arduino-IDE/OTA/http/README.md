[<img src="https://img.shields.io/badge/Anedya-Documentation-blue?style=for-the-badge">](https://docs.anedya.io?utm_source=github&utm_medium=link&utm_campaign=github-examples&utm_content=esp32)

# Update Firmware - OTA Example

This Arduino sketch allows you to connect your device to a WiFi network and update your firmware.

## Anedya OTA
Using Anedya, you can deploy and manage OTA Updates for thousands of devices. You can target specific groups of devices, roll out in a phased manner, and even automatically abort if something goes wrong. Overall, an OTA Update involves the following steps:
    - **You upload an asset:** A firmware, executable binary, video file, or even a zip file containing multiple files, which is called an Asset in terms of Anedya and can be deployed.
    - **Create a Deployment:** Create a deployment by specifying which devices to target, specify stages of deployment, and auto-abort criteria.
    - **Device fetches new deployments:** Device polls Anedya's MQTT or HTTP APIs at regular intervals (say every 3 Hours). Anedya will automatically provide information on applicable deployments.
    - **Device downloads the asset:** The device can then download the asset and notify Anedya that the update process has begun.
    - **Installation finishes**: The device performs installation actions.
    - **Device status update:** The device acknowledges Anedya about the success or failure of the update process.

## Set-Up Project in Anedya Dashboard

Following steps outline the overall steps to setup and OTA Deployment. You can read more about the steps [here](https://docs.anedya.io/ota/)

1. Create account and login
2. Create new project.
3. Create a node (e.g., for home- Room1 or study room).
4. Upload the Asset.
5. Create a Deploymnent.

 > [!TIP]
 > For more details, Visit anedya [documentation](https://docs.anedya.io?utm_source=github&utm_medium=link&utm_campaign=github-examples&utm_content=esp32)


## Hardware Set-Up

### Code Set-Up

1. Replace `<PHYSICAL-DEVICE-UUID>` with your 128-bit UUID of the physical device.
2. Replace `<CONNECTION-KEY>` with your connection key, which you can obtain from the node description.
3. Set up your WiFi credentials by replacing `SSID` and `PASSWORD` with your WiFi network's SSID and password.

> [!NOTE] 
> This example utilizes `Anedya Root CA 3 (ECC - 256)(Pem format)` for establishing a secure connection to the Anedya. If you opt to use an `Anedya Root CA 1 (RSA - 2048)` certificate instead, please ensure that you provide the necessary additional certificate details regarding the RSA certificate before initiating the connection.

## Dependencies

### HttpsOTAUpdate
The HttpsOTAUpdate.h library simplifies the process of updating firmware over-the-air using secure HTTPS connections. This library ensures that your firmware updates are delivered securely over the internet, allowing for easy and safe updates of your device.
**Note:** Make sure you have provided esp32 board url`https://dl.espressif.com/dl/package_esp32_index.json` in the preferences/additional boards manager URL's of your IDE.

### ArduinoJson

This repository contains the ArduinoJson library, which provides efficient JSON parsing and generation for Arduino and other embedded systems. It allows you to easily serialize and deserialize JSON data, making it ideal for IoT projects, data logging, and more.

1. Open the Arduino IDE.
2. Go to `Sketch > Include Library > Manage Libraries...`.
3. In the Library Manager, search for "ArduinoJson".
4. Click on the ArduinoJson entry in the list.
5. Click the "Install" button to install the library.
6. Once installed, you can include the library in your Arduino sketches by adding `#include <ArduinoJson.h>` at the top of your sketch.

### Timelib

The `timelib.h` library provides functionality for handling time-related operations in Arduino sketches. It allows you to work with time and date, enabling you to synchronize events, schedule tasks, and perform time-based calculations.

To include the `timelib.h` library:

1. Open the Arduino IDE.
2. Go to `Sketch > Include Library > Manage Libraries...`.
3. In the Library Manager, search for "Time".
4. Click on the ArduinoJson entry in the list(`Time by Michael Margolis`).
5. Click the "Install" button to install the library.
6. Once installed, you can include the library in your Arduino sketches by adding `#include <TimeLib.h>` at the top of your sketch.

> [!TIP]
> Looking for Python SDK? Visit [PyPi](https://pypi.org/project/anedya-dev-sdk/) or [Github Repository](https://github.com/anedyaio/anedya-dev-sdk-pyhton)

>[!TIP]
> For more information, visit [anedya.io](https://anedya.io/?utm_source=github&utm_medium=link&utm_campaign=github-examples&utm_content=esp32)