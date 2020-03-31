#include <STM32LowPower.h>
#include <SPI.h>
#include "mcp25151.h"

const int pin = PB12;
struct can_frame canMsg;
MCP2515 mcp2515(PB13);
void sendCAN(uint32_t id, uint8_t length, uint8_t data0 = 0x00 , uint8_t data1 = 0x00 , uint8_t data2 = 0x00 , uint8_t data3 = 0x00 , uint8_t data4 = 0x00 , uint8_t data5 = 0x00 , uint8_t data6 = 0x00 , uint8_t data7 = 0x00);

long lastInt;
bool wasSleeping = false;
bool ignitionState = false;
bool lightState = false;
bool reverseState = false;
float battery = 0.0;
bool received = false;

void setup() {
  pinMode(pin, INPUT_PULLUP);
  pinMode(PC13, OUTPUT);
  digitalWrite(PC13, LOW);
  pinMode(PA0, OUTPUT);
  digitalWrite(PA0, HIGH);
  pinMode(PA1, OUTPUT);
  digitalWrite(PA1, HIGH);
  pinMode(PA2, OUTPUT);
  digitalWrite(PA2, HIGH);

  pinMode(PA5, OUTPUT);
  digitalWrite(PA5, LOW);
  pinMode(PA6, OUTPUT);
  digitalWrite(PA6, LOW);
  pinMode(PA7, OUTPUT);
  digitalWrite(PA7, LOW);

  Serial.begin(115200);
  SPI.setMOSI(PB5);
  SPI.setMISO(PB4);
  SPI.setSCLK(PB3);
  delay(2000);
  Serial.println("Reset: " + String(mcp2515.reset()));
  mcp2515.setPin(0b00001000);//turnRXbuf1 pin to GND
  Serial.println("SetBitrate: " + String(mcp2515.setBitrate(CAN_100KBPS, MCP_8MHZ)));
  Serial.println("SetMode: " + String(mcp2515.setNormalMode()));
  Serial.println("VW CAN Interface");

  LowPower.begin();
  LowPower.attachInterruptWakeup(pin, canInt, FALLING);
}

void loop() {
  if (received) {
    received = false;
    lastInt = millis();
  }
  if (millis() - lastInt > 1000)digitalWrite(PC13, HIGH);
  if (wasSleeping) {
    wasSleeping = false;
    mcp2515.setConfigMode();
    delay(5);
    mcp2515.setPin(0b00001000);//turnRXbuf1 pin to GND
    delay(5);
    Serial.println("SetMode: " + String(mcp2515.setNormalMode()));
    delay(100);
  }
  if (mcp2515.readMessage(&canMsg) == MCP2515::ERROR_OK) {
    //Serial.println(millis()-lastInt);
    digitalWrite(PC13, !digitalRead(PC13));
    uint8_t interrupt = mcp2515.getInterrupts();
    Serial.println("GetInt: " + String(interrupt));
    mcp2515.clearInterrupts();
    lastInt = millis();
    if (interrupt == 0x00) {
      Serial.print(mcp2515.getErrorFlags(), HEX);
      Serial.print("  ");
      Serial.print(canMsg.can_id, HEX);
      Serial.print("  ");
      Serial.print(canMsg.can_dlc, HEX);
      Serial.print("  ");

      for (int i = 0; i < canMsg.can_dlc; i++)  { // print the data

        Serial.print(canMsg.data[i], HEX);
        Serial.print(" ");

      }
      Serial.println();
      filterCanMsg(canMsg);
    } else {
      Serial.println("SomeErrorFromCAN");
    }
  }
  if ((millis() - lastInt) > 10000) {
    Serial.print("SLEEPING Because: ");
    Serial.println(millis() - lastInt);

    digitalWrite(PC13, HIGH);
    digitalWrite(PA0, LOW);
    digitalWrite(PA1, LOW);
    digitalWrite(PA2, LOW);
    digitalWrite(PA6, LOW);
    digitalWrite(PA5, LOW);
    digitalWrite(PA7, LOW);
    mcp2515.clearInterrupts();
    delay(5);
    mcp2515.setConfigMode();
    delay(5);
    mcp2515.setPin(0b00101000);//turnRXbuf1 pin to VCC
    delay(5);
    mcp2515.setNormalMode();
    delay(500);
    Serial.println("SetMode: " + String(mcp2515.setSleepMode()));
    wasSleeping = true;
    delay(100);
    LowPower.deepSleep();
    digitalWrite(PA0, HIGH);
    digitalWrite(PA1, HIGH);
    digitalWrite(PA2, HIGH);
    digitalWrite(PA6, ignitionState);
    digitalWrite(PA5, reverseState);
    digitalWrite(PA7, lightState);
  }
}


void filterCanMsg(can_frame msg) {
  if (msg.can_id == 0x271) {
    ignitionState = (canMsg.data[0] & 0x01);
    Serial.println("KEY: " + String(ignitionState));
    digitalWrite(PA6, ignitionState);
    digitalWrite(PA1, ignitionState);
  } else if (msg.can_id == 0x351) {
    reverseState = (canMsg.data[0] & 0x02);
    Serial.println("REVERSE: " + String(reverseState));
    digitalWrite(PA5, reverseState);
  } else if (msg.can_id == 0x635) {
    lightState = (canMsg.data[0] & 0x5C);
    Serial.println("LIGHT: " + String(lightState));
    digitalWrite(PA7, lightState);
  } else if (msg.can_id == 0x571) {
    battery = (canMsg.data[0] * 0.05) + 5;
    Serial.println("BATTERY: " + String(battery) + "V");
  }
}

void sendCAN(uint32_t id, uint8_t length, uint8_t data0 , uint8_t data1 , uint8_t data2 , uint8_t data3 , uint8_t data4 , uint8_t data5 , uint8_t data6 , uint8_t data7) {
  struct can_frame canMsg1;
  canMsg1.can_id  = id;
  canMsg1.can_dlc = length;
  canMsg1.data[0] = data0;
  canMsg1.data[1] = data1;
  canMsg1.data[2] = data2;
  canMsg1.data[3] = data3;
  canMsg1.data[4] = data4;
  canMsg1.data[5] = data5;
  canMsg1.data[6] = data6;
  canMsg1.data[7] = data7;
  mcp2515.sendMessage(MCP2515::TXB1, &canMsg1);
}

void canInt() {
  digitalWrite(PA2, !digitalRead(PA2));
  received = true;
}
