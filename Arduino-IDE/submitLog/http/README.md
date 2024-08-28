[<img src="https://img.shields.io/badge/Anedya-Documentation-blue?style=for-the-badge">](https://docs.anedya.io?utm_source=github&utm_medium=link&utm_campaign=github-examples&utm_content=esp32)


# Room Monitoring - ESP32+DHT (Submit-Log) (http)



This Arduino sketch allows you to submit your device log to the Anedya.

> [!WARNING]
> This code is for hobbyists for learning purposes. Not recommended for production use!!

## Set-Up

> [!IMPORTANT]
> Prerequisite: Set up the dashboard and fill the connection key and physical device ID in the firmware. For detailed guidance, visit [here](https://github.com/anedyaio/anedya-example-nodemcu/blob/main/README.md) 

### Usages 

To submit the the log, use : 
```
anedya_submitLog("<REQUEST-ID>","<LOG-MESSAGE>");
```


> [!TIP]
> Looking for Python SDK? Visit [PyPi](https://pypi.org/project/anedya-dev-sdk/) or [Github Repository](https://github.com/anedyaio/anedya-dev-sdk-pyhton)

>[!TIP]
> For more information, visit [anedya.io](https://anedya.io/?utm_source=github&utm_medium=link&utm_campaign=github-examples&utm_content=esp32)
 