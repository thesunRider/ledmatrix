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
#include <WiFiClient.h>
#include <AnimatedGIF.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include "LittleFS.h"
#include <ElegantOTA.h>
#include <PNGdec.h>

AsyncWebServer server(80);
const char *ssid_sta = "AKPU_2.4GHz";//"unisiegen";
const char *password_sta = "Kingpin007";//"eLab4Zimtsuper#sicher";

const char *ssid_ap = "elab_hub75";
const char *password_ap = "elab@123";

//------------------------------------------------------------------------------------------------------------------

// Configure for your panel(s) as appropriate!
#define PANEL_WIDTH 64
#define PANEL_HEIGHT 32 // Panel height of 64 will required PIN_E to be defined.
#define CHAIN_LENGTH 2  // Number of chained panels, if just a single panel, obviously set to 1

#define PANEL_WIDTH_X (PANEL_WIDTH * CHAIN_LENGTH)
#define PANEL_WIDTH_Y (PANEL_HEIGHT)

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
// https://thelastoutpostworkshop.github.io/microcontroller_devkit/esp32partitionbuilder/
//------------------------------------------------------------------------------------------------------------------

MatrixPanel_I2S_DMA *dma_display = nullptr;

//------------------------------------------------------------------------------------------------------------------

//====================== Variables For scrolling Text=====================================================
unsigned long isAnimationDue;
int delayBetweeenAnimations = 60;               // Smaller == faster
int textXPosition = PANEL_WIDTH * CHAIN_LENGTH; // Will start off screen
int textYPosition = PANEL_HEIGHT / 2 - 16;      // center of screen - 8 (half of the text height)
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

String text_show = "ELAB IS OPEN WELCOME!";
#define DISPLAY_TEXT 0
#define DISPLAY_GRAPHIC 1
#define DISPLAY_ANIMATION 2

#define FILESYSTEM LittleFS

int DISPLAY_METHOD = DISPLAY_TEXT;

PNG png;
AnimatedGIF gif;
File f;
int x_offset, y_offset;

// Draw a line of image directly on the LED Matrix
void GIFDraw(GIFDRAW *pDraw)
{
  uint8_t *s;
  uint16_t *d, *usPalette, usTemp[320];
  int x, y, iWidth;

  iWidth = pDraw->iWidth;
  if (iWidth > dma_display->width())
    iWidth = dma_display->width();

  usPalette = pDraw->pPalette;
  y = pDraw->iY + pDraw->y; // current line

  s = pDraw->pPixels;
  if (pDraw->ucDisposalMethod == 2) // restore to background color
  {
    for (x = 0; x < iWidth; x++)
    {
      if (s[x] == pDraw->ucTransparent)
        s[x] = pDraw->ucBackground;
    }
    pDraw->ucHasTransparency = 0;
  }
  // Apply the new pixels to the main image
  if (pDraw->ucHasTransparency) // if transparency used
  {
    uint8_t *pEnd, c, ucTransparent = pDraw->ucTransparent;
    int x, iCount;
    pEnd = s + pDraw->iWidth;
    x = 0;
    iCount = 0; // count non-transparent pixels
    while (x < pDraw->iWidth)
    {
      c = ucTransparent - 1;
      d = usTemp;
      while (c != ucTransparent && s < pEnd)
      {
        c = *s++;
        if (c == ucTransparent) // done, stop
        {
          s--; // back up to treat it like transparent
        }
        else // opaque
        {
          *d++ = usPalette[c];
          iCount++;
        }
      } // while looking for opaque pixels
      if (iCount) // any opaque pixels?
      {
        for (int xOffset = 0; xOffset < iCount; xOffset++)
        {
          dma_display->drawPixel(x + xOffset, y, usTemp[xOffset]); // 565 Color Format
        }
        x += iCount;
        iCount = 0;
      }
      // no, look for a run of transparent pixels
      c = ucTransparent;
      while (c == ucTransparent && s < pEnd)
      {
        c = *s++;
        if (c == ucTransparent)
          iCount++;
        else
          s--;
      }
      if (iCount)
      {
        x += iCount; // skip these
        iCount = 0;
      }
    }
  }
  else // does not have transparency
  {
    s = pDraw->pPixels;
    // Translate the 8-bit pixels through the RGB565 palette (already byte reversed)
    for (x = 0; x < pDraw->iWidth; x++)
    {
      dma_display->drawPixel(x, y, usPalette[*s++]); // color 565
    }
  }
} /* GIFDraw() */

void *GIFOpenFile(const char *fname, int32_t *pSize)
{
  Serial.print("Playing gif: ");
  Serial.println(fname);
  f = FILESYSTEM.open(fname);
  if (f)
  {
    *pSize = f.size();
    return (void *)&f;
  }
  return NULL;
} /* GIFOpenFile() */

void GIFCloseFile(void *pHandle)
{
  File *f = static_cast<File *>(pHandle);
  if (f != NULL)
    f->close();
} /* GIFCloseFile() */

int32_t GIFReadFile(GIFFILE *pFile, uint8_t *pBuf, int32_t iLen)
{
  int32_t iBytesRead;
  iBytesRead = iLen;
  File *f = static_cast<File *>(pFile->fHandle);
  // Note: If you read a file all the way to the last byte, seek() stops working
  if ((pFile->iSize - pFile->iPos) < iLen)
    iBytesRead = pFile->iSize - pFile->iPos - 1; // <-- ugly work-around
  if (iBytesRead <= 0)
    return 0;
  iBytesRead = (int32_t)f->read(pBuf, iBytesRead);
  pFile->iPos = f->position();
  return iBytesRead;
} /* GIFReadFile() */

int32_t GIFSeekFile(GIFFILE *pFile, int32_t iPosition)
{
  int i = micros();
  File *f = static_cast<File *>(pFile->fHandle);
  f->seek(iPosition);
  pFile->iPos = (int32_t)f->position();
  i = micros() - i;
  //  Serial.printf("Seek time = %d us\n", i);
  return pFile->iPos;
} /* GIFSeekFile() */

char gifpath[] = "/anim/animation.gif";
File gifFile;

unsigned long start_tick = 0;

void ShowGIF(char *name)
{
  start_tick = millis();

  if (gif.open(name, GIFOpenFile, GIFCloseFile, GIFReadFile, GIFSeekFile, GIFDraw))
  {
    x_offset = (dma_display->width() - gif.getCanvasWidth()) / 2;
    if (x_offset < 0)
      x_offset = 0;
    y_offset = (dma_display->height() - gif.getCanvasHeight()) / 2;
    if (y_offset < 0)
      y_offset = 0;
    Serial.printf("Successfully opened GIF; Canvas size = %d x %d\n", gif.getCanvasWidth(), gif.getCanvasHeight());
    Serial.flush();
  }

} /* ShowGIF() */

void startgifplay()
{
  gifFile = LittleFS.open(gifpath, "r");
  if (gifFile)
  {
    // Show it.
    ShowGIF(gifpath);
  }
}

void handlesettings(AsyncWebServerRequest *request)
{
  String upinterval = request->arg("upinterval");
  delayBetweeenAnimations = upinterval.toInt();
}

void handleFormText(AsyncWebServerRequest *request)
{
  text_show = request->arg("text");
  String text_size = request->arg("size");
  String text_color = request->arg("color");
  text_color.replace("#", "");
  uint8_t r = strtol(text_color.substring(0, 2).c_str(), NULL, 16);
  uint8_t g = strtol(text_color.substring(2, 4).c_str(), NULL, 16);
  uint8_t b = strtol(text_color.substring(4, 6).c_str(), NULL, 16);

  dma_display->fillScreen(myBLACK);

  // add options if adding vertical pannel chaining
  if (text_size.toInt() <= 4)
    dma_display->setTextSize(text_size.toInt()); // size 2 == 16 pixels high
  dma_display->setTextWrap(false);               // N.B!! Don't wrap at end of line

  dma_display->setTextColor(dma_display->color565(r, g, b));

  Serial.print("Showtext:\t");
  Serial.println(text_show);
  Serial.println(r);
  Serial.println(g);
  Serial.println(b);
  DISPLAY_METHOD = DISPLAY_TEXT;
}

// Functions to access a file on the SD card
File pngfile;

void *myOpen(const char *filename, int32_t *size)
{
  Serial.printf("Attempting to open %s\n", filename);
  pngfile = LittleFS.open(filename);
  *size = pngfile.size();
  return &pngfile;
}
void myClose(void *handle)
{
  if (pngfile)
    pngfile.close();
}
int32_t myRead(PNGFILE *handle, uint8_t *buffer, int32_t length)
{
  if (!pngfile)
    return 0;
  return pngfile.read(buffer, length);
}
int32_t mySeek(PNGFILE *handle, int32_t position)
{
  if (!pngfile)
    return 0;
  return pngfile.seek(position);
}

int png_width = 0;
int png_height = 0;
int png_scroll_constant = 0;
bool frame_complete_png = false;

// Ensure buffer large enough for PNG width
static uint16_t usPixels[PANEL_WIDTH_X * 5]; // adjust max PNG width
static uint16_t dustPixels[PANEL_WIDTH_X];
int PNGDraw(PNGDRAW *pDraw)
{

  // usPixels buffer large enough for full PNG line
  memset(usPixels, 0, png_width * sizeof(uint16_t));
  png.getLineAsRGB565(pDraw, usPixels, PNG_RGB565_LITTLE_ENDIAN, 0xffffffff);

  // dustPixels buffer holds full panel line
  memset(dustPixels, 0, PANEL_WIDTH_X * sizeof(uint16_t));

    if (png_width > PANEL_WIDTH_X)
  {
  for (int i = 0; i < PANEL_WIDTH_X; i++)
  {
    // wrap-around scroll
    int srcIndex = (png_scroll_constant + i) % pDraw->iWidth;
    dustPixels[i] = usPixels[srcIndex];
  }}else{
    for (int i = 0; i < png_width; i++)
    {
      dustPixels[i] = usPixels[i];
    }
  }

  // Draw to display
  for (int i = 0; i < PANEL_WIDTH_X; i++)
  {
    dma_display->drawPixel(i, pDraw->y, dustPixels[i]);
  }

  if (png_height == PANEL_WIDTH_Y)
    frame_complete_png = true;

  return 1;
}

void startpngplay()
{
  int rc = png.open("/img/graphic.png", myOpen, myClose, myRead, mySeek, PNGDraw);
  if (rc == PNG_SUCCESS)
  {
    png_width = png.getWidth();
    png_height = png.getHeight();
    Serial.printf("image specs: (%d x %d), %d bpp, pixel type: %d\n", png.getWidth(), png.getHeight(), png.getBpp(), png.getPixelType());
    rc = png.decode(NULL, 0);
  }
}

void setup()
{

  Serial.begin(115200);
  Serial.println(F("\n##################################"));

  Serial.println(F("ESP32 Information:"));

  Serial.printf("Internal Total Heap %d, Internal Used Heap %d, Internal Free Heap %d\n", ESP.getHeapSize(), ESP.getHeapSize() - ESP.getFreeHeap(), ESP.getFreeHeap());

  Serial.printf("Sketch Size %d, Free Sketch Space %d\n", ESP.getSketchSize(), ESP.getFreeSketchSpace());

  Serial.printf("SPIRam Total heap %d, SPIRam Free Heap %d\n", ESP.getPsramSize(), ESP.getFreePsram());

  Serial.printf("Chip Model %s, ChipRevision %d, Cpu Freq %d, SDK Version %s\n", ESP.getChipModel(), ESP.getChipRevision(), ESP.getCpuFreqMHz(), ESP.getSdkVersion());

  Serial.printf("Flash Size %d, Flash Speed %d\n", ESP.getFlashChipSize(), ESP.getFlashChipSpeed());

  Serial.println(F("##################################\n\n"));

  if (!LittleFS.begin(true))
  {
    Serial.println("An Error has occurred while mounting LittleFS");
    return;
  }

  Serial.print("Setting AP (Access Point)…");

  WiFi.mode(WIFI_MODE_APSTA);

  WiFi.begin(ssid_sta, password_sta);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.println("Connecting to WiFi..");
  }


  // Remove the password parameter, if you want the AP (Access Point) to be open
  WiFi.softAP(ssid_ap, password_ap);
  

  Serial.print("ESP32 IP as soft AP: ");
  Serial.println(WiFi.softAPIP());
 
  Serial.print("ESP32 IP on the WiFi network: ");
  Serial.println(WiFi.localIP());
  
  // Initialize mDNS
  if (!MDNS.begin("elabhub75"))
  { // Set the hostname to "elabhub75.local"
    Serial.println("Error setting up MDNS responder!");
    while (1)
    {
      delay(1000);
    }
  }
  Serial.println("mDNS responder started");
  MDNS.addService("http", "tcp", 80);

  server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");
  // server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
  //   request->send(LittleFS, "/index.html");
  //  });

  server.on("/upload-text", HTTP_POST, handleFormText);
  server.on("/settings", HTTP_POST, handlesettings);

  // Handle POST requests for file uploads
  server.on("/upload-image", HTTP_POST, [](AsyncWebServerRequest *request)
            {
              // This handler is called when the request body is fully received.
              // We already saved the file in the onFileUpload handler.
              if (request->hasParam("status"))
              {
                Serial.println("Upload complete from HUB.");
                request->send(200, "text/plain", "Upload successful!");
              }
              else
              {
                request->send(200, "text/plain", "Upload started.");
              }
            },
            [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final)
            {
    // This is the onFileUpload handler, called as data chunks arrive.
    // It's crucial for saving the incoming file data.

    if(!index){ // First chunk of the file
      Serial.printf("Upload Started");
      // Open the file for writing
      if (FILESYSTEM.exists("/img/graphic.png")) {
            FILESYSTEM.remove("/img/graphic.png");
        }

      request->_tempFile = FILESYSTEM.open("/img/graphic.png", "w");
      if(!request->_tempFile){
        Serial.println("Failed to open file for writing");
        return;
      }

    }
    if(len){ // Write data chunks
      if(request->_tempFile){
        request->_tempFile.write(data, len);
      }
    }
    if(final){ // Last chunk of the file
      Serial.printf("Upload End, Size: %u\n", index + len);
      if(request->_tempFile){
        request->_tempFile.close();
      }
      png_scroll_constant = 0;
      startpngplay();
      DISPLAY_METHOD = DISPLAY_GRAPHIC;
    } });

  // Handle POST requests for file uploads
  server.on("/upload-anim", HTTP_POST, [](AsyncWebServerRequest *request)
            {
              // This handler is called when the request body is fully received.
              // We already saved the file in the onFileUpload handler.
              if (request->hasParam("status"))
              {
                Serial.println("Upload complete from HUB.");
                request->send(200, "text/plain", "Upload successful!");
              }
              else
              {
                request->send(200, "text/plain", "Upload started.");
              }
            },
            [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final)
            {
    // This is the onFileUpload handler, called as data chunks arrive.
    // It's crucial for saving the incoming file data.

    if(!index){ // First chunk of the file
      Serial.printf("Upload Started");
      // Open the file for writing
      if (FILESYSTEM.exists("/anim/animation.gif")) {
            FILESYSTEM.remove("/anim/animation.gif");
        }

      request->_tempFile = FILESYSTEM.open("/anim/animation.gif", "w");
      if(!request->_tempFile){
        Serial.println("Failed to open file for writing");
        return;
      }

    }
    if(len){ // Write data chunks
      if(request->_tempFile){
        request->_tempFile.write(data, len);
      }
    }
    if(final){ // Last chunk of the file
      Serial.printf("Upload End, Size: %u\n", index + len);
      if(request->_tempFile){
        request->_tempFile.close();
      }
      startgifplay();
      DISPLAY_METHOD = DISPLAY_ANIMATION;
    } });

  ElegantOTA.begin(&server); // Start ElegantOTA
  server.begin();
  Serial.println("HTTP server started");

  // Custom pin mapping for all pins
  HUB75_I2S_CFG mxconfig(
      PANEL_WIDTH,  // width
      PANEL_HEIGHT, // height
      CHAIN_LENGTH  // chain length
                    // ,_pins           // pin mapping
                    // ,HUB75_I2S_CFG::FM6126A         // driver chip
  );

  // mxconfig.i2sspeed = HUB75_I2S_CFG::HZ_20M;

  // If you are using a 64x64 matrix you need to pass a value for the E pin
  // The trinity connects GPIO 18 to E.
  // This can be commented out for any smaller displays (but should work fine with it)
  // mxconfig.gpio.e = -1;

  // May or may not be needed depending on your matrix
  // Example of what needing it looks like:
  // https://github.com/mrfaptastic/ESP32-HUB75-MatrixPanel-I2S-DMA/issues/134#issuecomment-866367216
  // mxconfig.clkphase = false;

  mxconfig.clkphase = false;
  mxconfig.latch_blanking = 4;
  mxconfig.i2sspeed = HUB75_I2S_CFG::HZ_8M;

  // Some matrix panels use different ICs for driving them and some of them have strange quirks.
  // If the display is not working right, try this.
  // mxconfig.driver = HUB75_I2S_CFG::FM6126A;

  dma_display = new MatrixPanel_I2S_DMA(mxconfig);
  dma_display->begin();
  dma_display->setBrightness8(230); // 0-255
  dma_display->clearScreen();

  dma_display->fillScreen(myBLACK);

  dma_display->setTextSize(4);     // size 2 == 16 pixels high
  dma_display->setTextWrap(false); // N.B!! Don't wrap at end of line
  dma_display->setTextColor(myWHITE);

  gif.begin(LITTLE_ENDIAN_PIXELS);
}

void drawText()
{

  unsigned long now = millis();
  if (now > isAnimationDue)
  {
    isAnimationDue = now + delayBetweeenAnimations;

    textXPosition -= 1;

    // Checking is the very right of the text off screen to the left
    dma_display->getTextBounds(text_show, textXPosition, textYPosition, &xOne, &yOne, &w, &h);
    if (textXPosition + w <= 0)
    {
      textXPosition = 64;
    }

    dma_display->setCursor(textXPosition, textYPosition);
    dma_display->fillScreen(myBLACK);
    // dma_display->fillRect(0, textYPosition, dma_display->width(), 16, myBLACK);
    dma_display->print(text_show);
  }
}

unsigned long lastGifFrame = 0;

void loop()
{
  ElegantOTA.loop();
  //  http://141.99.58.241/update
  if (DISPLAY_METHOD == DISPLAY_TEXT)
  {
    if (gifFile)
    {
      gifFile.close();
      gif.close();
    }

    drawText();
  }

  if (DISPLAY_METHOD == DISPLAY_ANIMATION)
  {
    unsigned long now = millis();
    if (now - lastGifFrame >= delayBetweeenAnimations)
    {
      lastGifFrame = now;
      gif.playFrame(true, NULL);
    }
  }

  if (DISPLAY_METHOD == DISPLAY_GRAPHIC)
  {
    unsigned long now = millis();
    if (frame_complete_png && now - lastGifFrame >= delayBetweeenAnimations && png_width > PANEL_WIDTH_X)
    {
      lastGifFrame = now;
      frame_complete_png = false;
      png_scroll_constant++;
      if (png_scroll_constant == png_width)
        png_scroll_constant = 0;

      png.close();
      startpngplay();
    }
  }
}
