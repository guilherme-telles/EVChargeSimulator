# ADR-0003: Composite tariff model

- Status: Accepted
- Date: 2026-10-26
- Related: D-02, RF-005, §2.1, UT-010, UT-011

## Decision

`CompositeTariff` (behind `ITariffStrategy`) computes:

```
energy = E_rede × energyRate
time   = duration_min × minuteRate
fixed  = fixedFee
total  = energy + time + fixed
```

A component set to zero is effectively disabled, so a "simple" energy-only tariff is a composite tariff with
`fixedFee = 0` and `minuteRate = 0`. Only `BRL` is accepted in the MVP.

## Consequences

Time-of-use tariffs (per time slot) are out of scope; they can be added as another `ITariffStrategy`.
