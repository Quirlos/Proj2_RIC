#include <WiFi.h>
//ArduinoWebsockets library http://bit.ly/3LeAGVl
#include <ArduinoWebsockets.h>
// JSON (de)serialisation library http://bit.ly/3ZHI5Rs
#include <ArduinoJson.h>
#include <Adafruit_Sensor.h>  //BIBLIOTECAS DO SENSOR BME 280
#include <Adafruit_BME280.h> 
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>

#define DEBUG true

#define ONBOARD_BUTTON 0

#define pmar 1013.25
// Wi-Fi credentials for the network the ESP will connect to
const char* ssid = "Chiqueiro";
const char* password = "tentaadivinhar";

AsyncWebServer server(80);

// Websocket URL
const char* websockets_server_host = "192.168.68.104";  //Enter server adress
const uint16_t websockets_server_port = 1880;          // Enter server port

Adafruit_BME280 bme;

using namespace websockets;
WebsocketsClient client;

// JSON Deserializer
DynamicJsonDocument doc(1024);

// Other global variables.
bool connectionStatus = false;
float temperatura;
float humidade;
float pressao;
float altitude;
float botao;
double lastSentMillis = 0;
int DATA_POST_INTERVAL_MS = 2000;  // In milliseconds
int pos=30;
//int aux=0;
//Servo myservo;
//int pservo=13;




void setup() {



  //myservo.attach(pservo, 0, 180);
  pinMode(ONBOARD_BUTTON, INPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  // Initialize the serial port.
  Serial.begin(115200);
  //myservo.write(pos);

  bool status;
  status = bme.begin(0x76);  
  if (!status) {
    Serial.println("ERRO no BME");
    while (1);
  }

  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print("-");
  }
#ifdef DEBUG
  Serial.println("WiFi connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
#endif
  //   Cnnect to WS

  bool connected = client.connect(websockets_server_host, websockets_server_port, "/test/esp32");
  if (connected) {
    Serial.println("WS Connected!");
    connectionStatus = true;
  } else {
    Serial.println("WS Connection failed    !");
  }
  // Callbacks for message and events
  client.onMessage(messageCallback);
  client.onEvent(eventCallback);

  // Ping to the remote WS server. Expect a pong
  client.ping();

  AsyncElegantOTA.begin(&server);    // Start ElegantOTA
  server.begin();

  Serial.print("IP da placa: ");
  Serial.println(WiFi.localIP());
}

void loop() {

  if (client.available()) {
    client.poll();
  }
  // Send every 30 seconds
  if ((millis() - lastSentMillis) > DATA_POST_INTERVAL_MS) {
    temperatura = bme.readTemperature();
    humidade = bme.readHumidity();
    pressao = bme.readPressure()/100;
    altitude = bme.readAltitude(pmar);
    botao = digitalRead(ONBOARD_BUTTON);
    String data = String(temperatura, 1) + "," + String(humidade, 1) + "," + String(pressao, 1) + "," + String(altitude, 1) + "," + String(botao);
    client.send(data);
    lastSentMillis = millis();
  }

  // Check if the connection is closed. If so reconnect
  if (!connectionStatus) {
    Serial.println("No Connection");
    bool reconnectStatus = client.connect(websockets_server_host, websockets_server_port, "/test/esp32");
    if (reconnectStatus) {
      Serial.println("WS Reconnected!");
      connectionStatus = true;
    } else {
      // 1 Min Wait
      delay(60000);
    }
  }
  // A delay
  delay(1000);
}

void messageCallback(WebsocketsMessage message) {
#ifdef DEBUG
  Serial.print("Message Received: ");
  Serial.println(message.data());
#endif
  deserializeJson(doc, message.data());
  if (doc["status"] == 1) {
    digitalWrite(LED_BUILTIN, HIGH);
  } else {
    digitalWrite(LED_BUILTIN, LOW);
  }
  
  pos=doc["angulo"];
  if (pos!=0){
  Serial.print("pos:");
  Serial.println(pos);
  }
}

void eventCallback(WebsocketsEvent event, String data) {
  if (event == WebsocketsEvent::ConnectionOpened) {
    Serial.println("Connnection Opened");
  } else if (event == WebsocketsEvent::ConnectionClosed) {
    Serial.println("Connnection Closed");
    connectionStatus = false;
  } else if (event == WebsocketsEvent::GotPing) {
    Serial.println("Pinged, responding with PONG!");
    client.pong();
  } else if (event == WebsocketsEvent::GotPong) {
    Serial.println("Got a Pong!");
  }
}
