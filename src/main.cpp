#include <Arduino.h>
#include <ArduinoRS485.h>
#include <ArduinoModbus.h>
#include <ThermiaWire.h>
#include <util/atomic.h>

#include "BridgeState.h"

constexpr uint8_t kI2cAddress = 0x2e;
constexpr uint8_t kModbusAddress = 42;
constexpr unsigned long kModbusBaud = 9600;

// Direct 3.3 V UART to XIAO ESP32-C6 on the same board: no DE or /RE GPIO.
// ArduinoModbus uses the RS485Class transport API even without transceivers.
RS485Class bridgeSerial(Serial1, PIN_PC0, -1, -1);
BridgeState bridge;
BridgeState::Snapshot currentSnapshot;

void transmitDataWire() {
  ThermiaWire.write(bridge.requestByte());
}

void receiveDataWire(int numBytes) {
  uint8_t bytes[3] = {};
  uint8_t length = 0;
  while (ThermiaWire.available()) {
    const uint8_t value = ThermiaWire.read();
    if (length < sizeof(bytes)) bytes[length] = value;
    if (length < 255) ++length;
  }
  if (numBytes != length) length = 255;
  bridge.receive(bytes, length, millis());
  ThermiaWire.thermiaNack(!bridge.allowQuery());
  // No Serial output, allocation or Modbus access in these interrupt callbacks.
}

void updateRegisters() {
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    const uint32_t now = millis();
    bridge.tick(now);
    ThermiaWire.thermiaNack(!bridge.allowQuery());
    bridge.snapshot(now, currentSnapshot);
  }
  ModbusRTUServer.writeInputRegisters(0, currentSnapshot.words,
                                     BridgeState::kInputCount);
}

void setup() {
  Serial.begin(115200);
  Serial.println(F("Thermia 901510 bridge: starting (read only)"));

  if (!ModbusRTUServer.begin(bridgeSerial, kModbusAddress, kModbusBaud, SERIAL_8N1) ||
      ModbusRTUServer.configureInputRegisters(0, BridgeState::kInputCount) != 1) {
    Serial.println(F("Modbus initialization failed"));
    while (true) delay(1000);
  }
  // No coils or holding registers: pump writes are not implemented yet.
  // Register callbacks before enabling the peripheral and its interrupts.
  ThermiaWire.onRequest(transmitDataWire);
  ThermiaWire.onReceive(receiveDataWire);
  ThermiaWire.thermiaNack(true);
  updateRegisters();
  ThermiaWire.begin(kI2cAddress);
  Serial.println(F("I2C 0x2E; Modbus 42, 9600 8N1; automatic reads 0x00..0x0F"));
}

void loop() {
  updateRegisters();
  ModbusRTUServer.poll();
}
