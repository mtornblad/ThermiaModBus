#pragma once

#include <stdint.h>

// No Arduino dependencies: the protocol/scheduler can be tested on a host.
// Calls from loop() must be protected against the TWI ISR with ATOMIC_BLOCK.
class BridgeState {
 public:
  static constexpr uint8_t kRegisterCount = 16;
  static constexpr uint16_t kIdle = 0xffff;
  static constexpr uint32_t kPollGapMs = 250;
  static constexpr uint32_t kReplyTimeoutMs = 5000;
  static constexpr uint32_t kStaleMs = 120000;
  static constexpr uint16_t kSnapshotStart = 0x20;
  static constexpr uint16_t kInputCount = kSnapshotStart + kRegisterCount * 3;

  struct Snapshot { uint16_t words[kInputCount]; };

  void tick(uint32_t now) {
    for (uint8_t reg = 0; reg < kRegisterCount; ++reg) {
      const uint16_t bit = static_cast<uint16_t>(1U << reg);
      if ((seen_ & bit) && elapsed(now, receivedAt_[reg]) >= kStaleMs)
        expired_ |= bit;
      if ((seen_ & bit) && elapsed(now, receivedAt_[reg]) >= 65534000UL)
        ageSaturated_ |= bit;
    }
    if (pending_ && elapsed(now, started_) >= kReplyTimeoutMs) {
      ++timeouts_;
      pending_ = false;
      finished_ = now;
    }
    if (!pending_ && (!startedOnce_ || elapsed(now, finished_) >= kPollGapMs)) {
      requested_ = next_;
      next_ = (next_ + 1) % kRegisterCount;
      pending_ = true;
      queryAccepted_ = false;
      startedOnce_ = true;
      started_ = now;
    }
  }

  bool allowQuery() const { return pending_ && !queryAccepted_; }
  uint8_t requestByte() const { return pending_ ? requested_ : 0xff; }

  void receive(const uint8_t *bytes, uint8_t length, uint32_t now) {
    // ThermiaWire may call onReceive(0) after it NACKs an idle 0xFE.
    if (length == 0) return;
    if (length == 1 && (bytes[0] == 0xfd || bytes[0] == 0xfe)) {
      if (bytes[0] == 0xfe) {
        ++queries_;
        queryAccepted_ = true;
      }
      return;
    }
    if (length != 3 || bytes[0] >= kRegisterCount) {
      ++malformed_;
      return;
    }
    const uint8_t reg = bytes[0];
    values_[reg] = static_cast<uint16_t>(bytes[1]) |
                   (static_cast<uint16_t>(bytes[2]) << 8);
    receivedAt_[reg] = now;
    seen_ |= static_cast<uint16_t>(1U << reg);
    expired_ &= static_cast<uint16_t>(~(1U << reg));
    ageSaturated_ &= static_cast<uint16_t>(~(1U << reg));
    ++responses_;
    // A delayed/unsolicited reply must not complete a different pending register.
    if (pending_ && requested_ == reg) {
      pending_ = false;
      finished_ = now;
    }
  }

  void snapshot(uint32_t now, Snapshot &out) const {
    for (uint16_t i = 0; i < kInputCount; ++i) out.words[i] = 0;
    uint16_t fresh = 0;
    for (uint8_t reg = 0; reg < kRegisterCount; ++reg) {
      const bool seen = (seen_ & (1U << reg)) != 0;
      const uint32_t age = elapsed(now, receivedAt_[reg]);
      const bool valid = seen && !(expired_ & (1U << reg)) && age < kStaleMs;
      if (valid) fresh |= static_cast<uint16_t>(1U << reg);
      out.words[reg] = values_[reg];  // preserve the original raw register map
      const uint16_t base = kSnapshotStart + reg * 3;
      out.words[base] = values_[reg];
      out.words[base + 1] = !seen ? 0 : (valid ? 1 : 2);
      out.words[base + 2] = !seen ? 0xffff :
          ((ageSaturated_ & (1U << reg)) || age / 1000 > 0xfffe ?
           0xfffe : static_cast<uint16_t>(age / 1000));
    }
    out.words[0x10] = 0x5448;  // "TH", bridge identification
    out.words[0x11] = 1;       // register map version
    out.words[0x12] = fresh;
    out.words[0x13] = seen_;
    out.words[0x14] = responses_;
    out.words[0x15] = timeouts_;
    out.words[0x16] = malformed_;
    out.words[0x17] = queries_;
    out.words[0x18] = pending_ ? requested_ : kIdle;
  }

 private:
  static uint32_t elapsed(uint32_t now, uint32_t then) { return now - then; }
  // Shared with the ISR. loop() holds an atomic guard when accessing this state.
  volatile uint16_t values_[kRegisterCount] = {};
  volatile uint32_t receivedAt_[kRegisterCount] = {};
  volatile uint16_t seen_ = 0;
  volatile uint16_t expired_ = 0, ageSaturated_ = 0;
  volatile uint16_t responses_ = 0, timeouts_ = 0, malformed_ = 0, queries_ = 0;
  volatile bool pending_ = false, queryAccepted_ = false;
  volatile uint8_t requested_ = 0;
  volatile uint32_t finished_ = 0;
  uint8_t next_ = 0;
  bool startedOnce_ = false;
  uint32_t started_ = 0;
};
