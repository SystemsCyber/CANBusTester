#include <Arduino.h>
#include <FlexCAN_T4.h>
FlexCAN_T4<CAN3, RX_SIZE_256, TX_SIZE_16> Can1;
FlexCAN_T4FD<CAN1, RX_SIZE_256, TX_SIZE_16> Canfd;
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
#define GREEN_LED_PIN 15
#define RED_LED_PIN 14
#define BLUE_LED_PIN 13
boolean GREEN_LED_state;
boolean RED_LED_state;
boolean LED_BUILTIN_state;

//Define default baudrate
#define BAUDRATE250K 250000
#define BAUDRATE500K 500000

String command;                                                     // Variable for serial commands - Remove when LCD menu is added
boolean toggle = false;
boolean enableCan1 = false;
boolean enableCan2 = true;
boolean enableCan3 = false;

elapsedMillis blinkTimer;

elapsedMicros micro_counter;                                        // Set up timer used to generate random bytes
union u_seconds {
  uint32_t count;
  byte b[4];
};

                                                                    // Declare the variable using the union
u_seconds u_counter;

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

  //Set message extension, ID, and length
  txmsg1.flags.extended = 1;
  txmsg1.len = 8;

  txmsg2.flags.extended = 1;
  txmsg2.len = 8;

  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  Serial.printf("Welcome to the CAN Bus Tester Device\n");
  Serial.printf("Start(y)   Stop(n)   SelfTest(t)\n\n");
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
      Serial.printf("Can1:\n");
      Serial.printf("Messages Sent:%d\n", TXCount1);                // Prints out test results
      Serial.printf("Messages Received:%d\n\n", RXCount2);
      resetTest();
      }
      if(enableCan2){
      Serial.printf("Can2:\n");
      Serial.printf("Messages Sent:%d\n", TXCount2);                
      Serial.printf("Messages Received:%d\n\n", RXCount1);
      resetTest();
      }
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
  RXCount1++;
  if (rxmsg1.id - prevMsgId1 != 1) {
    Serial.printf("Can 1 Test Failed: ID's did not match\n");
    Serial.printf("Previous Message ID: %d\n", prevMsgId1);
    Serial.printf("Current Message ID: %d\n", rxmsg1.id);
    resetTest();
    } else {
    prevMsgId1 = rxmsg1.id;
    }
}

void canSniff2(const CAN_message_t &rxmsg2) {                        // Function to increment on each received CAN frame
  RXCount2++;
  if (rxmsg2.id - prevMsgId2 != 1) {
    Serial.printf("Can 2 Test Failed: ID's did not match\n");
    Serial.printf("Previous Message ID: %d\n", prevMsgId2);
    Serial.printf("Current Message ID: %d\n", rxmsg2.id);
    resetTest();
    } else {
    prevMsgId2 = rxmsg2.id;
    }
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
}
