#include "sentinel/bounded_job.h"
#include "sentinel/governance.h"

#include <cstdint>
#include <iostream>

namespace {
int failures = 0;

#define CHECK(expr)                                                                                                    \
    do {                                                                                                               \
        if (!(expr)) {                                                                                                 \
            std::cerr << __FILE__ << ':' << __LINE__ << ": CHECK failed: " #expr << '\n';                           \
            ++failures;                                                                                                \
        }                                                                                                              \
    } while (false)

using sentinel::AuthorityGrant;
using sentinel::BoundedJob;
using sentinel::BudgetState;
using sentinel::CapabilityClass;
using sentinel::DecisionReason;
using sentinel::ExecutionLimits;
using sentinel::JobRequest;

JobRequest request(const char *capability, ExecutionLimits limits) {
    return {"job-test-001", capability, limits};
}

void test_policy_fail_closed() {
    const AuthorityGrant no_authority{false, false};
    const AuthorityGrant all_authority{true, true};

    auto unknown = sentinel::evaluatePolicy(request("sentinel.unknown", {1000, 1024, false}), no_authority);
    CHECK(!unknown.allowed);
    CHECK(unknown.reason == DecisionReason::UnknownCapability);

    const JobRequest empty_job{"", "sentinel.device.status", {1000, 1024, false}};
    auto empty = sentinel::evaluatePolicy(empty_job, no_authority);
    CHECK(!empty.allowed);

    auto observe = sentinel::evaluatePolicy(request("sentinel.rf.observe", {10000, 4096, false}), no_authority);
    CHECK(observe.allowed);
    CHECK(observe.reason == DecisionReason::Allowed);

    auto observe_tx = sentinel::evaluatePolicy(request("sentinel.rf.observe", {10000, 4096, true}), all_authority);
    CHECK(!observe_tx.allowed);
    CHECK(
        observe_tx.reason == DecisionReason::InvalidLimits ||
        observe_tx.reason == DecisionReason::ScopeViolation
    );

    auto too_long = sentinel::evaluatePolicy(request("sentinel.rf.observe", {15001, 4096, false}), no_authority);
    CHECK(!too_long.allowed);
    CHECK(too_long.reason == DecisionReason::InvalidLimits);

    auto too_large = sentinel::evaluatePolicy(request("sentinel.rf.observe", {10000, 65537, false}), no_authority);
    CHECK(!too_large.allowed);
    CHECK(too_large.reason == DecisionReason::InvalidLimits);

    for (const char *name : {"bruce.wifi.deauth", "bruce.rf.jam", "bruce.hid.execute"}) {
        auto denied = sentinel::evaluatePolicy(request(name, {1000, 1024, false}), all_authority);
        CHECK(!denied.allowed);
        CHECK(denied.reason == DecisionReason::ExcludedCapability);
    }

    auto gpio = sentinel::evaluatePolicy(request("sentinel.gpio.set", {1000, 1024, false}), all_authority);
    CHECK(!gpio.allowed);
    CHECK(gpio.reason == DecisionReason::AgentExposureDenied);

    auto ir = sentinel::evaluatePolicy(request("sentinel.ir.transmit", {1000, 1024, true}), all_authority);
    CHECK(!ir.allowed);
    CHECK(ir.reason == DecisionReason::AgentExposureDenied);
}

void test_registry_invariants() {
    CHECK(sentinel::capabilityCount() > 0);
    CHECK(sentinel::capabilityAt(sentinel::capabilityCount()) == nullptr);

    for (size_t i = 0; i < sentinel::capabilityCount(); ++i) {
        const auto *capability = sentinel::capabilityAt(i);
        CHECK(capability != nullptr);
        if (capability == nullptr) continue;

        if (capability->name == nullptr) {
            std::cerr << __FILE__ << ':' << __LINE__ << ": capability name is null\n";
            ++failures;
            continue;
        }
        CHECK(capability->name[0] != '\0');

        if (capability->agent_exposed) {
            CHECK(
                capability->capability_class == CapabilityClass::Introspection ||
                capability->capability_class == CapabilityClass::Observe
            );
            CHECK(!capability->ceiling.transmit);
            CHECK(capability->ceiling.max_runtime_ms > 0);
            CHECK(capability->ceiling.max_output_bytes > 0);
        }

        if (capability->capability_class == CapabilityClass::Excluded) {
            CHECK(!capability->agent_exposed);
        }
    }
}

void test_bounded_job() {
    BoundedJob invalid_runtime(0, 100);
    CHECK(!invalid_runtime.begin(1));

    BoundedJob invalid_output(100, 0);
    CHECK(!invalid_output.begin(1));

    BoundedJob output_job(1000, 10);
    CHECK(output_job.begin(100));
    CHECK(!output_job.begin(101));
    CHECK(output_job.accountOutput(4));
    CHECK(output_job.outputBytes() == 4);
    CHECK(output_job.accountOutput(6));
    CHECK(output_job.outputBytes() == 10);
    CHECK(!output_job.accountOutput(1));
    CHECK(output_job.state() == BudgetState::OutputExceeded);
    CHECK(output_job.shouldStop(200));

    BoundedJob runtime_job(100, 100);
    CHECK(runtime_job.begin(1000));
    CHECK(!runtime_job.shouldStop(1099));
    CHECK(runtime_job.shouldStop(1100));
    CHECK(runtime_job.state() == BudgetState::RuntimeExceeded);

    BoundedJob completed(100, 100);
    CHECK(completed.begin(50));
    completed.complete();
    CHECK(completed.state() == BudgetState::Completed);
    CHECK(completed.shouldStop(51));
    CHECK(!completed.accountOutput(1));

    BoundedJob cancelled(100, 100);
    CHECK(cancelled.begin(50));
    cancelled.cancel();
    CHECK(cancelled.state() == BudgetState::Cancelled);
    CHECK(cancelled.shouldStop(51));

    BoundedJob wraparound(50, 100);
    constexpr uint32_t near_wrap = UINT32_MAX - 15U;
    CHECK(wraparound.begin(near_wrap));
    CHECK(wraparound.elapsedMs(16U) == 32U);
    CHECK(!wraparound.shouldStop(16U));
    CHECK(wraparound.shouldStop(48U));
    CHECK(wraparound.state() == BudgetState::RuntimeExceeded);
}
} // namespace

int main() {
    test_policy_fail_closed();
    test_registry_invariants();
    test_bounded_job();

    if (failures != 0) {
        std::cerr << failures << " Sentinel test(s) failed\n";
        return 1;
    }

    std::cout << "Sentinel host tests passed\n";
    return 0;
}
