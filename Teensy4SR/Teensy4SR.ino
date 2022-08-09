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

String command;
boolean toggle = false;

elapsedMicros micro_counter;

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
  Can1.enableFIFO(true);
  Can1.enableFIFOInterrupt();
  Can1.onReceive(canSniff); // allows FIFO messages to be received in the supplied callback.
  Can2.begin();
  Can2.setBaudRate(BAUDRATE250K);

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
    command = Serial.readStringUntil('\n');
    if (command.equals("start")) {
      toggle = true;
    }
    else if (command.equals("stop")) {
      digitalWrite(LED_BUILTIN, LOW);
      toggle = false;
      delay(10);
      Serial.printf("Messages Sent:%d\n", TXCount2);
      Serial.printf("Messages Received:%d\n", RXCount1);
      RXCount1 = 0;
      TXCount2 = 0;
    }
  }

  if (toggle == true) {
    
    u_counter.count = micro_counter;
    for (int i = 0; i < 4; i++) {
      txmsg2.buf[i] = u_counter.b[i];
    }
    if (Can2.getTXQueueCount() == 0) {
      txmsg2.id = TXCount2;
      Can2.write(txmsg2);
      TXCount2++;
    }
    LED_BUILTIN_state = !LED_BUILTIN_state;
    digitalWrite(LED_BUILTIN, LED_BUILTIN_state);
  }
}

void canSniff(const CAN_message_t &rxmsg1) {
  RXCount1++;
}
