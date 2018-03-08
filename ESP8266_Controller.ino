#define DEBUG

#include "ESP8266_Controller.h"

void setup(void)
{
  pinMode(lamp0.pinNb, OUTPUT);
  pinMode(lamp1.pinNb, OUTPUT);

  DEBUG_INIT_LED(led, OUTPUT);
  DEBUG_LED(lamp0.pinNb, HIGH);
  DEBUG_LED(lamp1.pinNb, HIGH);
  DEBUG_LED(led, LOW);
  
#ifdef DEBUG
  Serial.begin(115200);
#endif
  DEBUGGING_L("");

  WiFiConnect();
  MDNSConnect();
  MqttConnect();
  WebSocketConnect();
  InitHandleHTTP();
  HTTPUpdateConnect();
  InitIR();

  DEBUG_LED(lamp0.pinNb, lamp0.state);
  DEBUG_LED(lamp1.pinNb, lamp0.state);
}

void loop(void)
{

  if (WiFi.status() != WL_CONNECTED)
  {
    WiFiConnect();
    MqttConnect();
    WebSocketConnect();
    MDNSConnect();
  }
  else
  {
    if (!client.connected())
    {
      MqttReconnect();
    }
    client.loop();
    webSocket.loop();
    httpServer.handleClient();
    //MDNS.update();
  }
}

