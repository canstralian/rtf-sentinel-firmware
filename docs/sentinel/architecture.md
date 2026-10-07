# Red Team Forge Sentinel firmware foundation

This fork keeps Bruce as the hardware and device-support upstream while adding a narrow governed-worker boundary for Red Team Forge.

## Architectural decision

The ESP32 is an execution worker, not an authority source.

```text
agent / MCP client
       |
       v
Red Team Forge
admission + authority + evidence
       |
       v
structured governed job
       |
       v
Sentinel firmware
capability registry -> local policy -> bounded execution -> evidence
       |
       v
Bruce hardware modules
```

The central invariant is:

> Intent is not authority.

A model, script, transport peer, or job payload cannot grant itself capabilities. Authority is validated separately and passed into the local policy boundary as trusted state.

## v0.1 boundary

The first phase is intentionally read-only from the agent interface.

Agent-exposed capabilities:

- `sentinel.device.status`
- `sentinel.device.capabilities`
- `sentinel.wifi.observe`
- `sentinel.ble.observe`
- `sentinel.rf.observe`
- `sentinel.gps.read`

Reserved but not agent-exposed:

- `sentinel.gpio.set`
- `sentinel.ir.transmit`

Explicitly excluded from the agent worker interface:

- disruptive Wi-Fi operations
- RF jamming
- arbitrary HID payload execution

Bruce can continue to provide its existing manual features. This boundary governs only the Red Team Forge worker interface and must not become a generic command passthrough.

## Fail-closed rules

A request is denied when any of the following are true:

1. the capability is unknown;
2. the capability is explicitly excluded;
3. the capability is not exposed to agents;
4. requested runtime or output exceeds the capability ceiling;
5. requested transmission does not match the capability scope;
6. required approval is absent.

Unknown states resolve to denial.

## Bounded jobs

`BoundedJob` supplies two local execution budgets:

- maximum runtime;
- maximum produced output.

Capability adapters are responsible for checking `shouldStop()` during long-running work and calling `accountOutput()` before emitting output. A ceiling violation changes the job to a terminal fail-closed state.

This is deliberately a small primitive rather than a scheduler. Cancellation, transport integration, FreeRTOS task ownership, and forced teardown belong in later milestones.

## Contracts

The `schemas/` directory is the wire-contract source of truth:

- `capability.schema.json` describes discoverable worker capabilities;
- `job.schema.json` describes a bounded job request;
- `evidence.schema.json` describes the execution result/evidence envelope.

The job schema contains an opaque `authority_ref`; it does not contain self-asserted permissions.

## Evidence

v0.1 defines the evidence shape but does not claim cryptographic provenance yet. `previous_hash` and `record_hash` remain nullable until canonical serialization and hash chaining are implemented.

A future milestone should make evidence append-only and replayable across the host/worker boundary.

## Gotchi / adaptive loop

The Pwnagotchi-inspired learning loop is intentionally deferred until the authority boundary is stable.

When added, the adaptive loop may tune parameters such as observation ordering, dwell time, sampling cadence, and exploration rate. It must operate strictly inside the admitted capability and scope. Learning can optimize execution; it cannot widen authority.

## Next milestone

1. Add a framed serial transport carrying schema-versioned jobs.
2. Serialize capability discovery from the registry.
3. Attach evidence emission to every allow/deny decision.
4. Add host-side tests that prove observe-only jobs cannot request transmission.
5. Integrate a single passive observation adapter before adding MCP.
