[<img src="https://img.shields.io/badge/Anedya-Documentation-blue?style=for-the-badge">](https://docs.anedya.io?utm_source=github&utm_medium=link&utm_campaign=github-examples&utm_content=esp32)

# Store-Device-Info - Example-value-store (mqtt)


This Arduino sketch allows you to set the your value to the anedya dashboard.

> [!WARNING]
> This code is for hobbyists for learning purposes. Not recommended for production use!!

## Set-Up Project in Anedya Dashboard

Following steps outline the overall steps to setup a project. You can read more about the steps [here](https://docs.anedya.io/getting-started/quickstart/#create-a-new-project)

1. Create account and login
2. Create new project.
4. Create a node.

### Code Setup

- Fill the and device id and connection key
- `anedya_valueStrValue("<key>", "<ValuePayload")` : Use to set the string value.
- `anedya_valueFloatValue("<key>", "<ValuePayload")` : Use to set the flaot value.

> [!NOTE] 
> This example utilizes `Anedya Root CA 3 (ECC - 256)(Pem format)` for establishing a secure connection to the Anedya. If you opt to use an `Anedya Root CA 1 (RSA - 2048)` certificate instead, please ensure that you provide the necessary additional certificate details regarding the RSA certificate before initiating the connection.

 > [!TIP]
 > For more details, Visit anedya [documentation](https://docs.anedya.io?utm_source=github&utm_medium=link&utm_campaign=github-examples&utm_content=esp32)


## Dependencies

### ArduinoJson

This repository contains the ArduinoJson library, which provides efficient JSON parsing and generation for Arduino and other embedded systems. It allows you to easily serialize and deserialize JSON data, making it ideal for IoT projects, data logging, and more.

1. Open the Arduino IDE.
2. Go to `Sketch > Include Library > Manage Libraries...`.
3. In the Library Manager, search for "ArduinoJson".
4. Click on the ArduinoJson entry in the list.
5. Click the "Install" button to install the library.
6. Once installed, you can include the library in your Arduino sketches by adding `#include <ArduinoJson.h>` at the top of your sketch.

### PubSubClient
The PubSubClient library provides a client for doing simple publish/subscribe messaging with a server that supports MQTT. It allows you to easily publish and subscribe to topics, making it ideal for IoT projects, data logging, and more.

1. Open the Arduino IDE.
2. Go to `Sketch > Include Library > Manage Libraries...`.
3. In the Library Manager, search for "PubSubClient".
4. Click on the PubSubClient by Nick O'Leary entry in the list.
5. Click the "Install" button to install the library.
6. Once installed, you can include the library in your Arduino sketches by adding `#include <PubSubClient.h>` at the top of your sketch.


### Timelib

The `timelib.h` library provides functionality for handling time-related operations in Arduino sketches. It allows you to work with time and date, enabling you to synchronize events, schedule tasks, and perform time-based calculations.

To include the `timelib.h` library:

1. Open the Arduino IDE.
2. Go to `Sketch > Include Library > Manage Libraries...`.
3. In the Library Manager, search for "Time".
4. Click on the ArduinoJson entry in the list(`Time by Michael Margolis`).
5. Click the "Install" button to install the library.
6. Once installed, you can include the library in your Arduino sketches by adding `#include <TimeLib.h>` at the top of your sketch.


## Usage

1. Upload this code to your device.
2. The device will connect to the WiFi network.
3. Open the Serial Monitor to view the device's output.


> [!TIP]
> Looking for Python SDK? Visit [PyPi](https://pypi.org/project/anedya-dev-sdk/) or [Github Repository](https://github.com/anedyaio/anedya-dev-sdk-python)

>[!TIP]
> For more information, visit [anedya.io](https://anedya.io/?utm_source=github&utm_medium=link&utm_campaign=github-examples&utm_content=esp32)