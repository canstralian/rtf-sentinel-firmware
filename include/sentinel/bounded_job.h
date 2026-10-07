// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once

#include <stdint.h>

namespace sentinel {

enum class BudgetState : uint8_t {
    Ready = 0,
    Running,
    Completed,
    RuntimeExceeded,
    OutputExceeded,
    Cancelled,
};

class BoundedJob {
  public:
    BoundedJob(uint32_t max_runtime_ms, uint32_t max_output_bytes);

    bool begin(uint32_t now_ms);
    bool accountOutput(uint32_t bytes);
    bool shouldStop(uint32_t now_ms);
    void complete();
    void cancel();

    BudgetState state() const;
    uint32_t outputBytes() const;
    uint32_t elapsedMs(uint32_t now_ms) const;

  private:
    uint32_t _max_runtime_ms;
    uint32_t _max_output_bytes;
    uint32_t _started_ms;
    uint32_t _output_bytes;
    BudgetState _state;
};

} // namespace sentinel
