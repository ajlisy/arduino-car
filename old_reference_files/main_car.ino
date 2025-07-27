//Notes
// This is a version that works and is on the car as of
// 10/29/2023. 
// This version has some additional code to connect to an MQTT server
// primarily for testing the distance measurement function.
// It is currently disabled (ATTEMPT_MQTT = 0). Would need to recreate
// MQTT server to make this work. 

#include <WiFi.h>
#include <HTTPClient.h>
#include <string>
#include <PubSubClient.h>
#include <stdio.h>
#include <NewPing.h>


//Pins
#define IN1 16
#define IN2 17
#define IN3 18
#define IN4 19
#define TRIGGER_PIN 22
#define ECHO_PIN 23
#define MAX_DISTANCE 400  // Maximum distance we want to measure (in centimeters).

#define ATTEMPT_MQTT 0

const char* ssid = "gavroche";
const char* password = "YOUR_WIFI_PASSWORD_HERE";
String instructionsURL = "https://pastebin.com/raw/ydRLi8ad";

const char* mqtt_server = "192.168.4.200";
const int mqtt_port = 1883;

WiFiClient espClient;
PubSubClient client(espClient);

NewPing sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE);

unsigned long lastTime = 0;
unsigned long timerDelay = 5000;


void setup() {
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  Serial.begin(115200);
  WiFi.begin(ssid, password);

  //Loop until we've connected to Wifi
  Serial.println("Connecting to Wifi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());

  //Loop until we're connected to MQTT
  if (ATTEMPT_MQTT) {
    client.setServer(mqtt_server, mqtt_port);
    if (!client.connected()) {
      reconnect();
    }
    client.loop();

     //Print distance
    int distance = sonar.ping_cm();
    char message[30];
    sprintf(message, "distance %d", distance);
    client.publish("car", message);
  }



  //Download instructions
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    Serial.println("Instructions URL: ");
    Serial.print(instructionsURL);
    http.begin(instructionsURL.c_str());
    int httpResponseCode = http.GET();

    //Process Response
    if (httpResponseCode > 0) {
      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);
      String payload = http.getString();
      http.end();
      Serial.println("Payload received ------------->>");
      Serial.println(payload);
      Serial.println("<< -------------");
      tokenize(payload);
    } else {
      Serial.print("Error code: ");
      Serial.println(httpResponseCode);
      http.end();
    }


  } else {
    Serial.println("WiFi Disconnected");
  }

}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESP32Client")) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void loop() {
  if (ATTEMPT_MQTT) client.loop();
  delay(5000);
}

void tokenize(String in) {
  Serial.println("Tokenizing...");
  String first;
  String second;
  int last, space, newline;
  last = 0;

  //Initial values
  space = in.indexOf(' ');
  newline = in.indexOf('\n');
  while (space != -1) {
    Serial.println("Input string: >>" + in + "<<");
    Serial.println("space = " + String(space) + "; newline=" + String(newline) + "; last=" + String(last));
    first = in.substring(last, space);
    second = in.substring(space + 1, newline);
    Serial.println("Tokens: >>" + first + "<< / >>" + second + "<<");
    parse_and_route(first, second.toInt());
    in = in.substring(newline + 1, in.length());
    space = in.indexOf(' ');
    newline = in.indexOf('\n');
  }
}

void parse_and_route(String command, int param) {
  if (command == "go_forward") {
    go_forward(param);
  } else if (command == "go_backward") {
    go_backward(param);
  } else if (command == "turn_degrees_left") {
    turn_degrees_left(param);
  } else if (command == "turn_degrees_right") {
    turn_degrees_right(param);
  } else if (command == "wait") {
    wait(param);
  } else if (command == "//") {
  } else {
    Serial.println("Error on parse_and_route. Received command: >>" + command + "<<. Received param: " + String(param));
  }
}

void wait(int milliseconds) {
  delay(milliseconds);
}

void go_forward(int milliseconds) {
  char message[30];
  sprintf(message, "forward %d", milliseconds);
  client.publish("car", message);
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  delay(milliseconds);

  stop_wheels();
}

void go_backward(int milliseconds) {
  char message[30];
  sprintf(message, "backward %d", milliseconds);
  client.publish("car", message);
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
  delay(milliseconds);

  stop_wheels();
}

void turn_90_right() {
  client.publish("car", "right 90");
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  delay(600);

  stop_wheels();
}

void turn_90_left() {
  client.publish("car", "left 90");
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
  delay(600);

  stop_wheels();
}

void turn_degrees_left(int degrees) {
  char message[30];
  sprintf(message, "left degrees %d", degrees);
  client.publish("car", message);
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
  delay(600 * (degrees / 90));

  stop_wheels();
}

void turn_degrees_right(int degrees) {
  char message[30];
  sprintf(message, "right degrees %d", degrees);
  client.publish("car", message);
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  delay(600 * (degrees / 90));

  stop_wheels();
}

void stop_wheels() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
}
