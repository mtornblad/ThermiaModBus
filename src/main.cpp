#include <Arduino.h>
#include <ThermiaWire.h>
#include <ArduinoRS485.h> 
#include <ArduinoModbus.h>

#define I2C_ADDRESS 0x2E
#define MODBUS_ADDRESS 42
#define MODBUS_BAUD 9600

// #define I2C_ADDRESS 0x2E

static uint8_t pullReg = 0;
const int numCoils = 10;
const int numDiscreteInputs = 10;
const int numHoldingRegisters = 20;
const int numInputRegisters = 20;

void onModbusRequest(int slave, int function, int address, int quantity) {
  Serial.print("Request: Func=");
  Serial.print(function);
  Serial.print(" Addr=");
  Serial.print(address);
  Serial.print(" Qty=");
  Serial.println(quantity);
}


void transmitDataWire() {
      ThermiaWire.write(pullReg);
}


void receiveDataWire(int numBytes) {
  uint8_t reg = ThermiaWire.read();         // read first byte, should be register or FE
  if ((reg == 0xFE) && numBytes == 1) {
    ThermiaWire.thermiaNack(true);
  } else if (reg <= 0x0F && numBytes == 3) {               // Check if we got register value, what is highetst reg?
    uint8_t low = ThermiaWire.read();         // read first byte, should be register or FE
    uint8_t high = ThermiaWire.read();         // read first byte, should be register or FE
    uint16_t value = (static_cast<uint16_t>(high) << 8) | low;

    Serial.print("I2C write: reg=");
    Serial.print(reg);
    Serial.print(" high=");
    Serial.print(high);
    Serial.print(" low=");
    Serial.print(low);
    Serial.print(" value=");
    Serial.println(value);
    ModbusRTUServer.inputRegisterWrite(reg, value);
  }
}


void setup() {
  Serial.begin(115200);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  Serial.println("CHIP started...");

  if (!ModbusRTUServer.begin(MODBUS_ADDRESS, MODBUS_BAUD)) {
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
  ThermiaWire.begin(I2C_ADDRESS);                 // initialize slave
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
      pullReg = (uint8_t) number;
      ThermiaWire.thermiaNack(false);
    }
  }
}

