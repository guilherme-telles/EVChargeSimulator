# ADR-0005: XML number and compatibility policy

- Status: Accepted
- Date: 2026-10-26
- Related: RF-009, §6.2, §6.3, IT-002..IT-007, RNF-008

## Decision

- **Canonical numbers** use `.` as decimal separator and are written with shortest round-trip precision.
- **Decimal comma (IT-004):** a value with exactly one `,` and no `.` (e.g. `7,4`) is normalized to `7.4` and
  produces warning **WARN-004**. Any other use of `,` (e.g. `1,060.5`, `1,2,3`) is rejected as ambiguous (IO-001).
- **Unknown elements (IT-007):** ignored with warning **WARN-003** (`scenario/...` path).
- **Unit attributes** are optional; when present they must match (`kWh`, `kW`, `kWh/100km`, `km`).
- **schemaVersion** must be exactly `1.0`. A higher major version asks the user to update the app (IT-005).
- **Semantic validation**: loaded scenarios run the same `validate()` as the engine; a file with invalid values
  is rejected (IO-001 + VAL codes). Saving an invalid scenario is also refused, so saved files always reopen.
- Limits: 1 MB (checked before reading), element depth 8, catalog ≤ 1000 vehicles.

## Consequences

Users cannot save half-filled scenarios. Revisit if "drafts" become a requirement.
