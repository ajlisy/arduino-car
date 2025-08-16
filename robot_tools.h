#ifndef ROBOT_TOOLS_H
#define ROBOT_TOOLS_H

#include <WiFi.h>
#include <HTTPClient.h>
#include <NewPing.h>
#include <string>
#include <PubSubClient.h>

// Pin definitions for motors
#define IN1 16
#define IN2 17
#define IN3 18
#define IN4 19

// Pin definitions for ultrasonic sensor
#define TRIGGER_PIN 22
#define ECHO_PIN 23
#define MAX_DISTANCE 400

// Global sonar object
extern NewPing sonar;

// Global MQTT client (optional, for logging)
extern PubSubClient client;

// Tool structure definition
struct Tool {
  String name;
  String description;
  String (*execute)(String params);
};

// Function declarations
String getSonarDistance(String params);
String moveCar(String params);
String testSonar(String params);
String getEnvironmentInfo(String params);
String sendMqttMessage(String params);
String listTools();
String executeTool(String toolName, String params = "");
int getToolCount();
Tool getToolByIndex(int index);
void initRobotTools();

// Motor control function declarations
void stopWheels();
void goForward(int milliseconds);
void goBackward(int milliseconds);
void turnLeft(int milliseconds);
void turnRight(int milliseconds);
void turnLeftDegrees(int degrees);
void turnRightDegrees(int degrees);

#endif // ROBOT_TOOLS_H 