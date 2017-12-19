#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <IRremoteESP8266.h>
#include <ESP8266HTTPUpdateServer.h>
#include <WebSocketsServer.h>
#include <PubSubClient.h>

#ifdef DEBUG
#define DEBUGGING(...) Serial.println( __VA_ARGS__ )
#define DEBUGGING_L(...) Serial.print( __VA_ARGS__ )
#define DEBUG_INIT_LED(...) pinMode( __VA_ARGS__ )
#define DEBUG_LED(...) digitalWrite( __VA_ARGS__ )
#endif

// Globals
const char *ssid = "WiFi-2.4-1538";
const char *password = "colloc69";
const char *host = "esp8266";
const char *mqtt_server = "test.mosquitto.org";

String html = 
"<html>\
  <head>\
    <meta http-equiv='refresh' content='5'/>\
    <title>ESP8266 Demo</title>\
    <style>\
      body { background-color: #993333; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }\
    </style>\
  </head>\
  <body>\
    <h1>Hello from ESP8266!</h1>\
    <p>Uptime: %%UPTIME%%</p>\
    <p>LED 0 : <a href='led0?cmd=turn_on'><button name='led0_ON'><strong>ON</strong></button></a>\
    <a href='led0?cmd=turn_off'><button name='led0_OFF'><strong>OFF</strong></button></a>\
    <a href='led0?cmd=toggle'><button name='led0_toggle'><strong>TOGGLE</strong></button></a></p>\
    <p>LED 1 : <a href='led1?cmd=turn_on'><button name='led1_ON'><strong>ON</strong></button></a>\
    <a href='led1?cmd=turn_off'><button name='led1_OFF'><strong>OFF</strong></button></a>\
    <a href='led1?cmd=toggle'><button name='led1_toggle'><strong>TOGGLE</strong></button></a></p>\
    <p>LG TV : <a href='tv?cmd=switch_on'><button name='lg_tv_power'><strong>ON/OFF</strong></button></a></p>\
    <br />\
    <p><a href='update'><button name='update'>UPDATE</button></a></p>\
  </body>\
</html>";

// Pin Number assignation
struct lamp
{
  int pinNb;
  int state;
  String strName; 
};

struct lamp lamp0{12, LOW, "lamp0"}; // Pin Number and state initializtion for the mood lamp
struct lamp lamp1{14, LOW, "lamp1"}; // Pin Number and state initializtion for the bedside lamp
const int led_ir = 4; // Pin number for IR LED
#ifdef DEBUG
const int led = 13; // Led that indicates the server request
#endif

WebSocketsServer webSocket = WebSocketsServer(81);
ESP8266WebServer httpServer(80);
ESP8266HTTPUpdateServer httpUpdater;
IRsend irsend(led_ir); //an IR led is connected to GPIO pin #4
WiFiClient espClient;
PubSubClient client(espClient);

// IR initialization
void InitIR(void)
{
  irsend.begin();
}

// WiFi Connection
void WiFiConnect(void)
{
  WiFi.begin(ssid, password);

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(100);
    DEBUGGING_L(".");
  }

  DEBUGGING("");
  DEBUGGING_L("Connected to ");
  DEBUGGING(ssid);
  DEBUGGING_L("IP address: ");
  DEBUGGING(WiFi.localIP());
}

// MDNS Initialization
void MDNSConnect()
{
  if (MDNS.begin(host, WiFi.localIP())) // http://esp8266.local/ in stead of IP adresse
  {
    DEBUGGING("MDNS responder started : http://" + String(host) + ".local");
  }
  else
  {
    DEBUGGING("Error setting up MDNS responder!");
  }
  
  // Add service to MDNS-SD
  MDNS.addService("ws", "tcp", 81);
  MDNS.addService("http", "tcp", 80);
}

//////////////////////////////////////////////////////////
//                    Commands functions                //
//////////////////////////////////////////////////////////
// Functions that execute the command reveived

// Lamp commands
bool CmdLamp(String Cmd, lamp *Lampe)
{
  bool cmd_executed = false;
  
  if (Cmd == "turn_on" || Cmd == "power_on" || Cmd == "switch_on")
  {
    DEBUGGING(Lampe->strName + " - On");
    digitalWrite(Lampe->pinNb, LOW); // Led on when GPIO is LOW
    Lampe->state = LOW;
    
    cmd_executed = true;
  }
  else if (Cmd == "turn_off" || Cmd == "power_off" || Cmd == "switch_off")
  {
    DEBUGGING(Lampe->strName + " - Off");
    digitalWrite(Lampe->pinNb, HIGH); // Led off when GPIO is HIGH
    Lampe->state = HIGH;

    cmd_executed = true;
  }
  else if (Cmd == "toggle")
  {
    Lampe->state = !Lampe->state;
    digitalWrite(Lampe->pinNb, Lampe->state);
    DEBUGGING((Lampe->state == HIGH) ? Lampe->strName + " - Off" : Lampe->strName + " - On");

    cmd_executed = true;
  }
  else
  {
    cmd_executed = false;
  }

  return cmd_executed;
}

// TV commands
bool CmdTV(String Cmd)
{
  bool cmd_executed = false;
  
  if (Cmd == "switch_off" || Cmd == "switch_on")
  {
    DEBUGGING("LG TV - On/Off");
    irsend.sendNEC(0x20DF10EF, 32); // Send ON/OFF TV
    delay(100);

    cmd_executed = true;
  }
  else
  {
    cmd_executed = false;
  }

  return cmd_executed;
}

//////////////////////////////////////////////////////////
//                  mqtt functions                      //
//////////////////////////////////////////////////////////

// mqtt reconnect
void MqttReconnect(void)
{
  // Loop until we're reconnected
  //while (!client.connected())
  //{
    DEBUGGING("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP8266_Client"))
    {
      DEBUGGING("connected");
      // ... and subscribe to topic
      client.subscribe("/mycroft/led0");
      client.subscribe("/mycroft/led1");
      client.subscribe("/mycroft/tv");
    }
    else
    {
      DEBUGGING_L("failed, rc=");
      DEBUGGING_L(client.state());
      DEBUGGING(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  //}
}

// mqtt callback
void MqttCallback(char* topic_p, byte* payload_p, unsigned int length_p)
{
  String top = String((char *) &topic_p[0]);
  String cmd = String((char *) &payload_p[0]);
  String mdl;

  cmd.remove(length_p);

  if (top.startsWith("/mycroft/"))
  {
    mdl = top.substring(top.lastIndexOf('/')+1, top.length());

    if(mdl.equals("led0") || mdl.equals("mood"))
    {
      CmdLamp(cmd, &lamp0); 
    }

    if(mdl.equals("led1") || mdl.equals("bedside") || mdl.equals("bed_side"))
    {
      CmdLamp(cmd, &lamp1);
    }
    
    if(mdl.equals("tv"))
    {
      CmdTV(cmd);
    } 
  }
}

// mqtt connection
void MqttConnect(void)
{
  client.setServer(mqtt_server, 1883);
  client.setCallback(MqttCallback);
}

//////////////////////////////////////////////////////////
//                  Websocket functions                 //
//////////////////////////////////////////////////////////

// WebSOcket Events
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length)
{
  switch (type)
  {
    case WStype_DISCONNECTED:
      break;
    case WStype_CONNECTED:
      {
        IPAddress ip = webSocket.remoteIP(num);
      }
      break;
    case WStype_TEXT:
      //TODO : fill the case !
      {
        String txt = String((char *) &payload[0]);
        
        if(txt.startsWith("led0") || txt.startsWith("mood"))
        {
          String lamp0_cmd = txt.substring(txt.indexOf("-")+1, txt.length());
          CmdLamp(lamp0_cmd, &lamp0); 
        }

        if(txt.startsWith("led1") || txt.startsWith("bedside") || txt.startsWith("bed_side"))
        {
          String lamp1_cmd = txt.substring(txt.indexOf("-")+1, txt.length());
          CmdLamp(lamp1_cmd, &lamp1);
        }
        
        if(txt.startsWith("tv"))
        {
          String tv_cmd = txt.substring(txt.indexOf("-")+1, txt.length());
          CmdTV(tv_cmd);
        }
      }
      break;
    case WStype_BIN:
      {
        hexdump(payload, length);
        // echo data back to browser
        webSocket.sendBIN(num, payload, length);
      }
      break;
  }
}

// WebSocket Connection
void WebSocketConnect(void)
{
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
}

//////////////////////////////////////////////////////////
//                     HTTP functions                   //
//////////////////////////////////////////////////////////

// HTTP request not found
void HandleNotFound(void)
{
  DEBUG_LED(led, HIGH);
  
  String message = "Not Found\n\n";
  message += "URI: ";
  message += httpServer.uri();
  message += "\nMethod: ";
  message += ( httpServer.method() == HTTP_GET ) ? "GET" : "POST";
  message += "\nArguments: ";
  message += httpServer.args();
  message += "\n";

  for ( uint8_t i = 0; i < httpServer.args(); i++ )
  {
    message += " " + httpServer.argName(i) + ": " + httpServer.arg(i) + "\n";
  }

  httpServer.send(404, "text/plain", message);

  DEBUG_LED(led, LOW);
}

// Function that handles the HTTP root
void HandleRoot(void)
{
  DEBUG_LED(led, HIGH);
 
  char temp[9];
  int sec = millis() / 1000;
  int min = sec / 60;
  int hr = min / 60;
  String to_send = html;

  snprintf(temp, sizeof(temp), "%02d:%02d:%02d", hr, min % 60, sec % 60);

  to_send.replace("%%UPTIME%%", temp);
  
  httpServer.send(200, "text/html", to_send);
  
  DEBUG_LED(led, LOW);
}

// Fonction that handles the GPIO
void HandleGPIO(lamp *Lampe)
{
  String cmd = httpServer.arg("cmd");
  
  CmdLamp(cmd, Lampe);

  HandleRoot();

  //httpServer.send(200, "text/html", html);
}


// Function that handles the LG TV commands
void HandleTV(void)
{
  String cmd = httpServer.arg("cmd");
  static String prev_cmd = "switch_off";

  if (cmd != prev_cmd)
  {
    if (CmdTV(cmd))
    {
      html.replace(cmd, prev_cmd);
      prev_cmd = cmd;
      
      HandleRoot();
    }
    else
    {
      HandleNotFound();
    }
  }
  else
  {
    HandleRoot();
  }
}

// Initialize the handler function (callback) for http requests
void InitHandleHTTP(void)
{
  httpServer.on("/", HandleRoot);
  httpServer.on("/led0", []() {HandleGPIO(&lamp0);});
  httpServer.on("/led1", []() {HandleGPIO(&lamp1);});
  httpServer.on("/tv", HandleTV);
  //httpServer.on( "/inline", []() {httpServer.send ( 200, "text/plain", "this works as well" );} );
  httpServer.onNotFound(HandleNotFound);
  httpServer.begin();
  DEBUGGING("HTTP server started.");
}

// HTTP updater connection
void HTTPUpdateConnect()
{
  httpUpdater.setup(&httpServer);
  httpServer.begin();
  DEBUGGING_L("HTTPUpdateServer ready! Open http://");
  DEBUGGING_L(host);
  DEBUGGING(".local/update in your browser\n");
}

