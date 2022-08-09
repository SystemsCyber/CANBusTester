#include <FlexCAN_T4.h>
FlexCAN_T4<CAN2, RX_SIZE_256, TX_SIZE_16> Can2;
FlexCAN_T4<CAN1, RX_SIZE_256, TX_SIZE_16> Can1;

//Define message from FlexCAN library
static CAN_message_t txmsg2;
static CAN_message_t rxmsg1;

uint32_t TXCount2 = 0;
uint32_t RXCount1 = 0;

//Define LED
#define GREEN_LED_PIN 15
#define RED_LED_PIN 14
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
  Can2.begin();
  Can2.setBaudRate(BAUDRATE250K);
  Can2.onTransmit(canSend);                                         // Interupt handler for counting transmitted CAN frames

  //Set message extension, ID, and length
  txmsg2.flags.extended = 1;
  txmsg2.len = 8;

  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
}

void loop() {
  if (Serial.available()) {
    command = Serial.readStringUntil('\n');                         // Serial start and stop to be replaced by LCD menu w/ buttons
    if (command.equals("start")) {
      toggle = true;
    }
    else if (command.equals("stop")) {
      digitalWrite(LED_BUILTIN, LOW);
      toggle = false;
      delay(10);                                                    // Delay needed for accurate count of received frames
      Serial.printf("Messages Sent:%d\n", TXCount2);                // Prints out test results
      Serial.printf("Messages Received:%d\n", RXCount1);
      RXCount1 = 0;                                                 // Reset Count for next test
      TXCount2 = 0;
    }
  }

  if (toggle == true) {
    u_counter.count = micro_counter;                                // Create random bytes to fill msg buffer
    for (int i = 0; i < 4; i++) {
      txmsg2.buf[i] = u_counter.b[i];
    }
    txmsg2.id = TXCount2;
    while(!Can2.write(txmsg2));                                     // Send CAN message once the previous message is done transmitting
    LED_BUILTIN_state = !LED_BUILTIN_state;
    digitalWrite(LED_BUILTIN, LED_BUILTIN_state);
  }
}

void canSniff(const CAN_message_t &rxmsg1) {                        // function to increment on each received CAN frame
  RXCount1++;
}
void canSend(const CAN_message_t &txmsg2) {                         // function to increment on each sent CAN frame
  TXCount2++;
}
