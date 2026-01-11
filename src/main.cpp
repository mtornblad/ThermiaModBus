#include <Arduino.h>
#include <ThermiaWire.h>
#include <ArduinoRS485.h> 
#include <ArduinoModbus.h>

static uint8_t reg = 0;
const int numCoils = 11;
const int numDiscreteInputs = 10;
const int numHoldingRegisters = 10;
const int numInputRegisters = 10;

void onModbusRequest(int slave, int function, int address, int quantity) {
  Serial.print("Request: Func=");
  Serial.print(function);
  Serial.print(" Addr=");
  Serial.print(address);
  Serial.print(" Qty=");
  Serial.println(quantity);
}


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

  // start the Modbus RTU server, with (slave) id 42
  if (!ModbusRTUServer.begin(42, 9600)) {
    Serial.println("Failed to start Modbus RTU Server!");
    while (1);
  } else {
    Serial.println("Started Modbus RTU Server!");
  }

  ModbusRTUServer.onRequest(onModbusRequest);
  ModbusRTUServer.configureCoils(0x00, numCoils);
  ModbusRTUServer.configureDiscreteInputs(0x00, numDiscreteInputs);
  ModbusRTUServer.configureHoldingRegisters(0x00, numHoldingRegisters);
  ModbusRTUServer.configureInputRegisters(0x00, numInputRegisters);

  ThermiaWire.thermiaNack(true);
  ThermiaWire.begin(0x2E);                 // initialize slave
  ThermiaWire.onRequest(transmitDataWire); // register transmitDataWire as the handler
  ThermiaWire.onReceive(receiveDataWire);  // give the Wire library the name of the function
}

void loop() {
  ModbusRTUServer.poll();

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

