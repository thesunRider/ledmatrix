// Modified from: https://github.com/wilson3682/My-HUB75-ESP32-Practice-Files/tree/main
// by MrCodetastic

/***************************************************************************************
 * This sketch does a couple of things, it uses GFX_Lite's GFX_Layer to independently  
 * draw the background and the text onto separate layers. (memory used for each)
 * Then these are stacked on top of each other with some opacity, and drawn directly
 * to the dma_display via a callback (layer_draw_callback).
 *
 * These layers are offscreen pixel buffers that use  memory of their own
 * (approx 3 bytes for each pixel).
 * 
 * By using this approach, we don't really need to use DMA double buffering though.
 *
 ***************************************************************************************/

#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <NetworkUdp.h>
#include <ArduinoOTA.h>




const char *ssid = "unisiegen";
const char *password = "eLab4Zimtsuper#sicher";
uint32_t last_ota_time = 0;

//------------------------------------------------------------------------------------------------------------------

// Configure for your panel(s) as appropriate!
#define PANEL_WIDTH 64
#define PANEL_HEIGHT 32  // Panel height of 64 will required PIN_E to be defined.
#define CHAIN_LENGTH 2   // Number of chained panels, if just a single panel, obviously set to 1


//------------------------------------------------------------------------------------------------------------------

/*
#define RL1 18
#define GL1 17
#define BL1 16
#define RL2 15
#define GL2 7
#define BL2 6
#define CH_A 4
#define CH_B 10
#define CH_C 14
#define CH_D 21
#define CH_E 5 // assign to any available pin if using two panels or 64x64 panels with 1/32 scan
#define CLK 47
#define LAT 48
#define OE  38
*/

//
//
//https://thelastoutpostworkshop.github.io/microcontroller_devkit/esp32partitionbuilder/
//------------------------------------------------------------------------------------------------------------------

MatrixPanel_I2S_DMA *dma_display = nullptr;

//------------------------------------------------------------------------------------------------------------------

//====================== Variables For scrolling Text=====================================================
unsigned long isAnimationDue;
int delayBetweeenAnimations = 60;                // Smaller == faster
int textXPosition = PANEL_WIDTH * CHAIN_LENGTH;  // Will start off screen
int textYPosition = PANEL_HEIGHT / 2 - 16; // center of screen - 8 (half of the text height)
//====================== Variables For scrolling Text=====================================================


// Pointers to this variable will be passed into getTextBounds,
// they will be updated from inside the method
int16_t xOne, yOne;
uint16_t w, h;



uint16_t myBLACK = dma_display->color565(0, 0, 0);
uint16_t myWHITE = dma_display->color565(255, 255, 255);
uint16_t myRED = dma_display->color565(255, 0, 0);
uint16_t myGREEN = dma_display->color565(0, 255, 0);
uint16_t myBLUE = dma_display->color565(0, 0, 255);

//------------------------------------------------------------------------------------------------------------------

void setup() {

  Serial.begin(115200);
  Serial.println(F("\n##################################"));

  Serial.println(F("ESP32 Information:"));

  Serial.printf("Internal Total Heap %d, Internal Used Heap %d, Internal Free Heap %d\n", ESP.getHeapSize(), ESP.getHeapSize() - ESP.getFreeHeap(), ESP.getFreeHeap());

  Serial.printf("Sketch Size %d, Free Sketch Space %d\n", ESP.getSketchSize(), ESP.getFreeSketchSpace());

  Serial.printf("SPIRam Total heap %d, SPIRam Free Heap %d\n", ESP.getPsramSize(), ESP.getFreePsram());

  Serial.printf("Chip Model %s, ChipRevision %d, Cpu Freq %d, SDK Version %s\n", ESP.getChipModel(), ESP.getChipRevision(), ESP.getCpuFreqMHz(), ESP.getSdkVersion());

  Serial.printf("Flash Size %d, Flash Speed %d\n", ESP.getFlashChipSize(), ESP.getFlashChipSpeed());

  Serial.println(F("##################################\n\n"));

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  ArduinoOTA
    .onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH) {
        type = "sketch";
      } else {  // U_SPIFFS
        type = "filesystem";
      }

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      Serial.println("Start updating " + type);
    })
    .onEnd([]() {
      Serial.println("\nEnd");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      if (millis() - last_ota_time > 500) {
        Serial.printf("Progress: %u%%\n", (progress / (total / 100)));
        last_ota_time = millis();
      }
    })
    .onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) {
        Serial.println("Auth Failed");
      } else if (error == OTA_BEGIN_ERROR) {
        Serial.println("Begin Failed");
      } else if (error == OTA_CONNECT_ERROR) {
        Serial.println("Connect Failed");
      } else if (error == OTA_RECEIVE_ERROR) {
        Serial.println("Receive Failed");
      } else if (error == OTA_END_ERROR) {
        Serial.println("End Failed");
      }
    });

  ArduinoOTA.begin();

  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  Serial.println("HTTP server started");

  //Custom pin mapping for all pins
  HUB75_I2S_CFG mxconfig(
    PANEL_WIDTH,   // width
    PANEL_HEIGHT,  // height
    CHAIN_LENGTH   // chain length
                   // ,_pins           // pin mapping
                   // ,HUB75_I2S_CFG::FM6126A         // driver chip
  );

  //mxconfig.i2sspeed = HUB75_I2S_CFG::HZ_20M;

  // If you are using a 64x64 matrix you need to pass a value for the E pin
  // The trinity connects GPIO 18 to E.
  // This can be commented out for any smaller displays (but should work fine with it)
  //mxconfig.gpio.e = -1;

  // May or may not be needed depending on your matrix
  // Example of what needing it looks like:
  // https://github.com/mrfaptastic/ESP32-HUB75-MatrixPanel-I2S-DMA/issues/134#issuecomment-866367216
  //mxconfig.clkphase = false;

  //mxconfig.clkphase = true;
  mxconfig.latch_blanking = 2;
  mxconfig.i2sspeed = HUB75_I2S_CFG::HZ_20M;

  // Some matrix panels use different ICs for driving them and some of them have strange quirks.
  // If the display is not working right, try this.
  //mxconfig.driver = HUB75_I2S_CFG::FM6126A;

  dma_display = new MatrixPanel_I2S_DMA(mxconfig);
  dma_display->begin();
  dma_display->setBrightness8(255);  //0-255
  dma_display->clearScreen();


  dma_display->fillScreen(myBLACK);

  dma_display->setTextSize(4);     // size 2 == 16 pixels high
  dma_display->setTextWrap(false); // N.B!! Don't wrap at end of line
    dma_display->setTextColor(myWHITE);

}

String text = "ELAB IS OPEN WELCOME!";

void loop() {

  ArduinoOTA.handle();

//  http://141.99.58.241/update

  unsigned long now = millis();
  if (now > isAnimationDue)
  {
    isAnimationDue = now + delayBetweeenAnimations;

    textXPosition -= 1;

    // Checking is the very right of the text off screen to the left
    dma_display->getTextBounds(text, textXPosition, textYPosition, &xOne, &yOne, &w, &h);
    if (textXPosition + w <= 0)
    {
      textXPosition = 64;
    }

    dma_display->setCursor(textXPosition, textYPosition);

    // The display has to do less updating if you only black out the area
    // the text is
    dma_display->fillScreen(myBLACK);
    //dma_display->fillRect(0, textYPosition, dma_display->width(), 16, myBLACK);
    dma_display->print(text);
  }


}
