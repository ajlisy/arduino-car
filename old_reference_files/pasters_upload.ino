#include "esp_camera.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include "camera_pins.h"

// Replace with your WiFi credentials
const char* ssid     = "gavroche";
const char* password = "***REMOVED***";

// Relay endpoint URL to send the image URL
const char* relay_url = "https://run.relay.app/api/v1/playbook/cm78lelqy0me00nm05s3g6a5l/trigger/EWDCMddZ6-NfCpAKGbxwug";

// Initialize the camera in JPEG mode (must support JPEG)
void initCamera() {
  camera_config_t config;
  config.ledc_channel    = LEDC_CHANNEL_0;
  config.ledc_timer      = LEDC_TIMER_0;
  config.pin_d0          = Y2_GPIO_NUM;
  config.pin_d1          = Y3_GPIO_NUM;
  config.pin_d2          = Y4_GPIO_NUM;
  config.pin_d3          = Y5_GPIO_NUM;
  config.pin_d4          = Y6_GPIO_NUM;
  config.pin_d5          = Y7_GPIO_NUM;
  config.pin_d6          = Y8_GPIO_NUM;
  config.pin_d7          = Y9_GPIO_NUM;
  config.pin_xclk        = XCLK_GPIO_NUM;
  config.pin_pclk        = PCLK_GPIO_NUM;
  config.pin_vsync       = VSYNC_GPIO_NUM;
  config.pin_href        = HREF_GPIO_NUM;
  config.pin_sccb_sda    = SIOD_GPIO_NUM;
  config.pin_sccb_scl    = SIOC_GPIO_NUM;
  config.pin_pwdn        = PWDN_GPIO_NUM;
  config.pin_reset       = RESET_GPIO_NUM;
  
  config.xclk_freq_hz    = 20000000;
  config.pixel_format    = PIXFORMAT_JPEG;  // Ensure JPEG mode
  
  if(psramFound()){
    config.frame_size    = FRAMESIZE_VGA;
    config.jpeg_quality  = 12;
    config.fb_count      = 2;
  } else {
    config.frame_size    = FRAMESIZE_QVGA;
    config.jpeg_quality  = 20;
    config.fb_count      = 1;
  }
  
  esp_err_t err = esp_camera_init(&config);
  if(err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    while(1) { delay(1000); }
  }
}

// Connect to WiFi network
void connectWiFi() {
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

// Upload the captured image to paste.rs and return the resulting URL
String uploadPhotoToPasteRS(camera_fb_t *fb) {
  String pasteUrl = "";
  WiFiClientSecure client;
  client.setInsecure();  // Disable certificate validation for simplicity
  
  HTTPClient http;
  if(http.begin(client, "https://paste.rs")) {
    http.addHeader("Content-Type", "image/jpeg");
    int httpResponseCode = http.POST(fb->buf, fb->len);
    if(httpResponseCode > 0) {
      pasteUrl = http.getString();
      Serial.print("PasteRS URL: ");
      Serial.println(pasteUrl);
    } else {
      Serial.print("Error on sending POST to paste.rs: ");
      Serial.println(httpResponseCode);
    }
    http.end();
  } else {
    Serial.println("Unable to connect to paste.rs");
  }
  return pasteUrl;
}

// Send the image URL to the relay endpoint via POST as a JSON payload
void sendImageURLToRelay(String imageUrl) {
  // Build a JSON payload: {"imageUrl": "https://paste.rs/your_id"}
  String payload = "{";
  payload += "\"imageUrl\":\"" + imageUrl + "\"";
  payload += "}";
  
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  if(http.begin(client, relay_url)) {
    http.addHeader("Content-Type", "application/json");
    int httpResponseCode = http.POST(payload);
    if(httpResponseCode > 0) {
      String response = http.getString();
      Serial.print("Relay response: ");
      Serial.println(response);
    } else {
      Serial.print("Error on sending POST to relay: ");
      Serial.println(httpResponseCode);
    }
    http.end();
  } else {
    Serial.println("Unable to connect to relay endpoint");
  }
}

void setup() {
  Serial.begin(115200);
  connectWiFi();
  initCamera();
}

void loop() {
  Serial.println("Capturing image...");
  camera_fb_t *fb = esp_camera_fb_get();
  if(!fb) {
    Serial.println("Camera capture failed");
    delay(20000);
    return;
  }
  
  // Upload image to paste.rs and retrieve the URL
  String pasteUrl = uploadPhotoToPasteRS(fb);
  esp_camera_fb_return(fb);
  
  // If we got a valid URL, send it to the relay endpoint
  if(pasteUrl.length() > 0) {
    sendImageURLToRelay(pasteUrl);
  }
  
  delay(20000); // Wait 20 seconds before taking the next picture
}
