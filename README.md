## Arduino Code for ESP8266 (Mycroft Skill)
Arduino code for ESP8266 to communicate with Mycroft through the ESP8266 Skill.

Communication protocol implemented :
* http GET request
* Websocket
* MQTT

## Requirement

Arduino libreries needed :

```
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <ESP8266HTTPUpdateServer.h>
#include <WebSocketsServer.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
```

To use JSON message, I had to change from

```
#define MQTT_MAX_PACKET_SIZE 128
```

to


```
#define MQTT_MAX_PACKET_SIZE 512
```

in `PubSubClient.h`.

## TODO

* Implement SPIFFS to get access to the flash memory and used to store HTML, CSS and JavaScript file.
* Implement ArduinoOTA.
* Use Mycroft Skill to set or reset the MQTT server or MQTT topic.
