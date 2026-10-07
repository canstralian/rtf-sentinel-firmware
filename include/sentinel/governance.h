// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once

#include <stddef.h>
#include <stdint.h>

namespace sentinel {

enum class CapabilityClass : uint8_t {
    Introspection = 0,
    Observe = 1,
    LocalAction = 2,
    Transmit = 3,
    Excluded = 255,
};

enum class DecisionReason : uint8_t {
    Allowed = 0,
    UnknownCapability,
    AgentExposureDenied,
    ExcludedCapability,
    InvalidLimits,
    ScopeViolation,
    MissingApproval,
};

struct ExecutionLimits {
    uint32_t max_runtime_ms;
    uint32_t max_output_bytes;
    bool transmit;
};

struct CapabilityDescriptor {
    const char *name;
    CapabilityClass capability_class;
    bool agent_exposed;
    ExecutionLimits ceiling;
};

struct JobRequest {
    const char *job_id;
    const char *capability;
    ExecutionLimits requested;
};

struct AuthorityGrant {
    bool allow_local_action;
    bool allow_transmit;
};

struct PolicyDecision {
    bool allowed;
    DecisionReason reason;
};

const CapabilityDescriptor *findCapability(const char *name);
size_t capabilityCount();
const CapabilityDescriptor *capabilityAt(size_t index);

PolicyDecision evaluatePolicy(const JobRequest &job, const AuthorityGrant &grant);

const char *decisionReasonName(DecisionReason reason);
const char *capabilityClassName(CapabilityClass capability_class);

} // namespace sentinel
