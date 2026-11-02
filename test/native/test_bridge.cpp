#include "BridgeState.h"
#include <assert.h>
#include <stdio.h>

static void reply(BridgeState &b, uint8_t reg, uint16_t value, uint32_t at) {
  const uint8_t data[] = {reg, static_cast<uint8_t>(value), static_cast<uint8_t>(value >> 8)};
  b.receive(data, 3, at);
}
static BridgeState::Snapshot snap(const BridgeState &b, uint32_t at) {
  BridgeState::Snapshot s;
  b.snapshot(at, s);
  return s;
}
int main() {
  BridgeState b;
  auto s = snap(b, 0);
  assert(s.words[0x0100] == 0x5448 && s.words[0x0101] == 2);
  assert(s.words[0x0102] == 0 && s.words[0x0103] == 0);
  assert(s.words[0x0121] == 0 && s.words[0x0122] == 0xffff);
  // No bridge metadata may occupy Thermia's 8-bit address space.
  for (unsigned address = 0; address <= 0xff; ++address)
    assert(s.words[address] == 0);
  assert(BridgeState::kInputCount <= BridgeState::kControlStart);
  b.tick(0);
  assert(b.allowQuery() && b.requestByte() == 0); // register zero works
  const uint8_t query[] = {0xfe};
  b.receive(query, 1, 5);
  assert(!b.allowQuery() && b.requestByte() == 0);
  reply(b, 0, 0xff9c, 10); // preserve negative signed temperature's raw bits
  assert(!b.allowQuery() && b.requestByte() == 0xff);
  s = snap(b, 10);
  assert(s.words[0] == 0xff9c && s.words[0x0120] == 0xff9c);
  assert(s.words[0x0121] == 1 && s.words[0x0122] == 0);
  assert(s.words[0x0102] == 1 && s.words[0x0104] == 1);
  b.tick(259);
  assert(b.requestByte() == 0xff);
  b.tick(260);
  assert(b.requestByte() == 1 && b.allowQuery());
  reply(b, 0, 42, 261); // late reply must not complete register one
  assert(b.requestByte() == 1);
  b.tick(5260);
  assert(b.requestByte() == 0xff && snap(b, 5260).words[0x0105] == 1);
  b.tick(5510);
  assert(b.requestByte() == 2 && b.allowQuery());

  const uint8_t truncated[] = {2, 1};
  const uint8_t outOfRange[] = {16, 1, 2};
  const uint8_t tooLong[] = {2, 1, 2, 3};
  b.receive(truncated, 2, 5511);
  b.receive(outOfRange, 3, 5511);
  b.receive(tooLong, 4, 5511);
  b.receive(nullptr, 0, 5511);
  assert(snap(b, 5511).words[0x0106] == 3);
  assert(b.requestByte() == 2);
  s = snap(b, 120261);
  assert(s.words[0x0121] == 2 && (s.words[0x0102] & 1) == 0);
  reply(b, 0, 0, 120262); // a real zero is valid, unlike unseen zero-filled RAM
  assert(snap(b, 120262).words[0x0121] == 1);
  assert(snap(b, 120262).words[0] == 0);
  assert(snap(b, 70000000).words[0x0122] == 0xfffe); // age saturates
  b.tick(70000000);
  // Simulate another full millis cycle with no new value: old data never revives.
  b.tick(120263);
  assert(snap(b, 120263).words[0x0121] == 2);
  assert(snap(b, 120263).words[0x0122] == 0xfffe);

  BridgeState wrap;
  wrap.tick(0xfffffff0U);
  reply(wrap, 0, 123, 0xfffffff5U);
  wrap.tick(0x000000efU); // 250 ms since completion, across millis overflow
  assert(wrap.requestByte() == 1);
  assert(snap(wrap, 0x10).words[0x0121] == 1);
  wrap.tick(0x1477U); // exactly 5000 ms from register one's start
  assert(snap(wrap, 0x1477U).words[0x0105] == 1);

  BridgeState cycle;
  for (uint8_t reg = 0; reg < 16; ++reg) {
    const uint32_t at = reg * 300;
    cycle.tick(at);
    assert(cycle.requestByte() == reg);
    reply(cycle, reg, reg + 100, at + 1);
  }
  const auto full = snap(cycle, 4800);
  assert(full.words[0x0102] == 0xffff);
  for (unsigned reg = 0; reg < 16; ++reg) {
    assert(full.words[reg] == reg + 100);  // direct 1:1 mapping
    assert(full.words[0x0120 + reg * 3] == full.words[reg]);
  }
  for (unsigned address = 16; address <= 0xff; ++address)
    assert(full.words[address] == 0);  // reserved, never bridge data
  cycle.tick(4800);
  assert(cycle.requestByte() == 0);
  puts("Bridge protocol, polling, validation, freshness and rollover tests passed");
}
