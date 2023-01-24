#include <Adafruit_HX8357.h>
#include <Adafruit_GFX.h>
#include <TouchScreen.h>
#include <SPI.h>
#include <FlexCAN_T4.h>

//Touch Screen Setup
// Define the display and touch screen objects
const int TFT_CS = 10;
const int TFT_DC = 9;
const int TFT_RST = 8;
Adafruit_HX8357 tft = Adafruit_HX8357(TFT_CS, TFT_DC, TFT_RST);
Adafruit_GFX_Button button;

// This is calibration data for the raw touch data to the screen coordinates
#define TS_MINX 109
#define TS_MINY 65
#define TS_MAXX 926
#define TS_MAXY 950

#define MINPRESSURE 10
#define MAXPRESSURE 1000

// Define the touch screen pins as analog
#define YP A0  // must be an analog pin, use "An" notation!
#define XM A3  // must be an analog pin, use "An" notation!
#define YM 16   // can be a digital pin
#define XP 15   // can be a digital pin
TouchScreen ts = TouchScreen(XP, YP, XM, YM, 290.6);

int can1BR = 250;
int can2BR = 250;

//Can Controller Setup
//Setting up buffers for Can1, Can2, and CanFD in parallel with Can1
FlexCAN_T4<CAN1, RX_SIZE_256, TX_SIZE_16> Can1;
FlexCAN_T4<CAN2, RX_SIZE_256, TX_SIZE_16> Can2;
FlexCAN_T4FD<CAN3, RX_SIZE_256, TX_SIZE_16> Canfd;
CANFD_timings_t config;

//Define message from FlexCAN library
static CAN_message_t txmsg1;
static CAN_message_t txmsg2;
static CAN_message_t rxmsg1;
static CAN_message_t rxmsg2;

//Initialize send and receive counters
uint32_t TXCount1 = 0;
uint32_t TXCount2 = 0;
uint32_t RXCount1 = 0;
uint32_t RXCount2 = 0;
uint32_t prevMsgId1 = -1;
uint32_t prevMsgId2 = -1;

//Define LED
#define GREEN_LED_PIN 2
#define RED_LED_PIN 3
#define BLUE_LED_PIN 4
#define ORANGE_LED_PIN 5
boolean GREEN_LED_state;
boolean RED_LED_state;
boolean BLUE_LED_state;
boolean ORANGE_LED_state;
boolean LED_BUILTIN_state;

//Define default baudrate
#define BAUDRATE250K 250000
#define BAUDRATE500K 500000

//Initialize logic variables for enabling CAN lines
boolean toggle = false;
boolean enableCan1 = true;
boolean enableCan2 = false;
//add CANFD

//Initialize Timers
elapsedMillis blinkTimer;
elapsedMillis testTimer;

void setup() {
  // Initialize touch screen and draw main menu
  tft.begin();
  tft.setRotation(3);
  tft.setSPISpeed(19000000);
  Serial.begin(115200);
  drawMenu();

  Can1.begin();
  Can1.setBaudRate(BAUDRATE250K);
  Can1.setMaxMB(16);
  Can1.enableFIFO(true);                                            // Enable FIFO to allow for overflow between RX mailboxes
  Can1.enableFIFOInterrupt();
  Can1.onReceive(canSniff1);                                         // Interupt handler for counting and aknowledging all received CAN frames

  Can2.begin();
  Can2.setBaudRate(BAUDRATE250K);
  Can2.setMaxMB(16);
  Can2.enableFIFO(true);                                            // Enable FIFO to allow for overflow between RX mailboxes
  Can2.enableFIFOInterrupt();
  Can2.onReceive(canSniff2);
  
  Canfd.begin();
  config.clock = CLK_24MHz;
  config.baudrate = 1000000;
  config.baudrateFD = 2000000;
  config.propdelay = 190;
  config.bus_length = 1;
  config.sample = 70;
  Canfd.setBaudRate(config);

  //Set message extension, ID, and length
  txmsg1.flags.extended = 1;
  txmsg1.len = 8;

  txmsg2.flags.extended = 1;
  txmsg2.len = 8;

  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(BLUE_LED_PIN, OUTPUT);
  pinMode(ORANGE_LED_PIN, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
    // Polling touch screen coordinates  
	TSPoint p = ts.getPoint();

	// we have some minimum pressure we consider 'valid'
    // pressure of 0 means no pressing!
	if (p.z < MINPRESSURE || p.z > MAXPRESSURE){
    	return;
  	}
	//Using calibration data to map analog data to screen coordinates
  	p.x = map(p.x, TS_MINX, TS_MAXX, 0, tft.width());
  	p.y = map(p.y, TS_MINY, TS_MAXY, 0, tft.height());

	Serial.print("X = "); Serial.print(p.x);
	Serial.print("\tY = "); Serial.print(p.y);
	Serial.print("\tPressure = "); Serial.println(p.z);

  //Quick Start Press
  if (p.x > 0 && p.x < 70 && p.y > 210 && p.y < 320) {
    	// Animate the button press by changing the color of the button
    	drawText(12,15,"Quick Start",HX8357_BLUE);
    	delay(200);
    	tft.fillScreen(HX8357_WHITE);
      drawText(0,0,"Performing Quick Test...");
      //Code to peform quick test
      delay(200);
      tft.println("(Results)");
      do {
      p = ts.getPoint();
      }while(p.z < MINPRESSURE);
      drawMenu();
	}

  //Self Test Press
  if (p.x > 0 && p.x < 70 && p.y > 100 && p.y < 210) {
    	// Animate the button press by changing the color of the button
    	drawText(185,15,"Self Test",HX8357_BLUE);
    	delay(200);
    	tft.fillScreen(HX8357_WHITE);
      drawText(0,0,"Performing Self Test...");
      //Code to peform quick test
      delay(200);
      tft.println("(Results)");
      do {
      p = ts.getPoint();
      }while(p.z < MINPRESSURE);
      drawMenu();
  }

  //Advanced Button Press
  if (p.x > 0 && p.x < 70 && p.y > 0 && p.y < 100) {
    	// Animate the button press by changing the color of the button
    	drawText(350,15,"Advanced",HX8357_BLUE);
    	delay(200);
    	// Return the button to its original color
    	drawText(350,15,"Advanced",HX8357_BLACK);
  }

  //CAN1 Baudrate Button
	if (p.x > 200 && p.x < 250 && p.y > 100 && p.y < 120) {
    	// Animate the button press by changing the color of the button
    	tft.fillTriangle(298,135,298,160,320,147.5,HX8357_BLUE);
    	delay(200);
    	// Return the button to its original color
    	tft.fillTriangle(298,135,298,160,320,147.5,HX8357_WHITE);
	}

  //CAN2 Baudrate Button
	if (p.x > 350 && p.x < 400 && p.y > 100 && p.y < 120) {
    	// Animate the button press by changing the color of the button
    	tft.fillTriangle(298,235,298,260,320,247.5,HX8357_BLUE);
    	delay(200);
    	// Return the button to its original color
    	tft.fillTriangle(298,235,298,260,320,247.5,HX8357_WHITE);
	}

  //Run Button Press
  if (p.x > 420 && p.x < 470 && p.y > 0 && p.y < 50) {
    	// Animate the button press by changing the color of the button
    	drawText(422,287,"Run",HX8357_BLUE);
    	delay(200);
    	// Return the button to its original color
    	drawText(422,287,"Run",HX8357_BLACK);
  }
}

//Possible helper function
void drawResults(String msg){
  tft.fillScreen(HX8357_WHITE);
  drawText(0,0,msg);
}

//Draw main menu
void drawMenu() {
  //480x320 pixels
  tft.fillScreen(HX8357_WHITE);
  tft.setTextColor(HX8357_BLACK);  tft.setTextSize(2);

  //Quick Start Button
  drawText(12,15,"Quick Start");
  tft.drawRect(0,0,160,50,HX8357_BLACK);

  //Self Test Button
  drawText(185,15,"Self Test");
  tft.drawRect(160,0,160,50,HX8357_BLACK);

  //Advanced Settings Button
  drawText(350,15,"Advanced");
  tft.drawRect(320,0,160,50,HX8357_BLACK);

  //Can2.0 selected by default
  drawText(20,70,"Can2.0",HX8357_BLUE);
  tft.drawRect(10,60,90,40,HX8357_BLACK);
  
  //CanFD
  drawText(115,70,"CanFD");
  tft.drawRect(100,60,90,40,HX8357_BLACK);

  //Can 1 Settings
  drawText(15,115,"Can1:");
  drawText(25,140,"Baudrate(Kbps) - ");
  tft.drawRect(230,130,55,35,HX8357_BLACK);
  drawText(240,140,String(can1BR));
  tft.drawTriangle(295,130,295,165,325,147.5,HX8357_BLACK);

  //Can 2 Settings
  drawText(15,215,"Can2:");
  drawText(25,240,"Baudrate(Kbps) - ");
  tft.drawRect(230,230,55,35,HX8357_BLACK);
  drawText(240,240,String(can2BR));
  tft.drawTriangle(295,230,295,265,325,247.5,HX8357_BLACK);

  //Run Button
  drawText(422,287,"Run");
  tft.drawRect(400,270,80,50,HX8357_BLACK);
}

//Helper function for drawing text at many different coordinate positions
void drawText(int x, int y, String msg){
  tft.setCursor(x,y);
  tft.println(msg);
}

//Inherited function allows for animating text on button press
void drawText(int x, int y, String msg, byte color){
  tft.setTextColor(color);
  tft.setCursor(x,y);
  tft.println(msg);
  tft.setTextColor(HX8357_BLACK);
}

void canSniff1(const CAN_message_t &rxmsg1) {                        // Function to increment on each received CAN frame
  Serial.print("Can1 ID: "); Serial.print(rxmsg1.id);
  Serial.println();
  RXCount1++;
  if (rxmsg1.id - prevMsgId1 != 1) {
    Serial.printf("Can 1 Test Failed: ID's did not match\n");
    Serial.printf("Previous Message ID: %d\n", prevMsgId1);
    Serial.printf("Current Message ID: %d\n", rxmsg1.id);
    digitalWrite(RED_LED_PIN, HIGH);
    } else {
    prevMsgId1 = rxmsg1.id;
    }
    BLUE_LED_state = !BLUE_LED_state;
    digitalWrite(BLUE_LED_PIN, BLUE_LED_state);
}

void canSniff2(const CAN_message_t &rxmsg2) {                        // Function to increment on each received CAN frame
  Serial.print("Can2 ID: "); Serial.print(rxmsg2.id);
  Serial.println();
  RXCount2++;
  if (rxmsg2.id - prevMsgId2 != 1) {
    Serial.printf("Can 2 Test Failed: ID's did not match\n");
    Serial.printf("Previous Message ID: %d\n", prevMsgId2);
    Serial.printf("Current Message ID: %d\n", rxmsg2.id);
    digitalWrite(RED_LED_PIN, HIGH);
    } else {
    prevMsgId2 = rxmsg2.id;
    }
    ORANGE_LED_state = !ORANGE_LED_state;
    digitalWrite(ORANGE_LED_PIN, ORANGE_LED_state);
}
