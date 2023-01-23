#include <Adafruit_HX8357.h>
#include <Adafruit_GFX.h>
#include <TouchScreen.h>
#include <SPI.h>

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

void setup() {
  // Initialize touch screen and draw main menu
  tft.begin();
  tft.setRotation(3);
  tft.setSPISpeed(19000000);
  Serial.begin(115200);
  drawMenu();
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
  tft.startWrite();
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

  tft.endWrite();
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