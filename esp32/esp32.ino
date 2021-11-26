// Name: Soksedtha Ly
// ID: 6188147
// Section: 2

/*----------------------------------------------------*/
// Camera & Server Initialization
#include "OV7670.h"
#include <WiFi.h>
#include <WiFiMulti.h>
#include <WiFiClient.h>
#include "BMP.h"

const int SIOD = 21; //SDA
const int SIOC = 22; //SCL
const int VSYNC = 34;
const int HREF = 35;
const int XCLK = 32;
const int PCLK = 33;
const int D0 = 27;
const int D1 = 17;
const int D2 = 16;
const int D3 = 25;
const int D4 = 14;
const int D5 = 13;
const int D6 = 12;
const int D7 = 26;

#define ssid1        "Chimpanzee"
#define password1    "12346780"
OV7670 *camera;
WiFiMulti wifiMulti;
WiFiServer server(80);
unsigned char bmpHeader[BMP::headerSize];

/*----------------------------------------------------*/
// LCD Initialization
#include <TFT_eSPI.h>
#include <SPI.h>

TFT_eSPI tft = TFT_eSPI();
unsigned long targetTime = 0;
byte red = 31;
byte green = 0;
byte blue = 0;
byte state = 0;
unsigned int colour = red << 11; // Colour order is RGB 5+6+5 bits each

/*----------------------------------------------------*/
// Switch + Interrupt Initialization
#define SW 5
volatile bool cameraOn;
volatile bool intFlag;
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
void IRAM_ATTR isr();

// serve() returns the response to client HTTP requests
void serve()
{
  WiFiClient client = server.available();
  
  if (client) {
    String currentLine = "";
    
    // open connection
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        
        if (c == '\n') {
          // Prepare the header
          if (currentLine.length() == 0) {
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println();
            
            // If the camera is on, include the img frame.
            if(cameraOn) {
              client.print(
                "<style>body{margin: 0; text-align: center;}\nimg{height: 60%; width: auto;}\nh1{padding: 1em; padding-top: 1.5em;}</style>"
                "<h1>The camera is ON.</h1>"
                "<img id='a' src='/camera' onload='this.style.display=\"initial\"; var b = document.getElementById(\"b\"); b.style.display=\"none\"; b.src=\"camera?\"+Date.now(); '>"
                "<img id='b' class=\"center\" style='display: none' src='/camera' onload='this.style.display=\"initial\"; var a = document.getElementById(\"a\"); a.style.display=\"none\"; a.src=\"camera?\"+Date.now(); '>");
            }
            
            // Otherwise, include only the message.
            else {
              client.print(
                "<style>body{margin: 0; text-align: center;}\nimg{height: 60%; width: auto;}\nh1{padding: 1em; padding-top: 1.5em;}</style>"
                "<h1>The camera is OFF.</h1>");
            }
            client.println();
            break;
          } 
          else {
            currentLine = "";
          }
        } 
        else if (c != '\r') {
          currentLine += c;
        }

        // Return camera frames to the GET /camera request
        if(currentLine.endsWith("GET /camera")) {
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:image/bmp");
            client.println();
            
            client.write(bmpHeader, BMP::headerSize);
            client.write(camera->frame, camera->xres * camera->yres * 2);
        }
      }
    }
    // close the connection:
    client.stop();
  }  
}

void setup() {
  // Switch + Interrupt Setup
  pinMode(SW, INPUT);
  attachInterrupt(digitalPinToInterrupt(SW), isr, FALLING);
  cameraOn = true;
  intFlag = false;
  Serial.begin(115200);

  // Camera & Server Setup
  wifiMulti.addAP(ssid1, password1);
  Serial.println("Connecting Wifi...");
  if(wifiMulti.run() == WL_CONNECTED) {
      Serial.println("");
      Serial.println("WiFi connected");
      Serial.println("IP address: ");
      Serial.println(WiFi.localIP());
  }
  camera = new OV7670(OV7670::Mode::QQVGA_RGB565, SIOD, SIOC, VSYNC, HREF, XCLK, PCLK, D0, D1, D2, D3, D4, D5, D6, D7);
  BMP::construct16BitHeader(bmpHeader, camera->xres, camera->yres);
  server.begin();

  // LCD Setup
  tft.init();
  tft.setRotation(3);
  tft.fillScreen(TFT_BLACK);
  tft.setTextSize(10);
  tft.setCursor(120, 100);
  tft.setTextColor(TFT_WHITE);
  tft.println("ON");
  targetTime = millis() + 1000;
}

void loop() {
  // If the switch is interrupted, update the LCD.
  if(intFlag){
    portENTER_CRITICAL(&mux);
    intFlag = false;
    portEXIT_CRITICAL(&mux);

    // Update the message to "ON"
    if(cameraOn) {
      tft.fillScreen(TFT_BLACK);
      tft.setTextSize(10);
      tft.setCursor(120, 100);
      tft.setTextColor(TFT_WHITE);
      tft.println("ON");
    }
    
    // Update the message to "OFF"
    else {
      tft.fillScreen(TFT_BLACK);
      tft.setTextSize(10);
      tft.setCursor(100, 100);
      tft.setTextColor(TFT_WHITE);
      tft.println("OFF");
    }
    
    Serial.print("Camera: ");
    Serial.println(cameraOn);
  }

  // If the camera is on, get the current camera frame
  if(cameraOn) {
    camera->oneFrame();
  }
  serve();
  delay(100);
}

// Detect interrupt
void IRAM_ATTR isr() {
  portENTER_CRITICAL_ISR(&mux);
  intFlag = true;
  cameraOn = !cameraOn;
  portEXIT_CRITICAL_ISR(&mux);
}
