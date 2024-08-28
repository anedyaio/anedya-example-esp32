[<img src="https://img.shields.io/badge/Anedya-Documentation-blue?style=for-the-badge">](https://docs.anedya.io?utm_source=github&utm_medium=link&utm_campaign=github-examples&utm_content=esp32)

# Get-Stored-Device-Info - Example-value-store (mqtt)


This Arduino sketch allows you to get the  value from anedya.

> [!WARNING]
> This code is for hobbyists for learning purposes. Not recommended for production use!!

## Set-Up Project in Anedya Dashboard

Following steps outline the overall steps to setup a project. You can read more about the steps [here](https://docs.anedya.io/getting-started/quickstart/#create-a-new-project)

1. Create account and login
2. Create new project.
4. Create a node.

### Code Setup

- Fill the and device id and connection key
- `anedya_getNodeValue("<key>")`
fill:-
  - 1 Parameter- key.

- `anedya_getGlobalValue("<NAMESPACE>","<KEY>")`
fill:-
  - 1 Parameter- NAMESPACE
  - 2 Parameter- Key

 > [!TIP]
 > For more details, Visit anedya [documentation](https://docs.anedya.io?utm_source=github&utm_medium=link&utm_campaign=github-examples&utm_content=esp32)


## Usage

1. Upload this code to your device.
2. The device will connect to the WiFi network.
3. Open the Serial Monitor to view the value.


> [!TIP]
> Looking for Python SDK? Visit [PyPi](https://pypi.org/project/anedya-dev-sdk/) or [Github Repository](https://github.com/anedyaio/anedya-dev-sdk-pyhton)

>[!TIP]
> For more information, visit [anedya.io](https://anedya.io/?utm_source=github&utm_medium=link&utm_campaign=github-examples&utm_content=esp32)