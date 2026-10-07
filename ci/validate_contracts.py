#!/usr/bin/env python3
from __future__ import annotations

import json
from pathlib import Path

from jsonschema import Draft202012Validator, ValidationError

ROOT = Path(__file__).resolve().parents[1]
SCHEMAS = ROOT / "schemas"


def load_schema(name: str) -> dict:
    with (SCHEMAS / name).open("r", encoding="utf-8") as handle:
        schema = json.load(handle)
    Draft202012Validator.check_schema(schema)
    return schema


def require_valid(schema: dict, instance: dict) -> None:
    Draft202012Validator(schema).validate(instance)


def require_invalid(schema: dict, instance: dict) -> None:
    try:
        Draft202012Validator(schema).validate(instance)
    except ValidationError:
        return
    raise AssertionError(f"instance unexpectedly validated: {instance!r}")


def main() -> int:
    capability = load_schema("capability.schema.json")
    job = load_schema("job.schema.json")
    evidence = load_schema("evidence.schema.json")

    require_valid(
        capability,
        {
            "name": "sentinel.rf.observe",
            "class": "observe",
            "agent_exposed": True,
            "ceiling": {"max_runtime_ms": 15000, "max_output_bytes": 65536, "transmit": False},
        },
    )
    require_invalid(
        capability,
        {
            "name": "sentinel.rf.observe",
            "class": "superuser",
            "agent_exposed": True,
            "ceiling": {"max_runtime_ms": 15000, "max_output_bytes": 65536, "transmit": False},
        },
    )

    valid_job = {
        "version": 1,
        "job_id": "rtf-01-test",
        "capability": "sentinel.rf.observe",
        "authority_ref": "authority:rtf:test",
        "scope": {"band": "433mhz"},
        "limits": {"max_runtime_ms": 10000, "max_output_bytes": 4096, "transmit": False},
    }
    require_valid(job, valid_job)

    missing_authority = dict(valid_job)
    missing_authority.pop("authority_ref")
    require_invalid(job, missing_authority)

    self_authorizing = dict(valid_job)
    self_authorizing["allow_transmit"] = True
    require_invalid(job, self_authorizing)

    malformed_limits = dict(valid_job)
    malformed_limits["limits"] = {
        "max_runtime_ms": 0,
        "max_output_bytes": 4096,
        "transmit": False,
    }
    require_invalid(job, malformed_limits)

    require_valid(
        evidence,
        {
            "version": 1,
            "job_id": "rtf-01-test",
            "device_id": "sentinel-test",
            "capability": "sentinel.rf.observe",
            "decision": "allow",
            "reason": "allowed",
            "status": "completed",
            "elapsed_ms": 1200,
            "output_bytes": 1024,
            "previous_hash": None,
            "record_hash": None,
        },
    )
    require_invalid(
        evidence,
        {
            "version": 1,
            "job_id": "rtf-01-test",
            "device_id": "sentinel-test",
            "capability": "sentinel.rf.observe",
            "decision": "allow",
            "reason": "allowed",
            "status": "completed",
            "elapsed_ms": -1,
            "output_bytes": 1024,
        },
    )

    print("Sentinel JSON contracts passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
