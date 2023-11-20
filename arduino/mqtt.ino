#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <ArduinoJson.h>

const char* ssid = "IR_Lab";
const char* password = "ccsadmin";
const char* mqtt_server = "192.168.0.20";
const char* mqtt_topic = "DHT11Data";
const int ledPin = D6;
float temp;
float hum;
DHT dht11(D4, DHT11);

WiFiClient espClient;
PubSubClient client(espClient);

void ReadDHT11() {
  temp = dht11.readTemperature();
  hum = dht11.readHumidity();
}

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());
  Serial.println("");
  Serial.println("WiFi Connected");
  Serial.println("IP address");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      client.subscribe("led");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  if (String(topic) == "led") {
    if (message == "on") {
      digitalWrite(ledPin, HIGH);
    } else if (message == "off") {
      digitalWrite(ledPin, LOW);
    }
  }
}

void setup() {
  pinMode(ledPin, OUTPUT);
  Serial.begin(115200);
  dht11.begin();
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  digitalWrite(ledPin, LOW);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  ReadDHT11();

  IPAddress localIP = WiFi.localIP();
  StaticJsonDocument<200> jsonDocument;
  jsonDocument["humidity"] = hum;
  jsonDocument["temperature"] = temp;
  jsonDocument["ip"] = localIP.toString();

  String jsonStr;
  serializeJson(jsonDocument, jsonStr);

  client.publish(mqtt_topic, jsonStr.c_str());

  delay(5000);
}
