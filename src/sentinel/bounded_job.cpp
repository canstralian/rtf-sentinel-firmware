// SPDX-License-Identifier: AGPL-3.0-or-later

#include "sentinel/bounded_job.h"

namespace sentinel {

BoundedJob::BoundedJob(uint32_t max_runtime_ms, uint32_t max_output_bytes)
    : _max_runtime_ms(max_runtime_ms),
      _max_output_bytes(max_output_bytes),
      _started_ms(0),
      _output_bytes(0),
      _state(BudgetState::Ready) {}

bool BoundedJob::begin(uint32_t now_ms) {
    if (_state != BudgetState::Ready || _max_runtime_ms == 0 || _max_output_bytes == 0) return false;

    _started_ms = now_ms;
    _output_bytes = 0;
    _state = BudgetState::Running;
    return true;
}

bool BoundedJob::accountOutput(uint32_t bytes) {
    if (_state != BudgetState::Running) return false;

    if (bytes > _max_output_bytes - _output_bytes) {
        _output_bytes = _max_output_bytes;
        _state = BudgetState::OutputExceeded;
        return false;
    }

    _output_bytes += bytes;
    return true;
}

bool BoundedJob::shouldStop(uint32_t now_ms) {
    if (_state != BudgetState::Running) return true;

    // Unsigned subtraction intentionally handles millis()-style wraparound.
    if (now_ms - _started_ms >= _max_runtime_ms) {
        _state = BudgetState::RuntimeExceeded;
        return true;
    }

    return false;
}

void BoundedJob::complete() {
    if (_state == BudgetState::Running) _state = BudgetState::Completed;
}

void BoundedJob::cancel() {
    if (_state == BudgetState::Ready || _state == BudgetState::Running) _state = BudgetState::Cancelled;
}

BudgetState BoundedJob::state() const { return _state; }

uint32_t BoundedJob::outputBytes() const { return _output_bytes; }

uint32_t BoundedJob::elapsedMs(uint32_t now_ms) const {
    if (_state == BudgetState::Ready) return 0;
    return now_ms - _started_ms;
}

} // namespace sentinel
