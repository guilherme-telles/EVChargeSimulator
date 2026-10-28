# ADR-0004: Segmented charge curve semantics

- Status: Accepted
- Date: 2026-10-26
- Related: D-04, RF-012, RN-07, UT-013

## Decision

- A curve is a list of segments `[fromSoc, toSoc) → maxPowerKw`.
- Inside a segment, `maxPowerKw` **replaces** the vehicle's constant accepted power.
- Outside every segment, the vehicle's constant accepted power applies.
- In both cases the result is capped by the supply limit: `min(charger power, installation limit)`.
- Time is integrated per SoC interval: `t = Σ (C_util × ΔSoC / 100 / η) / P_segment`.
- Segments must be inside 0..100, have `from < to`, not overlap (adjacent allowed) and have power > 0
  (VAL-009 / VAL-003).
- Without a curve, the engine uses constant power and emits WARN-001 (RN-07).
- The reported "effective power" is the session average `E_rede / t`; with constant power it equals the constant.

## Consequences

Temperature, pre-conditioning and power sharing are not modelled (spec §Sumário executivo).
