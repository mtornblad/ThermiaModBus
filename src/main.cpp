#include <Arduino.h>
#include <ThermiaWire.h>

static uint8_t reg = 0;


void transmitDataWire() {
      ThermiaWire.write(reg);
}


void receiveDataWire(int numBytes) {      // the Wire library tells us how many bytes
  for (int i = 0; i < numBytes; i++) {    // were received so we can for loop for that
    uint8_t c = ThermiaWire.read();                     // amount and read the received data
    if ((c == 0xFE) && i == 0) {
      ThermiaWire.thermiaNack(true);
    }
  }
}


void setup() {
  Serial.begin(115200);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  Serial.println("CHIP started...");

  ThermiaWire.thermiaNack(true);
  ThermiaWire.begin(0x2E);                 // initialize slave
  
  ThermiaWire.onRequest(transmitDataWire); // register transmitDataWire as the handler
  ThermiaWire.onReceive(receiveDataWire);  // give the Wire library the name of the function
}

void loop() {
  while (Serial.available()) {
    int number = Serial.parseInt();
    if (number != 0) {
      Serial.print("You entered: ");
      Serial.println(number);
      reg = (uint8_t) number;
      ThermiaWire.thermiaNack(false);
    }
  }
}

