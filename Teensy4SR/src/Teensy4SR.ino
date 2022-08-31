#include <Arduino.h>
#include <FlexCAN_T4.h>
#include <SPI.h>
#include "Adafruit_GFX.h"
#include "Adafruit_HX8357.h"

FlexCAN_T4<CAN1, RX_SIZE_256, TX_SIZE_16> Can1;
FlexCAN_T4FD<CAN3, RX_SIZE_256, TX_SIZE_16> Canfd;
FlexCAN_T4<CAN2, RX_SIZE_256, TX_SIZE_16> Can2;
CANFD_timings_t config;

//Define message from FlexCAN library
static CAN_message_t txmsg1;
static CAN_message_t txmsg2;
static CAN_message_t rxmsg1;
static CAN_message_t rxmsg2;

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

String command;                                                     // Variable for serial commands - Remove when LCD menu is added
boolean toggle = false;
boolean enableCan1 = true;
boolean enableCan2 = false;

elapsedMillis blinkTimer;
elapsedMillis testTimer;

elapsedMicros micro_counter;                                        // Set up timer used to generate random bytes
union u_seconds {
  uint32_t count;
  byte b[4];
};

u_seconds u_counter;                                                // Declare variable using union

void setup() {
  Serial.begin(9600); delay(400);
  Can1.begin();
  Can1.setBaudRate(BAUDRATE250K);
  Can1.setMaxMB(16);
  Can1.enableFIFO(true);                                            // Enable FIFO to allow for overflow between RX mailboxes
  Can1.enableFIFOInterrupt();
  Can1.onReceive(canSniff1);                                         // Interupt handler for counting and aknowledging all received CAN frames
  //Can1.enableLoopBack(true);

  Can2.begin();
  Can2.setBaudRate(BAUDRATE250K);
  Can2.setMaxMB(16);
  Can2.enableFIFO(true);                                            // Enable FIFO to allow for overflow between RX mailboxes
  Can2.enableFIFOInterrupt();
  Can2.onReceive(canSniff2);
  //Can2.enableLoopBack(true);
  
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

  Serial.printf("Welcome to the CAN Bus Tester Device\n");
  Serial.printf("Start(y)   Stop(n)   SelfTest(t)\n");
}

void loop() {
  if (Serial.available()) {
    command = Serial.readStringUntil('\n');                         // Serial start and stop to be replaced by LCD menu w/ buttons
    if (command.equals("y")) {
      toggle = true;
    }
    else if (command.equals("n")) {
      delay(20);                                                    // Delay needed for accurate count of received frames
      if(enableCan1){
      Serial.printf("\nCan1:\n");
      Serial.printf("Messages Sent:%d\n", TXCount1);                // Prints out test results
      Serial.printf("Messages Received:%d\n", RXCount2);
      resetTest();
      }
      if(enableCan2){
      Serial.printf("\nCan2:\n");
      Serial.printf("Messages Sent:%d\n", TXCount2);                
      Serial.printf("Messages Received:%d\n", RXCount1);
      resetTest();
      }
    }
    else if (command.equals("t")) {
      enableCan1 = true;
      enableCan2 = true;
      runSelfTest();
      enableCan1 = false;
    }
  }

  if (toggle == true) {
    txmsg1.id = TXCount1;
    txmsg2.id = TXCount2;

    if ((Can1.getTXQueueCount() == 0) && enableCan1) {
      Can1.write(txmsg1);
      TXCount1++;
    }
    if ((Can2.getTXQueueCount() == 0) && enableCan2){
      Can2.write(txmsg2);                                           // Send CAN message once the previous message is done transmitting
      TXCount2++;
    }
    if(blinkTimer > 400){
    LED_BUILTIN_state = !LED_BUILTIN_state;
    digitalWrite(LED_BUILTIN, LED_BUILTIN_state);
    //Serial.printf("C1Queue: %d, C2Queue: %d\n",Can1.getTXQueueCount(),Can2.getTXQueueCount());
    blinkTimer = 0;
    }
  }
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


void runSelfTest() {
  Serial.printf("Starting Self Test\n");
  testTimer = 0;
  //Turn on bridging
  while(TXCount1 < 100) {
    if ((Can1.getTXQueueCount() == 0)) {
      txmsg1.id = TXCount1;
      Can1.write(txmsg1);
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
      Can2.write(txmsg2);                                           // Send CAN message once the previous message is done transmitting
      TXCount2++;
    } else if(testTimer > 400){
      break;
    }
  }
  delay(20);
  Serial.printf("\nSelf Test Completed\n");
  Serial.printf("Can1 messages sent: %d\n",TXCount1);
  Serial.printf("Received: %d\n",RXCount2);
  Serial.printf("Can2 messages sent: %d\n",TXCount2);
  Serial.printf("Received: %d\n",RXCount1);
  resetTest();
}

void resetTest() {
  toggle = false;
  RXCount1 = 0;
  RXCount2 = 0;
  TXCount1 = 0;
  TXCount2 = 0;
  prevMsgId1 = -1;
  prevMsgId2 = -1;
  digitalWrite(LED_BUILTIN, LOW);
  digitalWrite(GREEN_LED_PIN, LOW);
  digitalWrite(BLUE_LED_PIN, LOW);
  digitalWrite(ORANGE_LED_PIN, LOW);
}
