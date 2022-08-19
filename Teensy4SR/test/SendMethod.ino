#include <Arduino.h>
#include <FlexCAN_T4.h>
FlexCAN_T4<CAN1, RX_SIZE_256, TX_SIZE_16> can1;
FlexCAN_T4<CAN2, RX_SIZE_256, TX_SIZE_16> can2;

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

void setup() {
  Serial.begin(9600); delay(400);
  can1.begin();
  can1.setBaudRate(BAUDRATE250K);
  can1.setMaxMB(16);
  can1.enableFIFO(true);                                            // Enable FIFO to allow for overflow between RX mailboxes
  can1.enableFIFOInterrupt();
  can1.onReceive(canSniff1);                                         // Interupt handler for counting and aknowledging all received CAN frames
  //can1.enableLoopBack(true);

  can2.begin();
  can2.setBaudRate(BAUDRATE250K);
  can2.setMaxMB(16);
  can2.enableFIFO(true);                                            // Enable FIFO to allow for overflow between RX mailboxes
  can2.enableFIFOInterrupt();
  can2.onReceive(canSniff2);
  //can2.enableLoopBack(true);

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
  delay(1000);
  txmsg1.id = TXCount1;
  while(!can1.write(txmsg1));
  TXCount1++;


}

void canSniff1(const CAN_message_t &rxmsg1) {                        // Function to increment on each received CAN frame
  Serial.print("MB "); Serial.print(rxmsg1.mb);
  Serial.print(" OVERRUN: "); Serial.print(rxmsg1.flags.overrun);
  Serial.print(" ID: "); Serial.print(rxmsg1.id, HEX);
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
  Serial.print("MB "); Serial.print(rxmsg2.mb);
  Serial.print(" OVERRUN: "); Serial.print(rxmsg2.flags.overrun);
  Serial.print(" LEN: "); Serial.print(rxmsg2.len);
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



