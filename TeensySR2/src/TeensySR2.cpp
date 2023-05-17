#include <Adafruit_HX8357.h>
#include <Adafruit_GFX.h>
#include <TouchScreen.h>
#include <SPI.h>
#include <FlexCAN_T4.h>
#include <initializer_list>

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
boolean Error1 = false;
boolean Error2 = false;
boolean enableCan1 = true;
boolean enableCan2 = false;
//add CANFD

//Initialize Timers
elapsedMillis blinkTimer;
elapsedMillis testTimer;


class Button
{
private:
  int16_t xMin = 0;
  int16_t xMax = 0;
  int16_t yMin = 0;
  int16_t yMax = 0;
  void (*callback)() = nullptr;

public:
  Button() = default;
  Button(int16_t _xMin, int16_t _xMax, int16_t _yMin, int16_t _yMax, void (*_callback)()) :
    xMin(_xMin), xMax(_xMax), yMin(_yMin), yMax(_yMax), callback(_callback) {};
  
  void pressedHandler(int16_t x, int16_t y) {
    if (x >= xMin && x <= xMax && y >= yMin && y <= yMax) {
      (callback)();
    }
  }
};

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

void canSniff1(const CAN_message_t &rxmsg1) {                        // Interrupt to increment on each received CAN frame and verify no loss of frames
  RXCount1++;
  if (((rxmsg1.id - prevMsgId1) != 1) && Error1 == false) {
    tft.println(rxmsg1.id - prevMsgId1);
    tft.printf("CAN 1 Test Failed: ID's did not match\n");
    tft.printf("Previous Message ID: %d\n", prevMsgId1);
    tft.printf("Current Message ID: %d\n", rxmsg1.id);
    Error1 = true;
    //digitalWrite(RED_LED_PIN, HIGH);
    } else {
    prevMsgId1 = rxmsg1.id;
    }
    //BLUE_LED_state = !BLUE_LED_state;
    //digitalWrite(BLUE_LED_PIN, BLUE_LED_state);
}

void canSniff2(const CAN_message_t &rxmsg2) {                        // Interrupt to increment on each received CAN frame and verify no loss of frames
  RXCount2++;
  if (((rxmsg2.id - prevMsgId2) != 1) && Error2 == false) {
    tft.println(rxmsg2.id - prevMsgId2);
    tft.printf("CAN 2 Test Failed: ID's did not match\n");
    tft.printf("Previous Message ID: %d\n", prevMsgId2);
    tft.printf("Current Message ID: %d\n", rxmsg2.id);
    Error2 = true;
    //digitalWrite(RED_LED_PIN, HIGH);
    } else {
     prevMsgId2 = rxmsg2.id;
    }
    //ORANGE_LED_state = !ORANGE_LED_state;
    //digitalWrite(ORANGE_LED_PIN, ORANGE_LED_state);
}

void resetCounters() {
  Error1 = false;
  Error2 = false;
  RXCount1 = 0;
  RXCount2 = 0;
  TXCount1 = 0;
  TXCount2 = 0;
  prevMsgId1 = -1;
  prevMsgId2 = -1;
  //digitalWrite(LED_BUILTIN, LOW);
  //digitalWrite(GREEN_LED_PIN, LOW);
  //digitalWrite(BLUE_LED_PIN, LOW);
  //digitalWrite(ORANGE_LED_PIN, LOW);
}

void runSelfTest() {
  testTimer = 0;
  //Turn on bridging
  while(TXCount1 < 100) {
    if ((Can1.getTXQueueCount() == 0)) {
      txmsg1.id = TXCount1;
      Can1.write(MB8, txmsg1);
      TXCount1++;
    } else if(testTimer > 400){
      break;
    }
  }
  testTimer = 0;
  delay(20);
  while(TXCount2 < 100) {
    if ((Can2.getTXQueueCount() == 0)){
      txmsg2.id = TXCount2;
      Can2.write(MB8, txmsg2);                                           // Send CAN message once the previous message is done transmitting
      TXCount2++;
    } else if(testTimer > 400){
      break;
    }
  }
  tft.printf("\nSelf Test Completed\n");
  tft.printf("Can1 messages sent: %d\n",TXCount1);
  tft.printf("Received: %d\n",RXCount2);
  tft.printf("Can2 messages sent: %d\n",TXCount2);
  tft.printf("Received: %d\n",RXCount1);
  resetCounters();
}

void quickStartHandle() {
  // Animate the button press by changing the color of the button
  drawText(12,15,"Quick Start",HX8357_BLUE);
  delay(200);
  tft.fillScreen(HX8357_WHITE);
  drawText(0,0,"Performing Quick Test...");
  //Code to peform quick test
  delay(200);
  tft.println("(Results)");
  TSPoint p = ts.getPoint();
  do {
    p = ts.getPoint();
  }while(p.z < MINPRESSURE);
  drawMenu();
}

void selfTestHandle() {
  // Animate the button press by changing the color of the button
  drawText(185,15,"Self Test",HX8357_BLUE);
  delay(200);
  tft.fillScreen(HX8357_WHITE);
  drawText(0,0,"Performing Self Test...");
  //Code to peform quick test
  runSelfTest();
  TSPoint p = ts.getPoint();
  do {
    p = ts.getPoint();
  }while(p.z < MINPRESSURE);
  drawMenu();
}

void advancedSettingHandle() {
  // Animate the button press by changing the color of the button
  drawText(350,15,"Advanced",HX8357_BLUE);
  delay(200);
  // Return the button to its original color
  drawText(350,15,"Advanced",HX8357_BLACK);
}

void can1BaudrateHandle() {
  // Animate the button press by changing the color of the button
  tft.fillTriangle(298,135,298,160,320,147.5,HX8357_BLUE);
  delay(200);
  // Return the button to its original color
  tft.fillTriangle(298,135,298,160,320,147.5,HX8357_WHITE);
}

void can2BaudrateHandle() {
  // Animate the button press by changing the color of the button
  tft.fillTriangle(298,235,298,260,320,247.5,HX8357_BLUE);
  delay(200);
  // Return the button to its original color
  tft.fillTriangle(298,235,298,260,320,247.5,HX8357_WHITE);
}

void runHandle() {
  // Animate the button press by changing the color of the button
  drawText(422,287,"Run",HX8357_BLUE);
  delay(200);
  // Return the button to its original color
  drawText(422,287,"Run",HX8357_BLACK);
}

Button quickStart(0, 70, 210, 320, quickStartHandle);
Button selfTest(0, 70, 100, 210, selfTestHandle);
Button advanced(0, 70, 0, 100, advancedSettingHandle);
Button can1BaudrateButton(200, 250, 100, 120, can1BaudrateHandle);
Button can2BaudrateButton(350, 400, 100, 120, can2BaudrateHandle);
Button run(420, 470, 0, 50, runHandle);

void setup() {
  // Initialize touch screen and draw main menu
  tft.begin();
  tft.setRotation(3);
  tft.setSPISpeed(19000000);
  Serial.begin(9600);
  drawMenu();

  Can1.begin();
  Can1.setBaudRate(BAUDRATE250K);
  Can1.setMaxMB(64);
  Can1.enableFIFO(true);                                            // Enable FIFO to allow for overflow between RX mailboxes
  Can1.enableFIFOInterrupt();
  Can1.onReceive(canSniff1);                                         // Interupt handler for counting and aknowledging all received CAN frames
  Can1.setMB(MB8, TX);

  Can2.begin();
  Can2.setBaudRate(BAUDRATE250K);
  Can2.setMaxMB(64);
  Can2.enableFIFO(true);                                            // Enable FIFO to allow for overflow between RX mailboxes
  Can2.enableFIFOInterrupt();
  Can2.onReceive(canSniff2);
  Can2.setMB(MB8, TX);
  
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

  /* pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(BLUE_LED_PIN, OUTPUT);
  pinMode(ORANGE_LED_PIN, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT); */
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

	//Serial.print("X = "); Serial.print(p.x);
	//Serial.print("\tY = "); Serial.print(p.y);
	//Serial.print("\tPressure = "); Serial.println(p.z);
  quickStart.pressedHandler(p.x, p.y);
  selfTest.pressedHandler(p.x, p.y);
  advanced.pressedHandler(p.x, p.y);
  can1BaudrateButton.pressedHandler(p.x, p.y);
  can2BaudrateButton.pressedHandler(p.x, p.y);
  run.pressedHandler(p.x, p.y);
}