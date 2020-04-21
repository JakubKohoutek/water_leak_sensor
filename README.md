# Water Leak Sensor

This repository contains circuit diagram and code for low power water leak sensor using ESP-8266.

## Description

* [ ] TODO

## Credentials and Configuration

This snippet uses a configuration file `config.h` that stores credentials and confidential information
used within the code.

To create the file, run the following script (replace `***` with the real values):

```bash
cat << EOF >> config.h
#ifndef CONFIG_H
#define CONFIG_H

#define WIFI_NAME "***"
#define WIFI_PSWD "***"

#define HOST      "https://***"
#define API_TOKEN "***"
#define PHONE_NUM "***"
#define EMAIL     "***"

#endif
EOF
```

## Circuit Diagram

* [ ] TODO

## Compilation and Upload Configuration

* Board: Generic ESP8266 Module
* Upload speed: 115200
* Flash size: 4M (1M SPIFFS)

## Adjustments To Minimize Power Consumption

I was able to lower the power consumption of ESP-8266 board by using the deep sleep mode.
