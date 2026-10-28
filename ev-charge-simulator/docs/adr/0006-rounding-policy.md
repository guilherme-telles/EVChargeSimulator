# ADR-0006: Rounding and pt-BR presentation

- Status: Accepted
- Date: 2026-10-26
- Related: RN-05, RN-06, UT-015, §12.3

## Decision

- All computation uses `double` at full precision; rounding happens only in `ev_app/Formatting`.
- Rounding: **half away from zero** at the display precision (`std::round(x × 10^d) / 10^d`).
- Formatting is locale-independent (`std::to_chars`) and then rendered as pt-BR: `.` thousands, `,` decimals.

| Quantity | Format | Example |
|----------|--------|---------|
| Money | 2 decimals | `R$ 1.234,56` |
| Energy | 2 decimals | `36,00 kWh` |
| Power | 2 decimals | `7,40 kW` |
| Range | 0 decimals | `225 km` |
| Duration | h + min (rounded minute) and decimal hours (≤ 2 decimals, trailing zeros trimmed) | `5 h 24 min`, `5,41 h`, `1,5 h` |
| Efficiency | ≤ 1 decimal | `90%` |

## Consequences

Binary floating point may round values such as 2.675 down; acceptable within the ±R$ 0,01 tolerance (§12.3).
