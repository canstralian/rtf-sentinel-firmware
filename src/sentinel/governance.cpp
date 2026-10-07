// SPDX-License-Identifier: AGPL-3.0-or-later

#include "sentinel/governance.h"

#include <cstring>

namespace sentinel {
namespace {

constexpr CapabilityDescriptor kCapabilities[] = {
    {"sentinel.device.status", CapabilityClass::Introspection, true, {2000, 4096, false}},
    {"sentinel.device.capabilities", CapabilityClass::Introspection, true, {2000, 8192, false}},
    {"sentinel.wifi.observe", CapabilityClass::Observe, true, {15000, 65536, false}},
    {"sentinel.ble.observe", CapabilityClass::Observe, true, {15000, 65536, false}},
    {"sentinel.rf.observe", CapabilityClass::Observe, true, {15000, 65536, false}},
    {"sentinel.gps.read", CapabilityClass::Observe, true, {5000, 4096, false}},

    // Reserved for later, explicit operator-authorized phases. These are deliberately
    // not exposed to an agent in the governed-worker foundation.
    {"sentinel.gpio.set", CapabilityClass::LocalAction, false, {2000, 4096, false}},
    {"sentinel.ir.transmit", CapabilityClass::Transmit, false, {5000, 4096, true}},

    // Capabilities that must never be reachable through the agent worker interface.
    // Keeping explicit deny entries makes accidental future bridging fail closed.
    {"bruce.wifi.deauth", CapabilityClass::Excluded, false, {0, 0, true}},
    {"bruce.rf.jam", CapabilityClass::Excluded, false, {0, 0, true}},
    {"bruce.hid.execute", CapabilityClass::Excluded, false, {0, 0, false}},
};

constexpr size_t kCapabilityCount = sizeof(kCapabilities) / sizeof(kCapabilities[0]);

bool validString(const char *value) {
    return value != nullptr && value[0] != '\0';
}

bool limitsWithin(const ExecutionLimits &requested, const ExecutionLimits &ceiling) {
    if (requested.max_runtime_ms == 0 || requested.max_output_bytes == 0) return false;
    if (requested.max_runtime_ms > ceiling.max_runtime_ms) return false;
    if (requested.max_output_bytes > ceiling.max_output_bytes) return false;
    if (requested.transmit && !ceiling.transmit) return false;
    return true;
}

} // namespace

const CapabilityDescriptor *findCapability(const char *name) {
    if (!validString(name)) return nullptr;

    for (size_t i = 0; i < kCapabilityCount; ++i) {
        if (std::strcmp(kCapabilities[i].name, name) == 0) return &kCapabilities[i];
    }
    return nullptr;
}

size_t capabilityCount() { return kCapabilityCount; }

const CapabilityDescriptor *capabilityAt(size_t index) {
    if (index >= kCapabilityCount) return nullptr;
    return &kCapabilities[index];
}

PolicyDecision evaluatePolicy(const JobRequest &job, const AuthorityGrant &grant) {
    if (!validString(job.job_id) || !validString(job.capability)) {
        return {false, DecisionReason::UnknownCapability};
    }

    const CapabilityDescriptor *descriptor = findCapability(job.capability);
    if (descriptor == nullptr) return {false, DecisionReason::UnknownCapability};

    if (descriptor->capability_class == CapabilityClass::Excluded) {
        return {false, DecisionReason::ExcludedCapability};
    }

    if (!descriptor->agent_exposed) {
        return {false, DecisionReason::AgentExposureDenied};
    }

    if (!limitsWithin(job.requested, descriptor->ceiling)) {
        return {false, DecisionReason::InvalidLimits};
    }

    switch (descriptor->capability_class) {
        case CapabilityClass::Introspection:
        case CapabilityClass::Observe:
            if (job.requested.transmit) return {false, DecisionReason::ScopeViolation};
            return {true, DecisionReason::Allowed};

        case CapabilityClass::LocalAction:
            if (!grant.allow_local_action) return {false, DecisionReason::MissingApproval};
            return {true, DecisionReason::Allowed};

        case CapabilityClass::Transmit:
            if (!job.requested.transmit) return {false, DecisionReason::ScopeViolation};
            if (!grant.allow_transmit) return {false, DecisionReason::MissingApproval};
            return {true, DecisionReason::Allowed};

        case CapabilityClass::Excluded:
        default:
            return {false, DecisionReason::ExcludedCapability};
    }
}

const char *decisionReasonName(DecisionReason reason) {
    switch (reason) {
        case DecisionReason::Allowed: return "allowed";
        case DecisionReason::UnknownCapability: return "unknown_capability";
        case DecisionReason::AgentExposureDenied: return "agent_exposure_denied";
        case DecisionReason::ExcludedCapability: return "excluded_capability";
        case DecisionReason::InvalidLimits: return "invalid_limits";
        case DecisionReason::ScopeViolation: return "scope_violation";
        case DecisionReason::MissingApproval: return "missing_approval";
        default: return "unknown";
    }
}

const char *capabilityClassName(CapabilityClass capability_class) {
    switch (capability_class) {
        case CapabilityClass::Introspection: return "introspection";
        case CapabilityClass::Observe: return "observe";
        case CapabilityClass::LocalAction: return "local_action";
        case CapabilityClass::Transmit: return "transmit";
        case CapabilityClass::Excluded: return "excluded";
        default: return "excluded";
    }
}

} // namespace sentinel
