# [<img src="https://img.shields.io/badge/Anedya-Documentation-blue?style=for-the-badge">](https://docs.anedya.io?utm_source=github&utm_medium=link&utm_campaign=github-examples&utm_content=esp32_quectel)

# Submit Data via HTTP with ESP32 and Quectel

This Arduino sketch demonstrates how to submit data to Anedya using an ESP32 and a Quectel EC200U module.

---

## Getting Started with Anedya

Follow these steps to set up a project in the Anedya Dashboard. Detailed instructions are available in the [Anedya Quickstart Guide](https://docs.anedya.io/getting-started/quickstart/#create-a-new-project):

1. **Create an Account**: Sign up and log in to Anedya.
2. **Create a New Project**: Set up your project on the dashboard.
3. **Add Variables**: Define the variables for your project, such as `temperature` and `humidity`.
4. **Add a Node**: Create a node (e.g., "Room1" or "Study Room") to represent your physical device.

> **Note**: Ensure that the variable identifiers are correctly filled out, as they are essential for data mapping.

For additional help, consult the [Anedya Documentation](https://docs.anedya.io?utm_source=github&utm_medium=link&utm_campaign=github-examples&utm_content=esp32_quectel).

---
  
## Hardware Setup

1. **Power the EC200U Module**: Connect the EC200U module to a 5V 2A power supply.
2. **Serial Communication**: Connect the ESP32 to the EC200U module via `Serial1`. For the ESP32-S3 Dev Board used in testing, this corresponds to GPIO 16 (TX) and GPIO 17 (RX).

---

## Code Configuration

1. **Insert Your Connection Key**:
   Replace `<CONNECTION-KEY>` with the connection key obtained from your node description in the Anedya Dashboard.

2. **Provide the Physical Device UUID**:
   Replace `<PHYSICAL-DEVICE-UUID>` with your device’s unique 128-bit UUID.

3. **Upload the Sketch**:
   Compile and upload the sketch to your ESP32 using the Arduino IDE or your preferred development environment.

---

## Dependency

### Timelib

The `timelib.h` library provides functionality for handling time-related operations in Arduino sketches. It allows you to work with time and date, enabling you to synchronize events, schedule tasks, and perform time-based calculations.

To include the `timelib.h` library:

1. Open the Arduino IDE.
2. Go to `Sketch > Include Library > Manage Libraries...`.
3. In the Library Manager, search for "Time".
4. Click on the Time  entry in the list(`Time by Michael Margolis`).
5. Click the "Install" button to install the library.
6. Once installed, you can include the library in your Arduino sketches by adding `#include <TimeLib.h>` at the top of your sketch.

---

## Additional Resources

- Looking for a Python SDK? Explore the [Anedya Python SDK on PyPI](https://pypi.org/project/anedya-dev-sdk/) or visit the [GitHub Repository](https://github.com/anedyaio/anedya-dev-sdk-python).

- For more information, visit the [Anedya Website](https://anedya.io/?utm_source=github&utm_medium=link&utm_campaign=github-examples&utm_content=esp32_quectel).

---

By following this guide, you can seamlessly submit sensor data to Anedya and integrate IoT functionalities into your projects.
