#define DEBUG

#include "ESP8266_Controller.h"

void setup(void)
{
  pinMode(mdl0.pinNb, OUTPUT);
  pinMode(mdl1.pinNb, OUTPUT);

  DEBUG_INIT_LED(led, OUTPUT);
  DEBUG_LED(mdl0.pinNb, HIGH);
  DEBUG_LED(mdl1.pinNb, HIGH);
  DEBUG_LED(led, LOW);
  
#ifdef DEBUG
  Serial.begin(115200);
#endif
  DEBUGGING_L("");

  WiFiConnect();
  MDNSSetup();
  MqttSetup();
  WebSocketSetup();
  InitHandleHTTP();
  HTTPUpdateConnect();
  InitIR();

  DEBUG_LED(mdl0.pinNb, mdl0.state);
  DEBUG_LED(mdl1.pinNb, mdl1.state);
}

void loop(void)
{

  if (WiFi.status() != WL_CONNECTED)
  {
    WiFiConnect();
    MDNSSetup();
    MqttSetup();
    WebSocketSetup();
  }
  else
  {
    if (!client.connected())
    {
      MqttConnect();
    }
    client.loop();
    webSocket.loop();
    httpServer.handleClient();
    //MDNS.update();
  }
}

