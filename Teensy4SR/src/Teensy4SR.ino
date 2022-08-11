#include <FlexCAN_T4.h>
FlexCAN_T4<CAN1, RX_SIZE_256, TX_SIZE_16> Can1;
FlexCAN_T4<CAN2, RX_SIZE_256, TX_SIZE_16> Can2;
FlexCAN_T4FD<CAN3, RX_SIZE_256, TX_SIZE_16> Can3;
CANFD_timings_t config;

//Define message from FlexCAN library
static CANFD_message_t txmsg3;
static CAN_message_t txmsg1;
static CAN_message_t txmsg2;
static CAN_message_t rxmsg1;

uint32_t TXCount2 = 0;
uint32_t RXCount = 0;

uint32_t prevMsgId = -1;

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

elapsedMicros micro_counter;                                        // Set up elapsed micros to generate random bytes
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
  Can1.onReceive(canSniff);                                         // Interupt handler for counting and aknowledging all received CAN frames
  //Can1.enableLoopback(true);

  Can2.begin();
  Can2.setBaudRate(BAUDRATE250K);
  Can2.setMaxMB(16);
  Can2.enableFIFO(true);                                            // Enable FIFO to allow for overflow between RX mailboxes
  Can2.enableFIFOInterrupt();
  Can2.onReceive(canSniff);
  Can2.enableLoopBack(true);

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
  Serial.printf("Start(y)    Stop(n)\n");
}

void loop() {
  if (Serial.available()) {
    command = Serial.readStringUntil('\n');                         // Serial start and stop to be replaced by LCD menu w/ buttons
    if (command.equals("y")) {
      toggle = true;
    }
    else if (command.equals("n")) {
      delay(20);                                                    // Delay needed for accurate count of received frames
      Serial.printf("Messages Sent:%d\n", TXCount2);                // Prints out test results
      Serial.printf("Messages Received:%d\n", RXCount);
      resetTest();
    }
  }

  if (toggle == true) {
    txmsg2.id = TXCount2;
    u_counter.count = micro_counter;                                // Create random bytes using counter to fill msg buffer
    for (int i = 0; i < 4; i++) {
      txmsg2.buf[i] = u_counter.b[i];
    }
    if (Can2.getTXQueueCount() == 0) {
      Can2.write(txmsg2);                                           // Send CAN message once the previous message is done transmitting
      TXCount2++;
    }
    LED_BUILTIN_state = !LED_BUILTIN_state;
    digitalWrite(LED_BUILTIN, LED_BUILTIN_state);
  }
}

void canSniff(const CAN_message_t &rxmsg1) {                        // Function to increment on each received CAN frame
  RXCount++;
  if (rxmsg1.id - prevMsgId != 1) {
    Serial.printf("Test Failed: ID's did not match\n");
    Serial.printf("Previous Message ID: %d\n", prevMsgId);
    Serial.printf("Current Message ID: %d\n", rxmsg1.id);
    resetTest();
  } else {
    prevMsgId = rxmsg1.id;
  }
}

void resetTest() {
  toggle = false;
  RXCount = 0;
  TXCount2 = 0;
  prevMsgId = -1;
  digitalWrite(LED_BUILTIN, LOW);
}
